#!/usr/bin/env python3
"""Render captured Slicks four-plane byte sprites as a diagnostic contact sheet."""

import argparse
import csv
from pathlib import Path

from PIL import Image, ImageDraw


def diagnostic_color(index: int) -> tuple[int, int, int]:
    if index == 0:
        return (0, 0, 0)
    return (
        ((index >> 5) & 7) * 255 // 7,
        ((index >> 2) & 7) * 255 // 7,
        (index & 3) * 85,
    )


def decode_sprite(blob: bytes,
                  palette: list[tuple[int, int, int]] | None) -> Image.Image:
    width_bytes, height = blob[:2]
    pixels = Image.new("RGB", (width_bytes * 4, height))
    plane_size = width_bytes * height
    for plane in range(4):
        start = 2 + plane * plane_size
        for y in range(height):
            row = start + y * width_bytes
            for byte_x in range(width_bytes):
                index = blob[row + byte_x]
                color = diagnostic_color(index) if palette is None else palette[index]
                pixels.putpixel((byte_x * 4 + plane, y), color)
    return pixels


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("hashes", nargs="+")
    parser.add_argument("--scale", type=int, default=8)
    parser.add_argument("--palette-index", type=Path)
    parser.add_argument("--palette-binary", type=Path)
    parser.add_argument("--palette-hash")
    args = parser.parse_args()

    wanted = set(args.hashes)
    records: list[dict[str, str]] = []
    with args.index.open(newline="") as source:
        for row in csv.DictReader(source):
            if row["source_hash"] in wanted:
                records.append(row)
    missing = wanted.difference(row["source_hash"] for row in records)
    if missing:
        raise SystemExit(f"missing sprite hashes: {', '.join(sorted(missing))}")

    if any((args.palette_index, args.palette_binary, args.palette_hash)) and not all(
        (args.palette_index, args.palette_binary, args.palette_hash)
    ):
        parser.error("palette index, binary, and hash must be supplied together")
    palette = None
    if args.palette_index:
        with args.palette_index.open(newline="") as source:
            palette_rows = {
                row["palette_hash"]: row for row in csv.DictReader(source)
            }
        row = palette_rows.get(args.palette_hash)
        if not row:
            raise SystemExit(f"missing palette hash: {args.palette_hash}")
        palette_data = args.palette_binary.read_bytes()
        offset = int(row["offset"], 0)
        raw = palette_data[offset:offset + int(row["size"], 0)]
        palette = [
            tuple(component * 255 // 63 for component in raw[i:i + 3])
            for i in range(0, len(raw), 3)
        ]

    data = args.binary.read_bytes()
    rendered: list[tuple[dict[str, str], Image.Image]] = []
    for row in records:
        offset = int(row["offset"], 0)
        size = int(row["size"], 0)
        blob = data[offset:offset + size]
        if len(blob) != size:
            raise SystemExit(f"truncated blob {row['source_hash']}")
        rendered.append((row, decode_sprite(blob, palette)))

    margin = 12
    label_height = 28
    widths = [image.width * args.scale + margin * 2 for _, image in rendered]
    heights = [image.height * args.scale + label_height + margin * 2
               for _, image in rendered]
    sheet = Image.new("RGB", (max(widths), sum(heights)), (32, 32, 32))
    draw = ImageDraw.Draw(sheet)
    top = 0
    for (row, sprite), tile_height in zip(rendered, heights):
        scaled = sprite.resize(
            (sprite.width * args.scale, sprite.height * args.scale),
            Image.Resampling.NEAREST,
        )
        sheet.paste(scaled, (margin, top + label_height + margin))
        draw.text(
            (margin, top + margin),
            f"{row['source_hash']}  {sprite.width}x{sprite.height}",
            fill=(255, 255, 255),
        )
        top += tile_height
    sheet.save(args.output)


if __name__ == "__main__":
    main()
