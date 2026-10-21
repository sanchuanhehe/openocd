#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Reset/boot breakpoint regression on WS63; leaves the application halted.

Requires an idle OpenOCD, enabled Flash software breakpoints, a full 4 MiB
Flash backup from this board, and an application instruction reached after boot.
Resets the board, borrows 64 RAM bytes, and temporarily patches one Flash sector.
Failures leave the CPU halted; retain the supplied backup and output directory.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import time

from check_core import Tcl


def durable(path, data):
    with path.open("xb") as file:
        file.write(data)
        file.flush()
        os.fsync(file.fileno())
    for directory in (path.parent.resolve(), path.parent.resolve().parent):
        fd = os.open(directory, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)


def expect_failure(client, command):
    try:
        client.call(command)
    except RuntimeError:
        return
    raise AssertionError("command unexpectedly succeeded: " + command)


def wait_halt(client, seconds=15):
    # FlashBoot can reset the entire chip; allow OpenOCD to reexamine SWD.
    deadline = time.monotonic() + seconds
    while True:
        try:
            client.call("wait_halt 1000")
            return
        except RuntimeError:
            if time.monotonic() >= deadline:
                raise
            time.sleep(0.05)


def check(client, address, backup, output):
    image = backup.read_bytes()
    if len(image) != 0x400000 or address % 2 or not 0x200000 <= address < 0x600000 - 4:
        raise RuntimeError("requires a full 4 MiB backup and an aligned Flash instruction")
    if client.call("bp").strip() or client.call("wp").strip():
        raise RuntimeError("remove existing breakpoints/watchpoints first")
    if client.call("WS63.cpu curstate") != "halted":
        raise RuntimeError("halt in the running application first")
    client.call("flash probe WS63.flash")
    sector = address & ~4095
    offset = sector - 0x200000
    original = image[offset:offset + 4096]
    if client.read("WS63.ap1", sector, 8, 4096) != original:
        raise RuntimeError("backup differs from current application sector")
    code_offset = address - sector
    width = 4 if original[code_offset] & 3 == 3 else 2
    if code_offset + width > 4096:
        raise RuntimeError("choose an instruction contained within one sector")
    expected = bytes.fromhex("73001000" if width == 4 else "0290")
    if original[code_offset:code_offset + width] == expected:
        raise RuntimeError("choose an instruction that is not already EBREAK")
    registers = client.registers(["pc", "sp", "mstatus"])
    ram = (registers["sp"] - 128) & ~3
    if not 0xA00000 <= ram < ram + 64 <= 0xA88000:
        raise RuntimeError("requires an SRAM application stack")
    saved_ram = client.read("WS63.cpu", ram, 8, 64)
    durable(output / "ram.bin", saved_ram)
    durable(output / "sector.bin", original)
    durable(output / "context.json", json.dumps({
        "ram": ram, "sector": sector, "address": address, "registers": registers,
        "backup": str(backup.resolve()), "backup_sha256": hashlib.sha256(image).hexdigest(),
    }, indent=2).encode())
    success = False
    ram_restored = False
    try:
        client.write(ram, bytes.fromhex("1305b0076f000000"))
        client.call(f"bp {ram:#x} 4")
        client.put({"mstatus": registers["mstatus"] & ~8})
        client.call(f"resume {ram + 4:#x}")
        assert client.call("WS63.cpu curstate") == "running"
        client.call("reset halt")
        assert client.registers(["pc"])["pc"] == 0x100000
        assert client.read("WS63.cpu", ram, 8, 4) == bytes.fromhex("1305b007")
        client.call(f"resume {ram:#x}")
        wait_halt(client, 3)
        hit = client.registers(["pc", "dcsr"])
        assert hit["pc"] == ram and (hit["dcsr"] >> 6) & 7 == 2, hit
        client.call(f"rbp {ram:#x}")
        # sw a0,0(a1); jal zero,0. The watchpoint must also survive reset.
        client.write(ram, bytes.fromhex("23a0a5006f000000"))
        client.call(f"wp {ram + 32:#x} 4 w")
        client.call("reset halt")
        client.put({"a0": 0x12345678, "a1": ram + 32})
        client.call(f"resume {ram:#x}")
        wait_halt(client, 3)
        hit = client.registers(["pc", "dcsr"])
        assert hit["pc"] == ram and (hit["dcsr"] >> 6) & 7 == 2, hit
        client.call(f"rwp {ram + 32:#x}")
        client.call("step")
        assert client.read("WS63.cpu", ram + 32, 32, 1) == bytes.fromhex("78563412")
        client.write(ram, saved_ram)
        ram_restored = True
        print("RAM software breakpoint becomes a working hardware breakpoint across reset: PASS", flush=True)
        print("Watchpoint rearmed across reset and traps the intended store: PASS", flush=True)

        client.call("reset halt")
        client.call(f"bp {address:#x} {width} hw")
        client.call("resume")
        wait_halt(client)
        assert client.registers(["pc"])["pc"] == address
        client.call(f"rbp {address:#x}")
        client.call(f"bp {address:#x} {width}")
        assert client.read("WS63.ap1", address, 8, width) == expected
        expect_failure(client, f"flash erase_sector WS63.flash {offset // 4096} {offset // 4096}")
        expect_failure(client, f"flash write_bank WS63.flash {{{output.resolve() / 'sector.bin'}}} {offset}")
        assert client.read("WS63.ap1", address, 8, width) == expected
        print("Flash erase/program overlapping an installed breakpoint is rejected: PASS", flush=True)

        client.call("reset halt")
        assert client.registers(["pc"])["pc"] == 0x100000
        assert client.read("WS63.ap1", sector, 8, 4096) == original
        client.call("resume")
        wait_halt(client)
        hit = client.registers(["pc", "dcsr"])
        assert hit["pc"] == address and (hit["dcsr"] >> 6) & 7 == 2, hit
        client.call(f"rbp {address:#x}")
        assert client.read("WS63.ap1", sector, 8, 4096) == original
        print("Flash breakpoint restored before signed boot and rearmed as hardware: PASS", flush=True)
        client.call(f"bp {address:#x} {width} hw")
        client.call("reset run")
        wait_halt(client)
        hit = client.registers(["pc", "dcsr"])
        assert hit["pc"] == address and (hit["dcsr"] >> 6) & 7 == 2, hit
        client.call(f"rbp {address:#x}")
        print("reset run rearms the hardware breakpoint before boot: PASS", flush=True)
        success = True
    finally:
        client.call("halt")
        for location in (ram, address):
            client.call(f"catch {{rbp {location:#x}}}")
        client.call(f"catch {{rwp {ram + 32:#x}}}")
        if not ram_restored:
            client.write(ram, saved_ram)
        actual = client.read("WS63.ap1", sector, 8, 4096)
        if actual != original:
            # Do not resume after recovery: ordinary programming requires a reset,
            # and a failed programming operation latches the session unsafe.
            client.call(f"flash erase_sector WS63.flash {offset // 4096} {offset // 4096}")
            client.call(f"flash write_bank WS63.flash {{{output.resolve() / 'sector.bin'}}} {offset}")
            assert client.read("WS63.ap1", sector, 8, 4096) == original
        print("Application sector restored; CPU halted; test " + ("passed" if success else "failed"), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=6666)
    parser.add_argument("--address", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--flash-backup", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    client = Tcl(args.port)
    client.socket.settimeout(60)
    try:
        check(client, args.address, args.flash_backup, args.output)
    finally:
        client.socket.close()


if __name__ == "__main__":
    main()
