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


def prepare_race_palette(palette: bytearray) -> None:
    """Apply the palette rewrites performed by the original race setup."""
    car_ramps = (
        ((32, 0, 0), (63, 45, 0)),
        ((20, 8, 45), (40, 48, 60)),
        ((20, 8, 45), (40, 48, 60)),
        ((20, 8, 45), (40, 48, 60)),
    )
    for car, (first, last) in enumerate(car_ramps):
        for shade in range(5):
            index = 1 + car * 5 + shade
            for component in range(3):
                palette[index * 3 + component] = (
                    first[component]
                    + (last[component] - first[component]) * shade // 4
                )

    palette[183 * 3 : 183 * 3 + 3] = bytes((39, 43, 10))
    yellow_ramp = (
        (63, 61, 1),
        (63, 59, 11),
        (63, 57, 21),
        (63, 55, 31),
        (63, 53, 41),
    )
    for shade, colour in enumerate(yellow_ramp):
        start = (199 + shade) * 3
        palette[start : start + 3] = bytes(colour)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("logical", type=Path)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--chunky", action="store_true",
                        help="input is the native 320x200 chunky surface")
    parser.add_argument("--palette", type=Path,
                        help="use a target-dumped 768-byte six-bit palette")
    parser.add_argument("--title-palette", action="store_true",
                        help="use the original partII title palette")
    parser.add_argument(
        "--race-palette",
        action="store_true",
        help="apply the DOS race-time palette rewrites",
    )
    args = parser.parse_args()

    logical = args.logical.read_bytes()
    expected_size = 64000 if args.chunky else 0x40000
    if len(logical) != expected_size:
        raise ValueError(f"expected {expected_size} screen bytes, got {len(logical)}")
    palette_data = bytearray(
        args.palette.read_bytes() if args.palette else
        archive_resource(args.archive.read_bytes(), b"partII" if args.title_palette else b"peli.@p")
    )
    if len(palette_data) != 768:
        raise ValueError("peli.@p does not contain a 256-colour palette")
    if args.race_palette:
        prepare_race_palette(palette_data)
    palette = [((value << 2) | (value >> 4)) for value in palette_data]

    pixels = bytearray(logical) if args.chunky else bytearray(320 * 200)
    if not args.chunky:
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
