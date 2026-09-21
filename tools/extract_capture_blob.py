#!/usr/bin/env python3
"""Extract and verify one FNV-identified blob from a captured binary bundle."""

import argparse
import csv
from pathlib import Path


def fnv1a(data: bytes) -> int:
    value = 14695981039346656037
    for byte in data:
        value ^= byte
        value = (value * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return value


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    parser.add_argument("binary", type=Path)
    parser.add_argument("key")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    with args.index.open(newline="") as source:
        reader = csv.DictReader(source)
        if not reader.fieldnames:
            raise SystemExit("empty capture index")
        key_field = reader.fieldnames[0]
        matches = [row for row in reader if row[key_field] == args.key]
    if len(matches) != 1:
        raise SystemExit(f"expected one {args.key} record, found {len(matches)}")

    row = matches[0]
    offset = int(row["offset"], 0)
    size = int(row["size"], 0)
    data = args.binary.read_bytes()[offset:offset + size]
    if len(data) != size:
        raise SystemExit(f"capture bundle is truncated at {offset:#x}+{size:#x}")
    if fnv1a(data) != int(args.key, 16):
        raise SystemExit(f"capture blob {args.key} failed its FNV-1a check")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)


if __name__ == "__main__":
    main()
