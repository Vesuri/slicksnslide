#!/usr/bin/env python3
"""Group traced VGA-memory accesses by instruction and enclosing function."""

import argparse
import csv
import re
from collections import defaultdict
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs


FUNCTION_RE = re.compile(
    r"; FUNC\s+(\S+)\s+([0-9a-fA-F]+):([0-9a-fA-F]+)"
    r"\s+-\s+([0-9a-fA-F]+):([0-9a-fA-F]+)"
)
LOAD_BASE = 0x10100


def physical(segment: str, offset: str) -> int:
    return int(segment, 16) * 16 + int(offset, 16)


def load_functions(path: Path | None) -> list[tuple[int, int, str]]:
    if path is None:
        return []
    functions = []
    for line in path.read_text().splitlines():
        match = FUNCTION_RE.match(line)
        if match:
            functions.append((
                physical(match[2], match[3]) - LOAD_BASE,
                physical(match[4], match[5]) - LOAD_BASE,
                match[1],
            ))
    return functions


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime", type=Path)
    parser.add_argument("memory", type=Path)
    parser.add_argument("--listing", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    image = args.runtime.read_bytes()
    functions = load_functions(args.listing)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    sites = defaultdict(lambda: {
        "events": 0, "directions": set(), "widths": set(), "pages": set(),
    })
    with args.memory.open(newline="") as source:
        for row in csv.DictReader(source):
            if row["region"] != "vga":
                continue
            offset = int(row["source_offset"], 0)
            site = sites[offset]
            site["events"] += int(row["count"], 0)
            site["directions"].add(row["direction"])
            site["widths"].add(int(row["width"], 0))
            site["pages"].add(int(row["target_offset"], 0))

    records = []
    for offset, site in sorted(sites.items()):
        instruction = next(
            decoder.disasm(image[offset:offset + 15], offset, count=1), None
        )
        if instruction is None:
            raise SystemExit(f"failed to decode VGA site 0x{offset:05x}")
        containing = [
            function for function in functions
            if function[0] <= offset <= function[1]
        ]
        if len(containing) > 1:
            raise SystemExit(f"overlapping functions at VGA site 0x{offset:05x}")
        function_start, function_name = ("", "")
        if containing:
            function_start = f"0x{containing[0][0]:05x}"
            function_name = containing[0][2]
        records.append({
            "offset": f"0x{offset:05x}",
            "physical": f"0x{LOAD_BASE + offset:05x}",
            "function_offset": function_start,
            "function_name": function_name,
            "directions": ";".join(sorted(site["directions"])),
            "widths": ";".join(str(width) for width in sorted(site["widths"])),
            "events": str(site["events"]),
            "vga_pages": str(len(site["pages"])),
            "mnemonic": instruction.mnemonic,
            "operands": instruction.op_str,
        })

    fieldnames = [
        "offset", "physical", "function_offset", "function_name", "directions",
        "widths", "events", "vga_pages", "mnemonic", "operands",
    ]
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("w", newline="") as output:
            writer = csv.DictWriter(output, fieldnames=fieldnames, lineterminator="\n")
            writer.writeheader()
            writer.writerows(records)
        print(f"wrote VGA site map: {args.output}")
    print(f"VGA access instructions: {len(records)}")
    print(f"enclosing functions: {len({row['function_offset'] for row in records})}")
    for row in sorted(records, key=lambda item: int(item["events"]), reverse=True):
        print(
            f"{row['offset']} {int(row['events']):>9} {row['directions']:<5} "
            f"{row['mnemonic']} {row['operands']} ({row['function_name']})"
        )


if __name__ == "__main__":
    main()
