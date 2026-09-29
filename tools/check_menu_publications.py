#!/usr/bin/env python3
"""Independent full-surface check of interleaved native menu publications."""
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
frames = sorted(root.glob("*.chunky"), key=lambda p: int(p.stem))
if not frames:
    raise SystemExit("No menu publications captured")
for frame in frames:
    chunky = frame.read_bytes()
    planar = frame.with_suffix(".planar").read_bytes()
    assert len(chunky) == len(planar) == 64000
    for y in range(200):
        for x in range(320):
            value = sum(((planar[y*320 + plane*40 + x//8] >> (7-x%8)) & 1) << plane
                        for plane in range(8))
            if value != chunky[y*320+x]:
                raise SystemExit(f"{frame.stem}: pixel ({x},{y}) planar={value} chunky={chunky[y*320+x]}")
print(f"{len(frames)} menu publications: all 64000 pixels match each chunky surface")
