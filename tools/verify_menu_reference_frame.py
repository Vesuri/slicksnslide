#!/usr/bin/env python3
"""Compare a native indexed menu dump with a diagnostic DOS capture.

Read-only. Captures remain local test evidence, never production assets.
Compare six-bit DAC RGB, allowing duplicate palette indices. Accept only
integer-scaled 320x200 frames and check every pixel in every scaled block.
"""
import argparse
from pathlib import Path
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("chunky", type=Path)
    parser.add_argument("palette", type=Path)
    parser.add_argument("reference", type=Path)
    args = parser.parse_args()
    chunky = args.chunky.read_bytes()
    palette = args.palette.read_bytes()
    if len(chunky) != 64000 or len(palette) != 768 or max(palette) > 63:
        parser.error("Expected 64000 indices and a 768-byte six-bit palette")
    with Image.open(args.reference) as source:
        image = source.convert("RGB")
    sx, rx = divmod(image.width, 320)
    sy, ry = divmod(image.height, 200)
    if rx or ry or not sx or not sy:
        parser.error("Reference must be an integer-scaled 320x200 frame")
    colours = [tuple(palette[i:i+3]) for i in range(0, 768, 3)]
    pixels = image.load()
    mismatch = 0
    for y in range(200):
        for x in range(320):
            expected = colours[chunky[y*320+x]]
            if any(tuple(c >> 2 for c in pixels[x*sx+dx, y*sy+dy]) != expected
                   for dy in range(sy) for dx in range(sx)):
                if mismatch < 10:
                    print(f"Mismatch x={x} y={y}: native DAC RGB={expected}")
                mismatch += 1
    if mismatch:
        print(f"FAIL: {mismatch} native pixels differ from the DOS capture")
        return 1
    print("PASS: all 64000 native menu pixels match the original DOS capture in six-bit RGB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
