#!/usr/bin/env python3
"""Generate ignored negative .SSS fixtures from the native menu's real save.

This never supplies race state to the emulator. The native Load menu reads
the intentionally damaged files and must reject them without publishing.
"""
import argparse
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'amiga/.run/championship-v1/dh1/E2E.SSS'
DEST = ROOT / 'amiga/.run/championship-failure-v1/dh1'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('kind', choices=['truncated', 'track', 'profile', 'index', 'recovery', 'check', 'check-resume'])
    args = parser.parse_args()
    if args.kind == 'check':
        root = DEST.parent
        for field in ['session', 'playlist']:
            assert (root / (field + '.before')).read_bytes() == (root / (field + '.after')).read_bytes(), field
        print('Rejected native load preserved session and playlist byte-for-byte')
        return
    data = bytearray(SOURCE.read_bytes())
    assert data[:2] == b'S\x08'
    count = struct.unpack_from('>H', data, 2)[0]
    if args.kind == 'check-resume':
        at = 6 + 8 * count
        expected = {field: bytearray() for field in ['points', 'cash', 'inventory', 'vehicles']}
        for _ in range(4):
            for _ in range(20):
                character = data[at]
                at += 1
                if not character:
                    break
            vehicle = data[at]
            expected['vehicles'].append(vehicle if vehicle < 10 else 0)
            expected['points'].extend(data[at + 3:at + 5])
            expected['cash'].extend(data[at + 5:at + 7])
            expected['inventory'].extend(data[at + 7:at + 33])
            at += 33
        assert at == len(data)
        for field, value in expected.items():
            assert (SOURCE.parent.parent / (field + '.after')).read_bytes() == value, field
        print('Native resumed points/cash/inventory/vehicles match the actual .SSS bytes')
        return
    if args.kind == 'truncated':
        data = data[:-1]
    elif args.kind == 'track':
        data[4:12] = b'MISSING!'
    elif args.kind == 'profile':
        start = 6 + 8 * count
        end = start
        while end - start < 20 and data[end]:
            end += 1
        data[start:end] = b'Z' * (end - start)
        if end - start < 20:
            end += 1
        data[end + 1] = 255  # human with a nonexistent profile name
    elif args.kind == 'index':
        struct.pack_into('>H', data, 4 + 8 * count, count)
    DEST.mkdir(parents=True, exist_ok=True)
    (DEST / 'FAIL.SSS').write_bytes(data)
    recovery = DEST / 'FAIL.SSS.new'
    if args.kind == 'recovery':
        recovery.write_bytes(b'preserve this recovery fixture')
    elif recovery.exists():
        recovery.unlink()  # Only this tool's named, ignored fixture.
    print(f'Prepared {args.kind} fixture: {DEST / "FAIL.SSS"}')


if __name__ == '__main__':
    main()
