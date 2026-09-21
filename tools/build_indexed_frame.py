#!/usr/bin/env python3
"""Convert a 320x200 RGB capture into Slicks' four-bank sprite layout."""

from pathlib import Path
import sys


WIDTH = 320
HEIGHT = 200
PLANE_WIDTH = WIDTH // 4


def expand_vga(value: int) -> int:
    return (value << 2) | (value >> 4)


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: build_indexed_frame.py frame.rgb palette.bin output.bin",
              file=sys.stderr)
        return 2

    rgb = Path(sys.argv[1]).read_bytes()
    palette = Path(sys.argv[2]).read_bytes()
    if len(rgb) != WIDTH * HEIGHT * 3:
        raise SystemExit(f"expected {WIDTH * HEIGHT * 3} RGB bytes, got {len(rgb)}")
    if len(palette) != 768:
        raise SystemExit(f"expected 768 palette bytes, got {len(palette)}")

    color_to_index: dict[tuple[int, int, int], int] = {}
    for index in range(256):
        offset = index * 3
        color = tuple(expand_vga(component)
                      for component in palette[offset:offset + 3])
        color_to_index.setdefault(color, index)

    planes = [bytearray(PLANE_WIDTH * HEIGHT) for _ in range(4)]
    for pixel in range(WIDTH * HEIGHT):
        source = pixel * 3
        color = tuple(rgb[source:source + 3])
        if color not in color_to_index:
            raise SystemExit(f"pixel {pixel} color {color} is absent from palette")
        x = pixel % WIDTH
        y = pixel // WIDTH
        planes[x & 3][y * PLANE_WIDTH + (x >> 2)] = color_to_index[color]

    output = bytes((PLANE_WIDTH, HEIGHT)) + b"".join(planes)
    Path(sys.argv[3]).write_bytes(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
