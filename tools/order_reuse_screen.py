#!/usr/bin/env python3
"""Screen exact actor-chain reuse from read-only GDB captures, not timings.

Unreachable next/previous bytes are deliberately ignored: they are stale
scratch storage, not members. Validate both traversal directions independently.
This measures an upper bound, not the cost or correctness of invalidation.
"""
import argparse
import json
from pathlib import Path
import struct

RECORD_SIZE = 4 + 128 + 200 + 1 + 128 + 200


def decode(data):
    if not data or len(data) % RECORD_SIZE:
        raise ValueError("empty or truncated order capture")
    records = []
    for offset in range(0, len(data), RECORD_SIZE):
        raw = data[offset:offset + RECORD_SIZE]
        frame, = struct.unpack_from(">I", raw)
        heads, following = raw[4:132], raw[132:332]
        maximum, tails, previous = raw[332], raw[333:461], raw[461:661]
        seen = set()
        chains = []
        for priority, head in enumerate(heads):
            chain = []
            handle = head
            while handle:
                if handle >= 200 or handle in seen:
                    raise ValueError("invalid, cyclic or duplicate forward handle")
                seen.add(handle)
                chain.append(handle)
                handle = following[handle]
            if chain != sorted(chain):
                raise ValueError("unstable equal-priority order")
            reverse = []
            handle = tails[priority]
            while handle:
                if handle >= 200 or handle in reverse:
                    raise ValueError("invalid or cyclic inverse handle")
                reverse.append(handle)
                handle = previous[handle]
            if reverse != chain[::-1]:
                raise ValueError("inverse chain disagrees")
            chains.append(tuple(chain))
        expected_max = max((p for p, chain in enumerate(chains) if chain), default=0)
        if maximum != expected_max:
            raise ValueError("incorrect maximum priority")
        if records and frame != records[-1][0] + 1:
            raise ValueError("non-consecutive frames")
        records.append((frame, tuple(chains), len(seen)))
    return records


def summarize(records):
    eligible = []
    examined = records[1:]
    changed_handles = []
    busy_changes = []
    for before, after in zip(records, examined):
        if before[1] == after[1]:
            eligible.append(after)
        old = {handle: p for p, chain in enumerate(before[1]) for handle in chain}
        new = {handle: p for p, chain in enumerate(after[1]) for handle in chain}
        changes = sum(old.get(handle) != new.get(handle) for handle in old.keys() | new.keys())
        changed_handles.append(changes)
        if after[2] >= 100:
            busy_changes.append(changes)
    visits = sum(row[2] for row in examined)
    saved = sum(row[2] for row in eligible)
    return {"transitions": len(examined), "unchanged": len(eligible),
            "linked_handles": visits, "reusable_linked_handles": saved,
            "unchanged_percent": 100 * len(eligible) / max(1, len(examined)),
            "reusable_handle_percent": 100 * saved / max(1, visits),
            "busy_transitions": sum(row[2] >= 100 for row in examined),
            "busy_unchanged": sum(row[2] >= 100 for row in eligible),
            "changed_handles_mean": sum(changed_handles) / max(1, len(changed_handles)),
            "changed_handles_max": max(changed_handles, default=0),
            "busy_changed_handles_mean": sum(busy_changes) / max(1, len(busy_changes))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", nargs="+", type=Path)
    parser.add_argument("--probe-frame", action="append", type=int, default=[])
    args = parser.parse_args()
    for path in args.captures:
        records = decode(path.read_bytes())
        if len(records) != 603 or records[0][0] != 98 or records[-1][0] != 700:
            raise ValueError("expected complete race updates 98..700")
        print(json.dumps({"capture": str(path), **summarize(records)}))
        for frame in args.probe_frame:
            pair = [row for row in records if row[0] in (frame - 1, frame)]
            if len(pair) != 2:
                raise ValueError("probe needs two captured consecutive frames")
            print(json.dumps({"capture": str(path), "frame": frame, **summarize(pair)}))


if __name__ == "__main__":
    main()
