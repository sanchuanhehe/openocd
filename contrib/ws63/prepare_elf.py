#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""ELF/DWARF metadata and conservative unwind information for WS63 SDK binaries.

Requires pyelftools. `prepare` adds missing CFI to a COPY of the ELF, never to
firmware bytes. Unsupported prologues/ambiguous control flow are left untouched.
"""

import argparse
import bisect
import copy
import hashlib
import io
from pathlib import Path
import re
import shutil
import struct
import subprocess


class DebugError(RuntimeError):
    pass


REGS = (
    ["zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1"]
    + ["a%d" % i for i in range(8)]
    + ["s%d" % i for i in range(2, 12)]
    + ["t3", "t4", "t5", "t6"]
)
REGNUM = {name: i for i, name in enumerate(REGS)}
REGNUM["fp"] = 8


def uleb(value):
    out = bytearray()
    while True:
        byte = value & 127
        value >>= 7
        out.append(byte | (128 if value else 0))
        if not value:
            return bytes(out)


def sdk_tool(elf, name):
    found = shutil.which("riscv32-linux-musl-" + name)
    if found:
        return found
    for parent in Path(elf).resolve().parents:
        found = list(
            parent.glob(
                "tools/bin/compiler/riscv/*/cc_riscv32_musl_fp/bin/riscv32-linux-musl-"
                + name
            )
        )
        if found:
            return str(found[0])
    raise DebugError("cannot find RISC-V %s; pass --%s" % (name, name))


class Image:
    def __init__(self, path):
        try:
            from elftools.elf.elffile import ELFFile
            from elftools.dwarf.callframe import FDE
        except ImportError as error:
            raise DebugError(
                "ELF features require pyelftools (python3 -m pip install pyelftools)"
            ) from error
        self.path = str(Path(path).resolve())
        self.data = Path(path).read_bytes()
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        self.elf = ELFFile(io.BytesIO(self.data))
        if (
            self.elf.elfclass != 32
            or not self.elf.little_endian
            or self.elf["e_machine"] != "EM_RISCV"
        ):
            raise DebugError("requires a little-endian RV32 ELF")
        table = self.elf.get_section_by_name(".symtab")
        if table is None:
            raise DebugError("ELF has no symbol table")
        self.symbols = {
            s.name: int(s["st_value"])
            for s in table.iter_symbols()
            if s.name and s["st_shndx"] != "SHN_UNDEF"
        }
        self.functions = {
            int(s["st_value"]): (s.name, int(s["st_size"]))
            for s in table.iter_symbols()
            if s["st_info"]["type"] == "STT_FUNC" and s["st_size"] > 0
        }
        self.starts = sorted(self.functions)
        self.dwarf = self.elf.get_dwarf_info()
        self.types = {}
        self._types_loaded = False
        self.fdes = []
        if self.dwarf.has_CFI():
            self.fdes = [e for e in self.dwarf.CFI_entries() if isinstance(e, FDE)]
        self.fdes.sort(key=lambda e: e["initial_location"])
        self.fde_starts = [e["initial_location"] for e in self.fdes]

    def symbol(self, name):
        try:
            return self.symbols[name]
        except KeyError:
            raise DebugError("ELF missing symbol " + name)

    def describe(self, pc):
        i = bisect.bisect_right(self.starts, pc) - 1
        if i < 0:
            return "0x%08x" % pc
        start = self.starts[i]
        name, size = self.functions[start]
        return "%s+0x%x" % (name, pc - start) if pc < start + size else "0x%08x" % pc

    def fde(self, pc):
        i = bisect.bisect_right(self.fde_starts, pc) - 1
        if i >= 0:
            e = self.fdes[i]
            if pc < e["initial_location"] + e["address_range"]:
                return e
        return None

    def type_layout(self, name):
        if not self._types_loaded:
            for cu in self.dwarf.iter_CUs():
                for die in cu.iter_DIEs():
                    attr = die.attributes.get("DW_AT_name")
                    if attr and die.tag in ("DW_TAG_typedef", "DW_TAG_structure_type"):
                        key = attr.value.decode(errors="replace")
                        if key not in self.types:
                            self.types[key] = die
            self._types_loaded = True
        if name not in self.types:
            raise DebugError("ELF missing DWARF type " + name)
        die = self.types[name]
        while die.tag in (
            "DW_TAG_typedef",
            "DW_TAG_const_type",
            "DW_TAG_volatile_type",
        ):
            die = die.get_DIE_from_attribute("DW_AT_type")
        if "DW_AT_byte_size" not in die.attributes:
            raise DebugError("incomplete type " + name)
        fields = {}
        for child in die.iter_children():
            if child.tag != "DW_TAG_member" or "DW_AT_name" not in child.attributes:
                continue
            offset = child.attributes.get("DW_AT_data_member_location")
            if offset is None or not isinstance(offset.value, int):
                continue
            typ = child.get_DIE_from_attribute("DW_AT_type")
            while typ.tag in (
                "DW_TAG_typedef",
                "DW_TAG_const_type",
                "DW_TAG_volatile_type",
            ):
                typ = typ.get_DIE_from_attribute("DW_AT_type")
            size = typ.attributes.get("DW_AT_byte_size")
            fields[child.attributes["DW_AT_name"].value.decode()] = (
                offset.value,
                size.value if size else 4,
            )
        return die.attributes["DW_AT_byte_size"].value, fields

    def check_target(self, hart):
        """Compare all executable Flash sections; RAM data never determines identity."""
        total = 0
        for sec in self.elf.iter_sections():
            a = sec["sh_addr"]
            n = sec["sh_size"]
            if sec["sh_flags"] & 6 == 6 and n and 0x200000 <= a < a + n <= 0x600000:
                if hart.read_mem(a, n) != sec.data():
                    raise DebugError("ELF/target mismatch in " + sec.name)
                total += n
        if total == 0:
            raise DebugError("no executable Flash sections to validate")
        return total


def disassemble(image, objdump=None):
    tool = objdump or sdk_tool(image.path, "objdump")
    output = subprocess.check_output([tool, "-d", image.path], text=True)
    insns = {}
    for line in output.splitlines():
        parts = line.split("\t")
        if len(parts) < 3 or not re.match(r"^\s*[0-9a-f]+:$", parts[0]):
            continue
        address = int(parts[0].strip()[:-1], 16)
        raw = parts[1].replace(" ", "")
        if not re.fullmatch("[0-9a-f]+", raw):
            continue
        text = " ".join(p.strip() for p in parts[2:]).split("#")[0].strip()
        op, _, args = text.partition(" ")
        insns[address] = (len(raw) // 2, op, args.strip())
    return insns


def reglist(text):
    regs = []
    for part in text.replace(" ", "").split(","):
        if "-" in part:
            a, b = part.split("-")
            if a[0] != b[0] or not a[1:].isdigit() or not b[1:].isdigit():
                raise ValueError("register range")
            regs += [REGNUM[a[0] + str(i)] for i in range(int(a[1:]), int(b[1:]) + 1)]
        else:
            regs.append(REGNUM[part])
    return regs


def stack_store(saved, original, offset, size, register=None):
    """Invalidate every saved word touched by a known CFA-relative store."""
    for reg, location in list(saved.items()):
        if offset < location + 4 and location < offset + size:
            del saved[reg]
    if size == 4 and register in original:
        # Prefer the newest copy. Keeping an older copy can retain a reused slot.
        saved[register] = offset


def unwind_states(start, size, instructions):
    """Dataflow for fixed stack frames, integer stores and WS63 push/stmia.

    Track CFA-relative register addresses, original-register locations and SP.
    Unknown SP writes or inconsistent merges make the function ineligible.
    Calls clobber scratch address calculations. Branches preserve frame state.
    """
    # state = (sp delta, saved original regs, CFA-relative address expressions,
    #          registers still containing their entry values)
    preserved = {1, 8, 9, *range(18, 28)}
    states = {}
    pending = [(start, (0, {}, {2: 0}, set(preserved)))]
    while pending:
        pc, state = pending.pop()
        if pc not in instructions or not start <= pc < start + size:
            continue
        if pc in states:
            # Ignore temporary address expressions at merge points. CFA/saves must agree.
            old = states[pc]
            if old[:2] != state[:2]:
                return {}
            merged = {r: v for r, v in old[2].items() if state[2].get(r) == v}
            original = old[3] & state[3]
            if merged == old[2] and original == old[3]:
                continue
            state = (old[0], dict(old[1]), merged, original)
        states[pc] = copy.deepcopy(state)
        delta, saved, expr, original = copy.deepcopy(state)
        length, op, args = instructions[pc]
        nextpc = pc + length
        parts = [s.strip() for s in args.split(",")]
        try:
            if op in ("addi", "add") and len(parts) == 3 and parts[0] in REGNUM:
                rd, rs = REGNUM[parts[0]], REGNUM.get(parts[1])
                original.discard(rd)
                try:
                    imm = int(parts[2], 0)
                except ValueError:
                    imm = None
                if imm is not None and rs in expr:
                    expr[rd] = expr[rs] + imm
                else:
                    expr.pop(rd, None)
                if rd == 2:
                    if 2 not in expr:
                        return {}
                    delta = expr[2]
            elif op == "mv" and len(parts) == 2:
                rd, rs = REGNUM.get(parts[0]), REGNUM.get(parts[1])
                if rd != rs:
                    original.discard(rd)
                if rd == 2 and rs not in expr:
                    return {}
                if rs in expr:
                    expr[rd] = expr[rs]
                else:
                    expr.pop(rd, None)
                if rd == 2:
                    delta = expr[2]
            elif op in ("push", "pop", "popret"):
                match = re.fullmatch(r"\{([^}]+)\},\s*(-?\d+)", args)
                if not match:
                    return {}
                regs = reglist(match[1])
                amount = int(match[2])
                if op == "push":
                    for i, r in enumerate(regs):
                        stack_store(saved, original, delta - 4 * (i + 1), 4, r)
                else:
                    for i, r in enumerate(regs):
                        if saved.get(r) == delta + amount - 4 * (i + 1):
                            saved.pop(r, None)
                            original.add(r)
                        else:
                            original.discard(r)
                        expr.pop(r, None)
                delta += amount
                expr[2] = delta
            elif op in ("sb", "sh", "sw", "sd", "fsw", "fsd"):
                match = re.fullmatch(
                    r"([^,]+),(-?\d+)\(([^)]+)\)", args.replace(" ", "")
                )
                if not match:
                    return {}
                base = REGNUM.get(match[3])
                if base in expr:
                    width = {"sb": 1, "sh": 2, "sw": 4, "sd": 8, "fsw": 4, "fsd": 8}[op]
                    r = REGNUM.get(match[1]) if op == "sw" else None
                    stack_store(saved, original, expr[base] + int(match[2]), width, r)
            elif op in ("stmia", "ldmia"):
                match = re.fullmatch(r"\{([^}]+)\},\s*\(([^)]+)\)", args)
                if match:
                    regs = reglist(match[1])
                    base = REGNUM[match[2]]
                    if base not in expr:
                        return {}
                    if op == "stmia":
                        for i, r in enumerate(regs):
                            stack_store(
                                saved, original, expr[base] + 4 * (len(regs) - i - 1), 4, r
                            )
                    else:
                        base_offset = expr[base]
                        for i, r in enumerate(regs):
                            if saved.get(r) == base_offset + 4 * (len(regs) - i - 1):
                                saved.pop(r, None)
                                original.add(r)
                            else:
                                original.discard(r)
                            expr.pop(r, None)
            elif (
                op in ("lw", "li", "lui", "l.li", "csrr", "csrrc", "csrrs")
                and parts[0] in REGNUM
            ):
                rd = REGNUM[parts[0]]
                if rd == 2:
                    return {}
                original.discard(rd)
                expr.pop(rd, None)
                if op == "lw":
                    match = re.fullmatch(
                        r"[^,]+,(-?\d+)\(([^)]+)\)", args.replace(" ", "")
                    )
                    if match and REGNUM.get(match[2]) in expr:
                        location = expr[REGNUM[match[2]]] + int(match[1])
                        if saved.get(rd) == location:
                            saved.pop(rd, None)
                            original.add(rd)
            elif parts and parts[0] == "sp" and op not in ("beq", "bne"):
                return {}
            elif (
                parts
                and parts[0] in REGNUM
                and not op.startswith("b")
                and op
                not in (
                    "sb",
                    "sh",
                    "sd",
                    "sw",
                    "fsw",
                    "fsd",
                    "jr",
                    "csrw",
                    "csrs",
                    "csrc",
                )
            ):
                # Unknown arithmetic invalidates affine addresses and original
                # values. Never reuse a stale temporary as a stack address.
                rd = REGNUM[parts[0]]
                expr.pop(rd, None)
                original.discard(rd)
        except (KeyError, ValueError):
            return {}
        out = (delta, saved, expr, original)
        if op in ("ret", "popret", "mret", "jr"):
            continue
        if op in ("jal", "jal16", "jalr") and not (op == "jal" and parts[0] == "zero"):
            original.discard(1)
            for r in (5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31):
                expr.pop(r, None)
            pending.append((nextpc, out))
            continue
        target_match = re.search(r"(?:^|,)\s*([0-9a-f]+)\s*<", args)
        if op == "j" or (op == "jal" and parts[0] == "zero"):
            if target_match:
                pending.append((int(target_match[1], 16), out))
            continue
        if op.startswith("b") and target_match:
            pending.append((int(target_match[1], 16), copy.deepcopy(out)))
        pending.append((nextpc, out))
        if len(states) > 10000:
            return {}
    return states


def cfi_rows(start, size, states):
    """Emit a standalone CIE/FDE-compatible instruction stream."""
    code = bytearray()
    last = start
    previous = {}
    for pc, state in sorted(states.items()):
        delta, saved, expr, original = state
        if not {1, 8, 9, *range(18, 28)} <= original | saved.keys():
            return None
        if (
            delta > 0
            or delta < -1024 * 1024
            or any(v >= 0 or v % 4 for v in saved.values())
        ):
            return None
        rules = {r: v for r, v in saved.items() if r in (1, 8, 9, *range(18, 28))}
        current = (delta, rules)
        if previous == current:
            continue
        if pc > last:
            code += b"\x04" + struct.pack("<I", pc - last)
            last = pc
        code += b"\x0c" + uleb(2) + uleb(-delta)  # DW_CFA_def_cfa sp, frame_size
        oldrules = previous[1] if previous else {}
        for r in oldrules.keys() - rules.keys():
            code += b"\x08" + uleb(r)  # same_value
        for r, v in sorted(rules.items()):
            code += bytes([0x80 | r]) + uleb(-v // 4)
        previous = current
    return bytes(code)


def prepare(path, output, objdump=None, objcopy=None):
    image = Image(path)
    insns = disassemble(image, objdump)
    section = image.elf.get_section_by_name(".debug_frame")
    data = bytearray(section.data() if section else b"")
    cieoff = len(data)
    body = struct.pack("<I", 0xFFFFFFFF) + b"\x01\x00\x01\x7c\x01\x0c\x02\x00"
    body += bytes((-len(body)) % 4)
    data += struct.pack("<I", len(body)) + body
    count = 0
    for start, (name, size) in sorted(image.functions.items()):
        if name == "OsTaskEntry":
            # This function is entered by the scheduler, not by a normal call.
            # Its apparent return address belongs to the initial context, not
            # to a caller. Explicitly terminate native GDB stack walking here.
            old = image.fde(start)
            if old:
                struct.pack_into("<I", data, old.offset + 12, 0)
            body = struct.pack("<III", cieoff, start, size) + b"\x07\x01"
            body += bytes((-len(body)) % 4)
            data += struct.pack("<I", len(body)) + body
            count += 1
            continue
        if name in (
            "ArchTaskSchedule",
            "SaveRunTask",
            "SwitchNewTask",
        ) or name.startswith(("trap", "exc", "interrupt")):
            continue
        states = unwind_states(start, size, insns)
        if not states or not any(1 in s[1] for s in states.values()):
            continue
        code = cfi_rows(start, size, states)
        if code is None:
            continue
        old = image.fde(start)
        if old:
            # Some vendor push/pop CFI leaves an epilogue's CFA active in a
            # later branch block. Replace only a demonstrable CFA contradiction.
            rows = old.get_decoded().table
            bad = False
            for pc, state in states.items():
                candidates = [r for r in rows if r["pc"] <= pc]
                if not candidates:
                    continue
                cfa = candidates[-1]["cfa"]
                if (
                    cfa.expr is None
                    and cfa.reg in state[2]
                    and state[2][cfa.reg] + cfa.offset != 0
                ):
                    bad = True
                    break
            if (
                not bad
                or old["initial_location"] != start
                or old["address_range"] != size
            ):
                continue
            # Keep all existing CIE offsets stable; a zero-length old FDE covers no PC.
            struct.pack_into("<I", data, old.offset + 12, 0)
        body = struct.pack("<III", cieoff, start, size) + code
        body += bytes((-len(body)) % 4)
        data += struct.pack("<I", len(body)) + body
        count += 1
    output = Path(output)
    if output.resolve() == Path(path).resolve() or output.exists():
        raise DebugError("output must be a new ELF path")
    output.parent.mkdir(parents=True, exist_ok=True)
    import tempfile

    with tempfile.TemporaryDirectory() as tmp:
        frame = Path(tmp) / "frame.bin"
        frame.write_bytes(data)
        tool = objcopy or sdk_tool(path, "objcopy")
        op = "--update-section" if section else "--add-section"
        subprocess.run(
            [tool, op, ".debug_frame=" + str(frame), str(path), str(output)], check=True
        )
    result = Image(output)
    for original in image.elf.iter_sections():
        if original["sh_flags"] & 2:
            new = result.elf.get_section_by_name(original.name)
            if (
                new is None
                or new["sh_addr"] != original["sh_addr"]
                or new.data() != original.data()
            ):
                output.unlink()
                raise DebugError("ELF preparation changed loadable bytes")
    return count


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("command", choices=["prepare"])
    ap.add_argument("elf")
    ap.add_argument("output")
    ap.add_argument("--objdump")
    ap.add_argument("--objcopy")
    ap.add_argument("--rtos-config")
    args = ap.parse_args()
    print(
        "Added unwind entries:",
        prepare(args.elf, args.output, args.objdump, args.objcopy),
    )
    if args.rtos_config:
        image = Image(args.output)
        tsize, fields = image.type_layout("LosTaskCB")
        csize, context = image.type_layout("TaskContext")
        expected = {
            "stackPointer": (0, 4),
            "taskStatus": (4, 2),
            "priority": (6, 2),
            "stackSize": (12, 4),
            "topOfStack": (16, 4),
            "taskId": (20, 4),
            "taskName": (40, 4),
        }
        if (
            tsize != 96
            or csize != 272
            or any(fields.get(k) != v for k, v in expected.items())
        ):
            raise DebugError(
                "native liteos_ws63 requires the SDK v1.0.102 RV32F layout"
            )
        expected_context = {
            "mstatus": (0, 4),
            "mepc": (4, 4),
            "s0": (60, 4),
            "s1": (56, 4),
            **{"s%d" % i: (60 - i * 4, 4) for i in range(2, 12)},
            "fs0": (172, 4),
            "fcsr": (256, 4),
        }
        if any(context.get(k) != v for k, v in expected_context.items()):
            raise DebugError("unsupported saved task context")
        values = " ".join(
            hex(image.symbol(s))
            for s in (
                "g_osTaskCBArray",
                "g_taskMaxNum",
                "g_newTask",
                "g_taskScheduled",
                "g_intCount",
            )
        )
        with open(args.rtos_config, "x") as output:
            output.write("# Generated from the same ELF as the GDB debug image.\n")
            output.write("WS63.cpu configure -rtos liteos_ws63\n")
            output.write("riscv ws63_rtos_symbols " + values + "\n")
        print("RTOS configuration:", args.rtos_config)


if __name__ == "__main__":
    main()
