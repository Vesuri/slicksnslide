#!/usr/bin/env python3
"""Reverse captured DOS relocations and package the runtime as a normal MZ."""

import argparse
import csv
import json
import struct
from pathlib import Path


def word(data: bytearray, offset: int) -> int:
    return data[offset] | data[offset + 1] << 8


def put_word(data: bytearray, offset: int, value: int) -> None:
    data[offset : offset + 2] = value.to_bytes(2, "little")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime", type=Path)
    parser.add_argument("state", type=Path)
    parser.add_argument("relocations", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    loaded = bytearray(args.runtime.read_bytes())
    state = json.loads(args.state.read_text())
    registers = state["registers"]
    load_segment = registers["cs"]
    if registers["ip"] != 0:
        raise SystemExit("unexpected nonzero captured entry IP")

    with args.relocations.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    if len(rows) != state["relocation_count"]:
        raise SystemExit("relocation CSV and capture state disagree")

    normalized = bytearray(loaded)
    offsets: list[int] = []
    for row in rows:
        offset = int(row["linear_offset"], 0)
        if offset + 2 > len(normalized):
            raise SystemExit(f"relocation outside image: {offset:#x}")
        offsets.append(offset)
        put_word(normalized, offset, (word(normalized, offset) - load_segment) & 0xFFFF)

    # Reapply the captured load segment as an internal round-trip proof.
    verified = bytearray(normalized)
    for offset in offsets:
        put_word(verified, offset, (word(verified, offset) + load_segment) & 0xFFFF)
    if verified != loaded:
        raise SystemExit("relocation normalization did not round-trip")

    relocation_table_offset = 0x1C
    header_bytes = relocation_table_offset + 4 * len(offsets)
    header_size = (header_bytes + 15) & ~15
    output_size = header_size + len(normalized)
    pages = (output_size + 511) // 512
    last_page_bytes = output_size % 512
    header = bytearray(header_size)
    header[0:2] = b"MZ"
    fields = (
        last_page_bytes,
        pages,
        len(offsets),
        header_size // 16,
        0,
        0xFFFF,
        (registers["ss"] - load_segment) & 0xFFFF,
        registers["sp"],
        0,
        registers["ip"],
        0,
        relocation_table_offset,
        0,
    )
    struct.pack_into("<13H", header, 2, *fields)
    for index, offset in enumerate(offsets):
        # Canonical segment:offset representation for the linear image offset.
        struct.pack_into("<HH", header, relocation_table_offset + 4 * index,
                         offset & 0xF, offset >> 4)

    args.output.write_bytes(header + normalized)
    print(
        f"wrote {args.output}: {len(normalized)} image bytes, "
        f"{len(offsets)} relocations, {header_size}-byte header"
    )


if __name__ == "__main__":
    main()
