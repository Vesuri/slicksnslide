#!/usr/bin/env python3
"""Verify records/recovery preserves both the race source and its bitmap."""
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
for race in range(2):
    entry = root / f"record-{race}-entry"
    close = root / f"record-{race}-close"
    for extension in (".chunky", ".planar"):
        before = entry.with_suffix(extension).read_bytes()
        after = close.with_suffix(extension).read_bytes()
        if len(before) != 64000 or before != after:
            raise SystemExit(f"Race {race}: {extension} changed across records owner")
    chunky = entry.with_suffix(".chunky").read_bytes()
    planar = entry.with_suffix(".planar").read_bytes()
    for y in range(200):
        for x in range(320):
            value = sum(((planar[y*320 + p*40 + x//8] >> (7-x%8)) & 1) << p
                        for p in range(8))
            if value != chunky[y*320+x]:
                raise SystemExit(f"Race {race}: pixel ({x},{y}) differs at owner entry")
print("Two record returns preserve all 64000 source pixels and all eight race bitplanes")
