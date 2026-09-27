#!/usr/bin/env python3
"""Replay diag_dirty_publications.gdb; analysis only, never target timing."""
import argparse
import re
from collections import Counter
from pathlib import Path


def clipped(r):
    l, t, r, b = r
    l, t, r, b = max(0, l), max(0, t), min(320, r), min(200, b)
    if l >= r or t >= b:
        return None
    return l & ~15, t, (r + 15) & ~15, b


def union(a, b):
    return min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3])


def area(r):
    return (r[2] - r[0]) * (r[3] - r[1])


def publish(rows, r, limit, strict=False):
    if r is None:
        return
    while True:
        for i, e in enumerate(rows):
            if r[0] >= e[0] and r[1] >= e[1] and r[2] <= e[2] and r[3] <= e[3]:
                return
            gap = max(e[0] - r[2], r[0] - e[2], e[1] - r[3], r[1] - e[3])
            if gap >= 0 if strict else gap > 0:
                continue
            r = union(r, e)
            rows[i] = rows[-1]
            rows.pop()
            break
        else:
            break
    if len(rows) >= limit:
        for e in rows:
            r = union(r, e)
        rows.clear()
    rows.append(r)


def pixels(rectangles):
    # Offline evidence only; no runtime framebuffer shadow or row bitmap.
    return {(x, y) for l, t, r, b in rectangles for y in range(t, b) for x in range(l, r)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('log', nargs='?', type=Path)
    p.add_argument('--limit', type=int, default=16)
    p.add_argument('--self-test', action='store_true')
    args = p.parse_args()
    if args.self_test:
        rows = []
        publish(rows, (0, 0, 16, 2), 16)
        publish(rows, (16, 2, 32, 4), 16)
        assert rows == [(0, 0, 32, 4)]
        publish(rows, (4, 1, 8, 2), 16)
        assert rows == [(0, 0, 32, 4)]
        publish(rows, (64, 8, 80, 10), 1)
        assert rows == [(0, 0, 80, 10)]
        assert clipped((-2, -1, 17, 3)) == (0, 0, 32, 3)
        assert clipped((320, 0, 340, 1)) is None
        separate = [(0, 0, 16, 4)]
        publish(separate, (16, 2, 32, 4), 16, strict=True)
        assert len(separate) == 2
        print('SELF_TEST_OK')
    if not args.log:
        return
    text = args.log.read_text()
    if 'DIRTY_CAPTURE_OK' not in text:
        raise SystemExit('incomplete capture')
    initial, final, requests = [], [], []
    for line in text.splitlines():
        m = re.fullmatch(r'RECT phase=(\d) xy=([\d,]+)', line)
        if m:
            (initial if m[1] == '0' else final).append(tuple(map(int, m[2].split(','))))
        m = re.fullmatch(r'PUBLICATION caller=(0x[0-9a-f]+) xy=([-\d,]+)', line)
        if m:
            requests.append((m[1], tuple(map(int, m[2].split(',')))))
    rows = initial.copy()
    raw, aligned = initial.copy(), initial.copy()
    counts = Counter()
    for i, (caller, request) in enumerate(requests):
        r = clipped(request)
        if r:
            l, t, rr, b = request
            raw.append((max(0, l), max(0, t), min(320, rr), min(200, b)))
            aligned.append(r)
        before = sum(map(area, rows))
        publish(rows, r, args.limit)
        delta = sum(map(area, rows)) - before
        counts[caller] += 1
        if delta:
            print(f'GROW call={i} caller={caller} request={request} area_delta={delta} rows={rows}')
    if rows != final:
        raise SystemExit(f'replay mismatch: computed {rows}, captured {final}')
    print(f'REPLAY_OK calls={len(requests)} raw_union={len(pixels(raw))} '
          f'aligned_union={len(pixels(aligned))} converted={sum(map(area, rows))}')
    for caller, count in counts.most_common():
        print(f'CALLER {caller} calls={count}')
    strict = initial.copy()
    for _, request in requests:
        publish(strict, clipped(request), args.limit, strict=True)
    print(f'STRICT_OVERLAP converted={sum(map(area, strict))} rows={strict}')


if __name__ == '__main__':
    main()
