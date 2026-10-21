#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise WS53 reset/boot without downloading or erasing Flash.

Requires a dedicated, initialized OpenOCD Tcl server. Resets discard application
RAM state. No other debugger may be connected. Failure does not release the CPU.
"""
import argparse
import json
from pathlib import Path
from check_debug import Tcl


def check(c, cycles, boot_wait_ms, results):
    if c.call('target current') != 'WS53.cpu':
        raise RuntimeError('Select WS53.cpu first')
    if c.call('bp').strip() or c.call('wp').strip():
        raise RuntimeError('Remove existing breakpoints/watchpoints first')
    c.call('halt')
    previous = None
    for _ in range(2):
        c.call('reset halt')
        assert c.call('WS53.cpu curstate') == 'halted'
        assert c.regs(['pc'])['pc'] == 0x10000
        status = int.from_bytes(c.read(0x5200057c, 32, 4), 'little')
        active = (status >> 12) & 3
        assert active in (1, 2), hex(status)
        channel = 0x52000578 if active == 1 else 0x5200057c
        config = int.from_bytes(c.read(channel, 32, 4), 'little')
        assert config & 0x1ff == 1, hex(config)
        if previous is not None:
            assert active != previous, 'inactive clock channel was not selected'
        previous = active
        c.call('step')
        assert c.regs(['pc'])['pc'] == 0x10004
        results.append({'reset_halt_xo_channel': active - 1, 'step_pc': '0x10004'})

    c.call('bp 0x10080 4 hw')
    try:
        c.call('reset run')
        c.call('wait_halt 2000')
        assert c.regs(['pc'])['pc'] == 0x10080
        results.append({'reset_breakpoint_rearm': 'PASS'})
    finally:
        c.call('rbp 0x10080')

    for cycle in range(cycles):
        # First reset starts halted; subsequent resets start from running code.
        c.call('reset run')
        c.call(f'sleep {boot_wait_ms}')
        assert c.call('WS53.cpu curstate') == 'running'
        c.call('halt')
        pc = c.regs(['pc'])['pc']
        instruction = c.read(pc, 8, 4).hex()
        c.call('step')
        stepped = c.regs(['pc'])['pc']
        c.call('resume')
        c.call('sleep 100')
        assert c.call('WS53.cpu curstate') == 'running'
        results.append({'cycle': cycle + 1, 'boot_pc': hex(pc),
                        'instruction': instruction, 'step_pc': hex(stepped)})
        print(f'reset run cycle {cycle + 1}: PASS (PC {pc:#x} -> {stepped:#x})', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=6666)
    parser.add_argument('--cycles', type=int, default=3)
    parser.add_argument('--boot-wait-ms', type=int, default=5000)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.cycles < 1 or args.boot_wait_ms < 1:
        parser.error('cycles and boot wait must be positive')
    args.output.mkdir(exist_ok=False)
    report = {'passed': False, 'results': []}
    client = None
    try:
        client = Tcl(args.port)
        check(client, args.cycles, args.boot_wait_ms, report['results'])
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        if client is not None:
            client.socket.close()
        (args.output/'results.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
