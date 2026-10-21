#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check WS53 AP views on a dedicated Tcl connection, without Flash writes.

Temporarily borrows 32 bytes below the A-core DTCM stack and restores them.
Temporarily activates an inactive AP2 DM to check data0, without halting its
hart, writing COMMAND, or enabling autoexec. Requires no other debugger.
"""
import argparse
import json
from pathlib import Path
from check_debug import Tcl


def check(c, output, passed):
    def read(target, address, width=32, count=1):
        values = c.call(f'{target} read_memory {address:#x} {width} {count}')
        return b''.join(int(v, 0).to_bytes(width // 8, 'little') for v in values.split())

    def write(target, address, width, data):
        step = width // 8
        values = ' '.join(str(int.from_bytes(data[i:i+step], 'little')) for i in range(0, len(data), step))
        c.call(f'{target} write_memory {address:#x} {width} {{{values}}}')

    def word(target, address):
        return int.from_bytes(read(target, address), 'little')

    def put_word(target, address, value):
        write(target, address, 32, value.to_bytes(4, 'little'))

    def ok(message):
        passed.append(message)
        print(message + ': PASS', flush=True)

    assert c.call('target current') == 'WS53.cpu'
    assert {'WS53.cpu', 'WS53.ap0', 'WS53.ap1', 'WS53.ap2'} <= set(c.call('target names').split())
    if c.call('bp').strip() or c.call('wp').strip():
        raise RuntimeError('Remove breakpoints/watchpoints first')
    running = c.call('WS53.cpu curstate') == 'running'
    c.call('halt')
    before = c.regs(['pc', 'sp', 't0', 't1', 'fp', 's1'])
    address = (before['sp'] - 128) & ~3
    if not 0x20010000 <= address < address + 32 <= 0x20060000:
        raise RuntimeError('Expected a WS53 A-core DTCM stack')
    saved = read('WS53.cpu', address, 8, 32)
    assert read('WS53.ap0', address, 8, 32) == saved
    (output/'recovery.json').write_text(json.dumps({'address': address, 'data': saved.hex()}, indent=2)+'\n')
    success = False
    try:
        assert read('WS53.ap0', 0x10000, 32, 4) == read('WS53.cpu', 0x10000, 32, 4)
        for width in (8, 16, 32):
            data = bytes((i * 17 + width) & 255 for i in range(32))
            write('WS53.ap0', address, width, data)
            for other in (8, 16, 32):
                assert read('WS53.ap0', address, other, 32 // (other // 8)) == data
            assert read('WS53.cpu', address, 8, 32) == data
        write('WS53.ap0', address + 1, 8, b'\x91\x23\x45\x67\x89')
        assert read('WS53.cpu', address + 1, 8, 5) == b'\x91\x23\x45\x67\x89'
        assert c.regs(before.keys()) == before
        ok('AP0 ROM and 8/16/32-bit RAM access agree with A-core view')
        assert word('WS53.ap1', 0x44) & 0x300 == 0x300
        assert word('WS53.ap1', 0x58) == 0x03000001
        ok('AP1 raw DM view matches halted CPU and abstract capabilities')

        # Refuse to disturb an AP2 DM already owned by another debugger.
        if word('WS53.ap2', 0x40) != 0:
            raise RuntimeError('AP2 DM is already active/in use')
        status = word('WS53.ap2', 0x44)
        assert status & 0xf == 2
        assert word('WS53.ap2', 0x58) == 0x03000001
        try:
            put_word('WS53.ap2', 0x40, 1)
            assert word('WS53.ap2', 0x60) == 0, 'AP2 autoexec enabled'
            assert not word('WS53.ap2', 0x58) & 0x1000, 'AP2 abstract command busy'
            data0 = read('WS53.ap2', 0x10)
            try:
                for value in (0x12345678, 0x87654321):
                    put_word('WS53.ap2', 0x10, value)
                    assert word('WS53.ap2', 0x10) == value
            finally:
                write('WS53.ap2', 0x10, 32, data0)
            assert word('WS53.ap2', 0x44) & 0xf00 == status & 0xf00
        finally:
            put_word('WS53.ap2', 0x40, 0)
        assert word('WS53.ap2', 0x40) == 0
        assert c.call('target current') == 'WS53.cpu'
        ok('AP2 data0 round-trip without hart control; inactive DM state restored')
        success = True
    finally:
        write('WS53.ap0', address, 8, saved)
        assert read('WS53.cpu', address, 8, 32) == saved
        assert c.regs(before.keys()) == before
        if success and running:
            c.call('resume')
        ok('Borrowed DTCM and A-core registers restored')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=6666)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    result = {'passed': False, 'checks': []}
    client = None
    try:
        client = Tcl(args.port)
        check(client, args.output, result['checks'])
        result['passed'] = True
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        if client is not None:
            client.socket.close()
        (args.output/'results.json').write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
