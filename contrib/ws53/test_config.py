#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check target configuration without opening a probe.

Usage: python3 test_config.py /path/to/openocd [--package /path/to/flat/package]
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("openocd", type=Path)
parser.add_argument("--package", type=Path)
args = parser.parse_args()
binary = str(args.openocd.resolve())
scripts = Path(__file__).resolve().parents[2] / "tcl"

def ws53_targets(chip):
    return (
        f'if {{[target current] ne "{chip}.cpu"}} {{error "wrong default target"}}; '
        f'foreach name {{{chip}.cpu {chip}.ap0 {chip}.ap1 {chip}.ap2}} {{'
        'if {[lsearch -exact [target names] $name] < 0} {error "missing target $name"}}; '
        f'foreach ap {{0 1 2}} {{if {{[{chip}.ap$ap cget -ap-num] != $ap}} '
        '{error "wrong AP number"}}; shutdown'
    )

cases = [
    ("ws53", ["-f", "interface/jlink.cfg", "-f", "target/ws53.cfg", "-c", ws53_targets("WS53")], 0),
    ("ws53-custom-name", ["-f", "interface/jlink.cfg", "-c", "set CHIPNAME custom53", "-f", "target/ws53.cfg", "-c", ws53_targets("custom53")], 0),
    ("ws63", ["-f", "interface/jlink.cfg", "-c", "set WS63_FLASH 0", "-f", "target/ws63.cfg", "-c", "shutdown"], 0),
    ("exclusive-53-63", ["-f", "interface/jlink.cfg", "-f", "target/ws53.cfg", "-c", "riscv ws63 on; shutdown"], 1),
    ("exclusive-63-53", ["-f", "interface/jlink.cfg", "-c", "set WS63_FLASH 0", "-f", "target/ws63.cfg", "-c", "riscv ws53 on; shutdown"], 1),
]
# Legacy files explicitly call init/halt. Stub both before loading them so these
# checks cannot connect to or stop hardware, even when a probe is plugged in.
for entry in ("WS63-jlink.cfg", "WS63-swd.cfg"):
    cases.append((entry, ["-f", "interface/jlink.cfg", "-c",
        "proc init {} {}; proc halt {} {}", "-f", "target/vendorhm/" + entry,
        "-c", 'if {$WS63_FLASH != 0} {error "unexpected Flash enable"}; shutdown'], 0))

def check(name, search, options, expected):
    with tempfile.TemporaryDirectory() as cwd:
        result = subprocess.run([binary, "-s", str(search), *options], cwd=cwd,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=10)
    if result.returncode != expected or "PANIC" in result.stdout:
        raise SystemExit(f"{name}: rc={result.returncode}\n{result.stdout}")
    print(f"{name}: PASS")

for name, options, expected in cases:
    check(name, scripts, options, expected)
if args.package:
    for entry in ("WS63-jlink.cfg", "WS63-swd.cfg"):
        check("flat-" + entry, args.package.resolve(), ["-f", "jlink.cfg", "-c",
            "proc init {} {}; proc halt {} {}", "-f", entry, "-c",
            'if {$WS63_FLASH != 0} {error "unexpected Flash enable"}; shutdown'], 0)
    check("flat-ws53", args.package.resolve(), ["-f", "jlink.cfg", "-f", "ws53.cfg", "-c", ws53_targets("WS53")], 0)
