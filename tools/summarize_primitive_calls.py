#!/usr/bin/env python3
"""Validate and summarize traced calls to native graphics candidates."""

import argparse
import csv
from collections import Counter
from pathlib import Path


TARGETS = {
    0x2B40A: (
        "vga_plot",
        ("x", "y", "value", "screen_base"),
    ),
    0x2B8DE: (
        "vga_planar_subrect_blit",
        (
            "dest_x", "dest_y", "source_x", "source_y", "width", "height",
            "source_offset", "source_segment", "screen_base",
        ),
    ),
}


def number(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()

    rows_by_target: dict[int, int] = Counter()
    calls_by_target: dict[int, int] = Counter()
    values: dict[int, list[set[int]]] = {}
    strides: dict[int, Counter[int]] = {}
    source_sizes: Counter[tuple[int, int]] = Counter()
    violations: Counter[str] = Counter()
    violation_calls: Counter[str] = Counter()

    with args.trace.open(newline="") as source:
        reader = csv.DictReader(source)
        required = {
            "target", "name", *(f"arg{i}" for i in range(9)), "count",
            "stride", "source_width", "source_height",
        }
        missing = required.difference(reader.fieldnames or ())
        if missing:
            raise SystemExit(f"missing columns: {', '.join(sorted(missing))}")
        for row in reader:
            target = number(row["target"])
            if target not in TARGETS:
                raise SystemExit(f"unknown primitive target 0x{target:05x}")
            expected_name, arg_names = TARGETS[target]
            if row["name"] != expected_name:
                raise SystemExit(
                    f"target 0x{target:05x}: expected {expected_name}, got {row['name']}"
                )
            count = number(row["count"])
            if count <= 0:
                raise SystemExit(f"target 0x{target:05x}: non-positive count")
            arguments = [number(row[f"arg{i}"]) for i in range(len(arg_names))]
            for index in range(len(arg_names), 9):
                if row[f"arg{index}"]:
                    raise SystemExit(
                        f"target 0x{target:05x}: unexpected arg{index} value"
                    )

            rows_by_target[target] += 1
            calls_by_target[target] += count
            if target not in values:
                values[target] = [set() for _ in arg_names]
                strides[target] = Counter()
            for seen, value in zip(values[target], arguments):
                seen.add(value)

            stride = number(row["stride"])
            strides[target][stride] += count

            def reject(reason: str) -> None:
                violations[reason] += 1
                violation_calls[reason] += count

            if not stride:
                reject("zero framebuffer stride")

            if target == 0x2B40A:
                x, y, _, screen_base = arguments
                if screen_base + y * stride + (x >> 2) >= 0x10000:
                    reject("plot destination outside 64 KiB plane")

            if target == 0x2B8DE:
                (dest_x, dest_y, source_x, source_y, width, height,
                 _, _, screen_base) = arguments
                source_width = number(row["source_width"])
                source_height = number(row["source_height"])
                source_sizes[(source_width, source_height)] += count
                width_bytes = (width + 3) >> 2
                if not width or not height:
                    reject("zero-sized blit")
                if (source_x >> 2) + width_bytes > source_width:
                    reject("blit source x range outside sprite")
                if source_y + height > source_height:
                    reject("blit source y range outside sprite")
                final_byte = (
                    screen_base
                    + (dest_y + source_y + height - 1) * stride
                    + ((dest_x + source_x) >> 2)
                    + width_bytes
                    - 1
                )
                if final_byte >= 0x10000:
                    reject("blit destination outside 64 KiB plane")

    missing_targets = set(TARGETS).difference(rows_by_target)
    if missing_targets:
        rendered = ", ".join(f"0x{target:05x}" for target in sorted(missing_targets))
        raise SystemExit(f"missing primitive targets: {rendered}")

    for target in sorted(rows_by_target):
        name, arg_names = TARGETS[target]
        print(
            f"0x{target:05x} {name}: {calls_by_target[target]} calls, "
            f"{rows_by_target[target]} distinct tuples"
        )
        for name, seen in zip(arg_names, values[target]):
            print(
                f"  {name:<14} {min(seen):5d}..{max(seen):5d} "
                f"({len(seen)} distinct)"
            )
        if strides[target]:
            rendered = ", ".join(
                f"{stride} ({count} calls)"
                for stride, count in strides[target].most_common()
            )
            print(f"  stride         {rendered}")

    if source_sizes:
        print("sprite byte-width x height values:")
        for dimensions, count in source_sizes.most_common():
            print(f"  {dimensions[0]} x {dimensions[1]}: {count} calls")

    if violations:
        for reason in sorted(violations):
            print(
                f"ERROR: {reason}: {violations[reason]} tuples, "
                f"{violation_calls[reason]} calls"
            )
        raise SystemExit(1)
    print("primitive argument and bounds checks: passed")


if __name__ == "__main__":
    main()
