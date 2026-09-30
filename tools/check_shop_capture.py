#!/usr/bin/env python3
"""Check retained shop display and actual BMP files from the native shortcut."""
import pathlib
import struct
import sys

root = pathlib.Path(sys.argv[1])
data = pathlib.Path(sys.argv[2])
failed = len(sys.argv) == 4 and sys.argv[3] == "--failed"
frames = sorted(root.glob("*-before.planar"))
assert len(frames) == (1 if failed else 2), len(frames)
for n, frame in enumerate(frames):
    prefix = frame.name.removesuffix("-before.planar")
    for suffix, length in (("planar", 64000), ("palette", 768)):
        before = (root / f"{prefix}-before.{suffix}").read_bytes()
        assert len(before) == length
        assert before == (root / f"{prefix}-after.{suffix}").read_bytes()
    if failed:
        continue
    pixels = (root / f"{prefix}.chunky").read_bytes()
    palette = (root / f"{prefix}.palette").read_bytes()
    bmp = (data / f"TUNING{n:02}.BMP").read_bytes()
    assert len(pixels) == 64000 and len(palette) == 768
    assert len(bmp) == 65078 and bmp[:2] == b"BM"
    assert struct.unpack_from("<I", bmp, 10)[0] == 1078
    assert struct.unpack_from("<ii", bmp, 18) == (320, 200)
    assert struct.unpack_from("<HH", bmp, 26) == (1, 8)
    assert bmp[1078:] == b"".join(pixels[y*320:(y+1)*320] for y in range(199, -1, -1))
    assert bmp[54:1078] == b"".join(bytes((palette[i+2]<<2, palette[i+1]<<2, palette[i]<<2, 0)) for i in range(0, 768, 3))
print(f"{len(frames)} capture I/O windows retain pixels/palettes; " +
      ("failed write" if failed else "native BMP pixels/palettes match"))
