#!/usr/bin/env python3
"""Render a target-dumped Slicks four-bank logical screen to a PNG."""

import argparse
from pathlib import Path

from PIL import Image


def archive_resource(archive: bytes, wanted: bytes) -> bytes:
    if archive[:3] != b"MF\x1a":
        raise ValueError("not a Slicks resource archive")
    count = int.from_bytes(archive[3:5], "big")
    entries = []
    for index in range(count):
        at = 5 + index * 19
        name = archive[at : at + 16].split(b"\0", 1)[0]
        offset = int.from_bytes(archive[at + 16 : at + 19], "big")
        entries.append((name, offset))
    for index, (name, start) in enumerate(entries[:-1]):
        if name == wanted:
            return archive[start : entries[index + 1][1]]
    raise ValueError(f"resource {wanted.decode()} not found")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("logical", type=Path)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    logical = args.logical.read_bytes()
    if len(logical) != 0x40000:
        raise ValueError(f"expected 262144 logical bytes, got {len(logical)}")
    palette_data = archive_resource(args.archive.read_bytes(), b"peli.@p")
    if len(palette_data) != 768:
        raise ValueError("peli.@p does not contain a 256-colour palette")
    palette = [((value << 2) | (value >> 4)) for value in palette_data]

    pixels = bytearray(320 * 200)
    for y in range(200):
        for x in range(320):
            pixels[y * 320 + x] = logical[
                (x & 3) * 0x10000 + y * 100 + (x >> 2)
            ]
    image = Image.frombytes("P", (320, 200), bytes(pixels))
    image.putpalette(palette)
    image.save(args.output)


if __name__ == "__main__":
    main()
