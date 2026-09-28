#!/usr/bin/env python3
"""Count point/nonpoint transitions, not timings or exact fast-path calls.

Nonpoint runs can contain general actors and packet failures. Dirty overflow
can split point runs. Thus run counts are lower bounds for dispatch entries,
not a promise that every nonpoint run is handled by the sprite fast path.
"""
import argparse
import json
import re
from pathlib import Path
import struct
from order_reuse_screen import decode as decode_order, RECORD_SIZE as ORDER_SIZE

RECORD_SIZE = ORDER_SIZE + 400


def dispatch_counts(text, records):
    found = {}
    for frame, point, sprite, general in re.findall(
            r'^DRAW_DISPATCH frame=(\d+) point=(\d+) sprite=(\d+) general=(\d+)$',
            text, re.M):
        frame = int(frame)
        if frame in found:
            raise ValueError("duplicate dispatch frame")
        found[frame] = dict(point_calls=int(point), sprite_calls=int(sprite),
                            general_calls=int(general))
    if set(found) != {r['frame'] for r in records}:
        raise ValueError("dispatch and chain capture windows differ")
    return found


def decode(data):
    if not data or len(data) % RECORD_SIZE:
        raise ValueError("empty or truncated chain capture")
    orders = decode_order(b"".join(data[i:i+ORDER_SIZE]
                         for i in range(0, len(data), RECORD_SIZE)))
    result = []
    for i, (frame, chains, count) in enumerate(orders):
        indices = struct.unpack_from(">200h", data, i * RECORD_SIZE + ORDER_SIZE)
        runs = transitions = points = nonpoints = occupied = 0
        for chain in chains:
            if not chain:
                continue
            occupied += 1
            previous = None
            for handle in chain:
                index = indices[handle]
                if index >= 256:
                    raise ValueError("point index exceeds pool")
                point = index >= 0
                points += point
                nonpoints += not point
                if previous is None or point != previous:
                    runs += 1
                    transitions += previous is not None
                previous = point
        assert points + nonpoints == count
        result.append(dict(frame=frame, handles=count, points=points,
                           nonpoints=nonpoints, runs=runs,
                           transitions=transitions, occupied=occupied))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--probe-frame", type=int, action="append", default=[])
    parser.add_argument("--dispatch", type=Path)
    args = parser.parse_args()
    rows = decode(args.capture.read_bytes())
    if len(rows) != 603 or rows[0]["frame"] != 98 or rows[-1]["frame"] != 700:
        raise ValueError("expected updates 98..700")
    keys = ["handles", "points", "nonpoints", "runs", "transitions", "occupied"]
    if args.dispatch:
        calls = dispatch_counts(args.dispatch.read_text(), rows)
        for row in rows:
            row.update(calls[row['frame']])
        keys += ['point_calls', 'sprite_calls', 'general_calls']
    print(json.dumps({"frames": len(rows), **{
        key: {"mean": sum(r[key] for r in rows)/len(rows),
              "max": max(r[key] for r in rows)}
        for key in keys}}))
    for row in rows:
        if row["frame"] in args.probe_frame:
            print(json.dumps(row))


if __name__ == "__main__":
    main()
