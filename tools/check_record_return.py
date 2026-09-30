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

# The separate I/O fixture captures the displayed image, which can be either
# the race or its records/recovery overlay. Never accept a missing after dump.
windows = sorted(root.glob("io-*-before.planar"))
for before_path in windows:
    stem = before_path.name.removesuffix("-before.planar")
    for extension, size in (("planar", 64000), ("palette", 768)):
        before = (root / f"{stem}-before.{extension}").read_bytes()
        after = (root / f"{stem}-after.{extension}").read_bytes()
        if len(before) != size or before != after:
            raise SystemExit(f"{stem}: displayed {extension} changed during disk I/O")
if windows:
    print(f"{len(windows)} disk I/O windows preserve every displayed pixel and palette byte")
