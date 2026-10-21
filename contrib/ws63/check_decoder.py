#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compare native WS63 short-access decoding with the SDK objdump (no board)."""

import argparse
import ctypes
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--objdump", required=True, help="WS63 SDK riscv32-linux-musl-objdump"
    )
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    names = ["zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1"]
    names += [f"a{i}" for i in range(8)] + [f"s{i}" for i in range(2, 12)]
    names += ["t3", "t4", "t5", "t6"]
    registers = {name: i for i, name in enumerate(names)}
    registers["fp"] = 8
    with tempfile.TemporaryDirectory() as temp:
        directory = Path(temp)
        source = directory / "decode.c"
        source.write_text("""#include "src/target/riscv/ws63_insn.h"
int decode(uint32_t code, int32_t *out)
{
    struct ws63_access a;
    if (!ws63_decode_access(code, &a))
        return 0;
    out[0] = a.base; out[1] = a.size; out[2] = a.offset; out[3] = a.load;
    return 1;
}
""")
        library = directory / "decode.so"
        subprocess.run(
            [
                args.cc,
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fPIC",
                "-shared",
                "-I",
                str(root),
                str(source),
                "-o",
                str(library),
            ],
            check=True,
        )
        decode = ctypes.CDLL(str(library)).decode
        decode.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_int32)]
        decode.restype = ctypes.c_int
        binary = directory / "short.bin"
        binary.write_bytes(
            b"".join(i.to_bytes(2, "little") for i in range(65536) if i & 3 != 3)
        )
        result = subprocess.run(
            [args.objdump, "-D", "-b", "binary", "-m", "riscv:rv32", str(binary)],
            text=True,
            capture_output=True,
            check=True,
            timeout=30,
        )
        sizes = {
            "lbu": 1,
            "lhu": 2,
            "lw": 4,
            "flw": 4,
            "sb": 1,
            "sh": 2,
            "sw": 4,
            "fsw": 4,
        }
        seen = set()
        for line in result.stdout.splitlines():
            match = re.match(
                r"\s*[0-9a-f]+:\s+([0-9a-f]{4})\s+(\w+)\s+[^,]+,(\d+)\(([^)]+)\)", line
            )
            if not match or match[2] not in sizes:
                continue
            raw, op, offset, base = match.groups()
            code = int(raw, 16)
            output = (ctypes.c_int32 * 4)()
            expected = [
                registers[base],
                sizes[op],
                int(offset),
                int(op in ("lbu", "lhu", "lw", "flw")),
            ]
            assert decode(code, output) and list(output) == expected, line
            seen.add(code)
        for code in range(65536):
            if code & 3 != 3:
                assert bool(decode(code, (ctypes.c_int32 * 4)())) == (
                    code in seen
                ), hex(code)
        assert len(seen) == 24512, len(seen)
        print(
            "24,512 scalar memory encodings match SDK objdump; all other short encodings rejected: PASS"
        )


if __name__ == "__main__":
    main()
