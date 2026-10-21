#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""No-flash WS53 debug regression against a dedicated OpenOCD Tcl server.

Temporarily borrows 32 bytes at ITCM 0x20000 and 32 bytes below the DTCM
stack. Saves RAM/registers to a new output directory before any target write.
Do not run while another debugger uses this target. Failures leave it halted.
"""
import argparse
import json
from pathlib import Path
import socket
import struct


class Tcl:
    def __init__(self, port):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=30)

    def call(self, command):
        self.socket.sendall(('set rc [catch {' + command +
            '} result]; format "%d %s" $rc $result').encode() + b'\x1a')
        reply = bytearray()
        while b'\x1a' not in reply:
            data = self.socket.recv(65536)
            if not data:
                raise RuntimeError('OpenOCD disconnected')
            reply.extend(data)
        status, _, result = reply.split(b'\x1a')[0].decode().partition(' ')
        if status != '0':
            raise RuntimeError(command + ': ' + result)
        return result

    def regs(self, names):
        items = self.call('WS53.cpu get_reg -force {' + ' '.join(names) + '}').split()
        return {items[i]: int(items[i+1], 0) for i in range(0, len(items), 2)}

    def put(self, values):
        self.call('WS53.cpu set_reg {' + ' '.join(f'{k} {v:#x}' for k, v in values.items()) + '}')

    def read(self, address, width, length):
        values = self.call(f'WS53.cpu read_memory {address:#x} {width} {length // (width // 8)}')
        return b''.join(int(v, 0).to_bytes(width // 8, 'little') for v in values.split())

    def write(self, address, width, data):
        step = width // 8
        values = ' '.join(str(int.from_bytes(data[i:i+step], 'little')) for i in range(0, len(data), step))
        self.call(f'WS53.cpu write_memory {address:#x} {width} {{{values}}}')


def check(c, output):
    passed = []
    def ok(name):
        passed.append(name)
        print(name + ': PASS', flush=True)
        (output/'results.json').write_text(json.dumps({'passed': passed}, indent=2)+'\n')
    if c.call('target current') != 'WS53.cpu':
        raise RuntimeError('Select WS53.cpu first')
    if c.call('bp').strip() or c.call('wp').strip():
        raise RuntimeError('Remove existing breakpoints/watchpoints first')
    c.call('halt')
    names = ['zero', 'ra', 'sp', 'gp', 'tp', 't0', 't1', 't2', 'fp', 's1']
    names += [f'a{i}' for i in range(8)] + [f's{i}' for i in range(2, 12)]
    names += ['t3', 't4', 't5', 't6', 'pc', 'mstatus', 'dcsr', 'ft0']
    before = c.regs(names)
    data_addr = (before['sp'] - 128) & ~3
    code_addr = 0x20000
    if not 0x20010000 <= data_addr < data_addr + 32 <= 0x20060000:
        raise RuntimeError('Expected a WS53 DTCM stack')
    saved_data = c.read(data_addr, 8, 32)
    saved_code = c.read(code_addr, 8, 32)
    (output/'recovery.json').write_text(json.dumps({
        'registers': before, 'data_address': data_addr, 'data': saved_data.hex(),
        'code_address': code_addr, 'code': saved_code.hex()}, indent=2)+'\n')
    hardware = []
    watch = False
    software = False
    success = False
    try:
        for width in (8, 16, 32):
            payload = bytes((i * 17 + width) & 255 for i in range(32))
            c.write(data_addr, width, payload)
            for read_width in (8, 16, 32):
                assert c.read(data_addr, read_width, 32) == payload
        c.write(data_addr + 1, 8, b'\x91\x23\x45\x67\x89')
        assert c.read(data_addr + 1, 8, 5) == b'\x91\x23\x45\x67\x89'
        assert c.regs(names) == before, 'memory access changed registers'
        ok('8/16/32-bit RAM access, unaligned byte access, register preservation')
        changes = {'t0': 0x12345678, 't1': 0x87654321, 'a0': 0x10203040,
                   'a1': 0x55667788, 'fp': 0x11223344, 's1': 0xaabbccdd}
        c.put(changes)
        assert c.regs(list(changes)) == changes
        c.put({k: before[k] for k in changes})
        c.put({'mstatus': before['mstatus'] | 0x2000, 'ft0': 0x3f800000})
        assert c.regs(['ft0'])['ft0'] == 0x3f800000
        c.put({'ft0': before['ft0'], 'mstatus': before['mstatus']})
        assert c.regs(names) == before
        ok('GPR/FPR/CSR write-read-restore')
        # addi a0,zero,123; sw a0,0(a1); lw a2,0(a1); addi a0,a0,1; jal zero,0
        code = struct.pack('<5I', 0x07b00513, 0x00a5a023, 0x0005a603, 0x00150513, 0x0000006f)
        c.write(code_addr, 32, code)
        assert c.read(code_addr, 32, len(code)) == code
        c.put({'mstatus': before['mstatus'] & ~8, 'pc': code_addr, 'a1': data_addr})
        c.call('step')
        hit = c.regs(['pc', 'a0', 'dcsr'])
        assert hit['pc'] == code_addr + 4 and hit['a0'] == 123 and (hit['dcsr'] >> 6) & 7 == 4, hit
        ok('deterministic instruction step in backed-up ITCM')
        c.call(f'bp {code_addr + 12:#x} 4 hw'); hardware.append(code_addr + 12)
        c.call('resume'); c.call('wait_halt 2000')
        hit = c.regs(['pc', 'a2', 'dcsr'])
        assert hit['pc'] == code_addr + 12 and hit['a2'] == 123 and (hit['dcsr'] >> 6) & 7 == 2, hit
        c.call('step')
        assert c.regs(['a0'])['a0'] == 124
        c.call(f'rbp {hardware.pop():#x}')
        ok('hardware breakpoint hit/removal and continue/step')
        for kind, allowed in [('w', (4, 8)), ('r', (8, 12)), ('a', (4, 8))]:
            c.put({'pc': code_addr, 'a1': data_addr})
            c.call(f'wp {data_addr:#x} 4 {kind}'); watch = True
            c.call(f'resume {code_addr:#x}'); c.call('wait_halt 2000')
            hit = c.regs(['pc', 'dcsr'])
            assert hit['pc'] - code_addr in allowed and (hit['dcsr'] >> 6) & 7 == 2, hit
            c.call(f'rwp {data_addr:#x}'); watch = False
            ok('watchpoint ' + kind + ' actual hit/removal')
        for i in range(8):
            location = code_addr + 4 * i
            c.call(f'bp {location:#x} 4 hw'); hardware.append(location)
        try:
            c.call(f'bp {code_addr + 32:#x} 4 hw')
        except RuntimeError:
            pass
        else:
            hardware.append(code_addr + 32)
            raise AssertionError('ninth hardware breakpoint unexpectedly accepted')
        for location in hardware:
            c.call(f'rbp {location:#x}')
        hardware.clear()
        ok('eight hardware trigger slots and clean exhaustion')
        # GDB forces hardware breakpoints; additionally check explicit RAM SWBP.
        c.put({'pc': code_addr, 'a1': data_addr})
        c.call(f'bp {code_addr:#x} 4'); software = True
        assert c.read(code_addr, 32, 4) == bytes.fromhex('73001000')
        c.call(f'resume {code_addr:#x}'); c.call('wait_halt 2000')
        hit = c.regs(['pc', 'dcsr'])
        assert hit['pc'] == code_addr and (hit['dcsr'] >> 6) & 7 == 1, hit
        c.call('step')
        assert c.regs(['pc', 'a0']) == {'pc': code_addr + 4, 'a0': 123}
        assert c.read(code_addr, 32, 4) == bytes.fromhex('73001000')
        c.call(f'rbp {code_addr:#x}'); software = False
        assert c.read(code_addr, 32, len(code)) == code
        c.put({'pc': code_addr})
        c.call('step')
        assert c.regs(['pc', 'a0']) == {'pc': code_addr + 4, 'a0': 123}
        ok('explicit ITCM software breakpoint hit and instruction restoration')
        success = True
    finally:
        c.call('halt')
        if watch:
            c.call(f'rwp {data_addr:#x}')
        for location in hardware:
            c.call(f'rbp {location:#x}')
        if software:
            c.call(f'rbp {code_addr:#x}')
        c.write(code_addr, 32, saved_code)
        c.write(data_addr, 32, saved_data)
        # x0 and dcsr.cause are read-only; restore only the writable state.
        c.put({k: v for k, v in before.items() if k != 'zero'})
        assert c.read(code_addr, 32, 32) == saved_code
        assert c.read(data_addr, 32, 32) == saved_data
        after = c.regs(names)
        expected = dict(before)
        after['dcsr'] &= ~0x1c0; expected['dcsr'] &= ~0x1c0
        assert after == expected, (after, expected)
        ok('original ITCM/DTCM/register state restored')
        if success:
            c.call('resume')
            assert c.call('WS53.cpu curstate') == 'running'
            ok('application resumed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=6675)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    client = Tcl(args.port)
    try:
        check(client, args.output)
    finally:
        client.socket.close()
