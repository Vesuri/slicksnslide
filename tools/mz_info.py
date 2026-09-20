#!/usr/bin/env python3
"""Inspect a DOS MZ executable without executing it."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


FIELD_NAMES = (
    "magic", "last_page_bytes", "pages", "relocations", "header_paragraphs",
    "min_alloc", "max_alloc", "ss", "sp", "checksum", "ip", "cs",
    "reloc_table_offset", "overlay",
)


def parse_mz(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) < 28:
        raise ValueError("file is shorter than the fixed MZ header")

    values = struct.unpack_from("<14H", data)
    header = dict(zip(FIELD_NAMES, values, strict=True))
    if header["magic"] != 0x5A4D:
        raise ValueError("not an MZ executable")

    pages = int(header["pages"])
    last = int(header["last_page_bytes"])
    declared_size = 0 if pages == 0 else (pages - 1) * 512 + (last or 512)
    header_size = int(header["header_paragraphs"]) * 16
    reloc_offset = int(header["reloc_table_offset"])
    reloc_count = int(header["relocations"])

    relocs = []
    for index in range(reloc_count):
        offset = reloc_offset + index * 4
        if offset + 4 > len(data):
            raise ValueError(f"relocation {index} lies outside the file")
        reloc_word, reloc_segment = struct.unpack_from("<HH", data, offset)
        relocs.append({"segment": reloc_segment, "offset": reloc_word})

    first_kib = data[header_size:header_size + 1024]
    compack = b"Copyright (c) 1991 W Collis" in first_kib

    return {
        "path": str(path),
        "sha256": hashlib.sha256(data).hexdigest(),
        "actual_size": len(data),
        "declared_size": declared_size,
        "header_size": header_size,
        "image_size": max(0, min(len(data), declared_size) - header_size),
        "entry": {"cs": header["cs"], "ip": header["ip"]},
        "stack": {"ss": header["ss"], "sp": header["sp"]},
        "allocation": {
            "minimum_paragraphs": header["min_alloc"],
            "maximum_paragraphs": header["max_alloc"],
        },
        "relocations": relocs,
        "overlay": header["overlay"],
        "compack_signature": compack,
        "header": header,
    }


def hex4(value: object) -> str:
    return f"{int(value):04X}"


def print_text(info: dict[str, object]) -> None:
    entry = info["entry"]
    stack = info["stack"]
    allocation = info["allocation"]
    assert isinstance(entry, dict)
    assert isinstance(stack, dict)
    assert isinstance(allocation, dict)

    print(f"file:             {info['path']}")
    print(f"sha256:           {info['sha256']}")
    print(f"size:             {info['actual_size']} bytes")
    print(f"declared size:    {info['declared_size']} bytes")
    print(f"header:           {info['header_size']} bytes")
    print(f"load image:       {info['image_size']} bytes")
    print(f"entry CS:IP:      {hex4(entry['cs'])}:{hex4(entry['ip'])}")
    print(f"stack SS:SP:      {hex4(stack['ss'])}:{hex4(stack['sp'])}")
    print(f"minimum alloc:    {hex4(allocation['minimum_paragraphs'])} paragraphs")
    print(f"maximum alloc:    {hex4(allocation['maximum_paragraphs'])} paragraphs")
    print(f"relocations:      {len(info['relocations'])}")
    print(f"overlay number:   {info['overlay']}")
    print(f"Compack signature:{' yes' if info['compack_signature'] else ' no'}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    info = parse_mz(args.executable)
    if args.json:
        print(json.dumps(info, indent=2, sort_keys=True))
    else:
        print_text(info)


if __name__ == "__main__":
    main()
