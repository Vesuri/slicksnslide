#!/usr/bin/env python3
"""Verify that each complete menu transition retained its outgoing display."""
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
frames = sorted(root.glob("*-before.planar"))
assert len(frames) == 4, ("Expected two opens and two closes", len(frames))
for frame in frames:
    stem = frame.name.removesuffix("-before.planar")
    for kind, size in (("planar", 64000), ("palette", 768)):
        before = (root / f"{stem}-before.{kind}").read_bytes()
        assert len(before) == size
        for phase in ("held", "after"):
            assert before == (root / f"{stem}-{phase}.{kind}").read_bytes(), (stem, kind, phase)
print("4 menu transitions: outgoing pixels and palettes retained unchanged")
