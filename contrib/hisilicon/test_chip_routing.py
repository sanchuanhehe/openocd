#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check HiSpark CLI chip-name routing without opening hardware."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('openocd', type=Path)
args = parser.parse_args()
binary = str(args.openocd.resolve())
cases = [
    ('3322-jlink.cfg', '3322'),
    ('target/vendorhm/3322-jlink.cfg', '3322'),
    ('target\\vendorhm\\3322-jlink.cfg', '3322'),
    ('target/CHIP-MCU/3066H-swd.cfg', '3066H'),
    ('target/vendorhm/WS63-swd.cfg', 'WS63'),
    ('nested/NB18-ACORE-jtag.cfg', 'NB18-ACORE'),
    ('x' * 60 + '-jlink.cfg', None),
    ('ws53.cfg', None),
]
for filename, expected in cases:
    with tempfile.TemporaryDirectory() as directory:
        config = Path(directory) / filename
        config.parent.mkdir(parents=True, exist_ok=True)
        config.write_text('# Routing-only fixture; no target or adapter.\n')
        result = subprocess.run([binary, '-f', filename, '-c', 'shutdown'], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=10)
    found = [line.partition(': ')[2] for line in result.stdout.splitlines()
             if line.startswith('Extracted chiptype: ')]
    wanted = [] if expected is None else [expected]
    if result.returncode or found != wanted:
        raise SystemExit(f'{filename}: expected {wanted}, got {found}\n{result.stdout}')
    print(f'{filename}: PASS')
