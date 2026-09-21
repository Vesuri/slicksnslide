#!/usr/bin/env python3
"""Validate VGA/runtime memory aggregates and detect executed-code writes."""

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

from summarize_execution_trace import RUNTIME_SIZE, load_bitmap


FIELDS = [
    "direction", "width", "region", "source_offset", "target_offset", "count",
]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("memory", type=Path)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("bitmap", type=Path)
    args = parser.parse_args()

    image = args.runtime.read_bytes()
    if len(image) != RUNTIME_SIZE:
        raise SystemExit(f"{args.runtime}: expected {RUNTIME_SIZE} bytes")
    executed = load_bitmap(args.bitmap)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    executed_bytes = bytearray(RUNTIME_SIZE)
    for offset in executed:
        instruction = next(
            decoder.disasm(image[offset:offset + 15], offset, count=1), None
        )
        if instruction is None:
            raise SystemExit(f"failed to decode executed start 0x{offset:05x}")
        executed_bytes[offset:offset + instruction.size] = bytes(
            [1] * instruction.size
        )

    totals = Counter()
    sites = defaultdict(set)
    pages = defaultdict(set)
    code_write_rows = 0
    code_write_events = 0
    rows = 0
    previous = None
    with args.memory.open(newline="") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != FIELDS:
            raise SystemExit(
                f"{args.memory}: unexpected CSV header {reader.fieldnames}"
            )
        for line_number, row in enumerate(reader, start=2):
            try:
                direction = row["direction"]
                width = int(row["width"], 0)
                region = row["region"]
                source_offset = int(row["source_offset"], 0)
                target_offset = int(row["target_offset"], 0)
                count = int(row["count"], 0)
            except (TypeError, ValueError) as error:
                raise SystemExit(
                    f"{args.memory}:{line_number}: malformed aggregate"
                ) from error
            if direction not in ("read", "write") or width not in (1, 2, 4):
                raise SystemExit(f"{args.memory}:{line_number}: invalid operation")
            if region not in ("vga", "runtime") or count <= 0:
                raise SystemExit(f"{args.memory}:{line_number}: invalid region/count")
            if not 0 <= source_offset < RUNTIME_SIZE:
                raise SystemExit(f"{args.memory}:{line_number}: invalid source")
            if region == "vga":
                if target_offset & 0xFF or not 0 <= target_offset < 0x20000:
                    raise SystemExit(f"{args.memory}:{line_number}: invalid VGA page")
            elif not 0 <= target_offset <= RUNTIME_SIZE - width:
                raise SystemExit(f"{args.memory}:{line_number}: invalid runtime target")
            key_order = (
                direction == "write", width, region == "runtime",
                source_offset, target_offset,
            )
            if previous is not None and key_order <= previous:
                raise SystemExit(
                    f"{args.memory}:{line_number}: aggregates not strictly sorted"
                )
            previous = key_order
            key = (region, direction, width)
            totals[key] += count
            sites[key].add(source_offset)
            pages[(region, direction)].add(
                target_offset if region == "vga" else target_offset >> 8
            )
            if (
                region == "runtime" and direction == "write" and
                any(executed_bytes[target_offset:target_offset + width])
            ):
                code_write_rows += 1
                code_write_events += count
            rows += 1

    if not rows:
        raise SystemExit(f"{args.memory}: no memory aggregates")
    print(f"memory aggregate rows: {rows}")
    print(f"memory events represented: {sum(totals.values())}")
    for (region, direction, width), count in sorted(totals.items()):
        print(
            f"{region:7} {direction:5} {width * 8:2}-bit: {count} events, "
            f"{len(sites[(region, direction, width)])} source sites"
        )
    for region, direction in sorted(pages):
        print(
            f"{region} {direction} target 256-byte pages: "
            f"{len(pages[(region, direction)])}"
        )
    print(
        f"writes overlapping executed instruction bytes: "
        f"{code_write_rows} rows, {code_write_events} events"
    )
    if code_write_events:
        raise SystemExit("observed writes overlap executed instruction bytes")


if __name__ == "__main__":
    main()
