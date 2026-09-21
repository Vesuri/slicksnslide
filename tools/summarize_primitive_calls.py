#!/usr/bin/env python3
"""Validate and summarize traced calls to native graphics candidates."""

import argparse
import csv
from collections import Counter
from pathlib import Path


TARGETS = {
    0x00D9F: (
        "far_fill",
        ("destination_offset", "destination_segment", "count", "value_word"),
    ),
    0x24499: (
        "vga_remap_copy",
        (
            "x0", "y0", "x1", "y1", "table_offset", "table_segment",
            "screen_base",
        ),
    ),
    0x29E35: (
        "vga_span_fill",
        ("x0", "y0", "x1", "y1", "value_word", "screen_base"),
    ),
    0x2A97C: (
        "vga_transparent_blit",
        ("x", "y", "source_offset", "source_segment", "screen_base"),
    ),
    0x2A9F2: (
        "vga_planar_blit",
        ("x", "y", "source_offset", "source_segment", "screen_base"),
    ),
    0x2AAE5: (
        "vga_readback",
        (
            "x", "y", "width", "height_word", "destination_offset",
            "destination_segment", "screen_base",
        ),
    ),
    0x2AD92: ("vga_clear_full", ()),
    0x2ADB7: ("vga_mode_setup", ("mode", "virtual_width")),
    0x2B40A: (
        "vga_plot",
        ("x", "y", "value", "screen_base"),
    ),
    0x2B45E: (
        "vga_plot_plane",
        ("x", "y", "value", "screen_base"),
    ),
    0x2B48E: (
        "vga_read_pixel",
        ("x", "y", "screen_base"),
    ),
    0x2B8DE: (
        "vga_planar_subrect_blit",
        (
            "dest_x", "dest_y", "source_x", "source_y", "width", "height",
            "source_offset", "source_segment", "screen_base",
        ),
    ),
}

# This exported primitive is not reached by the bounded BASIC.SS race, but it
# remains instrumented so other modes can add coverage without rebuilding.
OPTIONAL_TARGETS = {0x29E35}
STRIDE_FREE_TARGETS = {0x00D9F, 0x2AD92, 0x2ADB7}


def number(value: str) -> int:
    return int(value, 0)


def signed_word(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()

    rows_by_target: dict[int, int] = Counter()
    calls_by_target: dict[int, int] = Counter()
    values: dict[int, list[set[int]]] = {}
    strides: dict[int, Counter[int]] = {}
    source_sizes: Counter[tuple[int, int, int]] = Counter()
    readback_sizes: Counter[tuple[int, int]] = Counter()
    fill_sizes: Counter[tuple[int, int]] = Counter()
    remap_sizes: Counter[tuple[int, int]] = Counter()
    far_fill_video_calls = 0
    rejected_fills = 0
    rejected_fill_calls = 0
    violations: Counter[str] = Counter()
    violation_calls: Counter[str] = Counter()
    wraps: Counter[str] = Counter()
    wrap_calls: Counter[str] = Counter()

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
            if target not in STRIDE_FREE_TARGETS:
                strides[target][stride] += count

            def reject(reason: str) -> None:
                violations[reason] += 1
                violation_calls[reason] += count

            def note_wrap(reason: str) -> None:
                wraps[reason] += 1
                wrap_calls[reason] += count

            if target not in STRIDE_FREE_TARGETS and not stride:
                reject("zero framebuffer stride")

            if target == 0x00D9F:
                destination_offset, destination_segment, byte_count, _ = arguments
                if destination_offset + byte_count > 0x10000:
                    reject("far fill destination wraps 16-bit offset")
                if destination_segment in (0xA000, 0xB800):
                    far_fill_video_calls += count

            if target == 0x24499:
                (x0_word, y0_word, x1_word, y1_word, table_offset, _,
                 screen_base) = arguments
                x0 = signed_word(x0_word)
                y0 = signed_word(y0_word)
                x1 = signed_word(x1_word)
                y1 = signed_word(y1_word)
                if table_offset + 0x100 > 0x10000:
                    reject("remap table wraps 16-bit far offset")
                if x1 > x0 and y1 > y0:
                    remap_sizes[(x1 - x0, y1 - y0)] += count
                    first_byte = screen_base + y0 * stride + (x0 // 4)
                    final_byte = (
                        screen_base + (y1 - 1) * stride + ((x1 - 1) // 4)
                    )
                    if first_byte < 0 or final_byte >= 0x10000:
                        reject("remap destination outside 64 KiB plane")

            if target == 0x29E35:
                x0_word, y0_word, x1_word, y1_word, _, screen_base = arguments
                x0 = signed_word(x0_word)
                y0 = signed_word(y0_word)
                x1 = signed_word(x1_word)
                y1 = signed_word(y1_word)
                if x1 <= x0 or y1 <= y0:
                    rejected_fills += 1
                    rejected_fill_calls += count
                else:
                    fill_sizes[(x1 - x0, y1 - y0)] += count
                    first_byte = screen_base + y0 * stride + (x0 >> 2)
                    final_byte = (
                        screen_base + (y1 - 1) * stride + ((x1 - 1) >> 2)
                    )
                    if first_byte < 0 or final_byte >= 0x10000:
                        reject("fill destination outside 64 KiB plane")

            if target in (0x2B40A, 0x2B45E):
                x, y, _, screen_base = arguments
                if screen_base + y * stride + (x >> 2) >= 0x10000:
                    reject("plot destination outside 64 KiB plane")

            if target == 0x2B48E:
                x, y, screen_base = arguments
                if screen_base + y * stride + (x >> 2) >= 0x10000:
                    reject("pixel read outside 64 KiB plane")

            if target in (0x2A97C, 0x2A9F2):
                x, y, source_offset, _, screen_base = arguments
                source_width = number(row["source_width"])
                source_height = number(row["source_height"])
                source_sizes[(target, source_width, source_height)] += count
                if not source_width or not source_height:
                    reject("zero-sized sprite")
                elif (
                    screen_base
                    + (y + source_height - 1) * stride
                    + (x >> 2)
                    + source_width
                    > 0x10000
                ):
                    note_wrap("sprite destination wraps 16-bit VGA offset")
                if source_offset + 2 + 4 * source_width * source_height > 0x10000:
                    reject("sprite source wraps 16-bit far offset")

            if target == 0x2AAE5:
                (x, y, width, height_word, destination_offset,
                 _, screen_base) = arguments
                height = height_word & 0xFF
                width_bytes = (width + 3) >> 2
                readback_sizes[(width, height)] += count
                if not width or not height:
                    reject("zero-sized readback")
                elif (
                    screen_base
                    + (y + height - 1) * stride
                    + (x >> 2)
                    + width_bytes
                    > 0x10000
                ):
                    note_wrap("readback source wraps 16-bit VGA offset")
                output_size = 3 + 4 * width_bytes * height
                if destination_offset + output_size > 0x10000:
                    reject("readback destination wraps 16-bit far offset")

            if target == 0x2B8DE:
                (dest_x, dest_y, source_x, source_y, width, height,
                 _, _, screen_base) = arguments
                source_width = number(row["source_width"])
                source_height = number(row["source_height"])
                source_sizes[(target, source_width, source_height)] += count
                width_bytes = (width + 3) >> 2
                if not width or not height:
                    reject("zero-sized blit")
                if (source_x >> 2) + width_bytes > source_width:
                    reject("blit source x range outside sprite")
                if source_y + height > source_height:
                    reject("blit source y range outside sprite")
                if (
                    arguments[6] + 2 + 4 * source_width * source_height
                    > 0x10000
                ):
                    reject("blit source wraps 16-bit far offset")
                final_byte = (
                    screen_base
                    + (dest_y + source_y + height - 1) * stride
                    + ((dest_x + source_x) >> 2)
                    + width_bytes
                    - 1
                )
                if final_byte >= 0x10000:
                    reject("blit destination outside 64 KiB plane")

    missing_targets = set(TARGETS).difference(OPTIONAL_TARGETS, rows_by_target)
    if missing_targets:
        rendered = ", ".join(f"0x{target:05x}" for target in sorted(missing_targets))
        raise SystemExit(f"missing primitive targets: {rendered}")

    for target in sorted(OPTIONAL_TARGETS.difference(rows_by_target)):
        print(f"0x{target:05x} {TARGETS[target][0]}: not observed")

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
        ordered_sizes = source_sizes.most_common()
        for dimensions, count in ordered_sizes[:20]:
            print(
                f"  {TARGETS[dimensions[0]][0]}: "
                f"{dimensions[1]} x {dimensions[2]} ({count} calls)"
            )
        if len(ordered_sizes) > 20:
            print(f"  ... {len(ordered_sizes) - 20} additional size combinations")

    if readback_sizes:
        print("readback width x height values:")
        for dimensions, count in readback_sizes.most_common(20):
            print(f"  {dimensions[0]} x {dimensions[1]} ({count} calls)")
        if len(readback_sizes) > 20:
            print(f"  ... {len(readback_sizes) - 20} additional size combinations")

    if fill_sizes:
        print("fill width x height values:")
        for dimensions, count in fill_sizes.most_common(20):
            print(f"  {dimensions[0]} x {dimensions[1]} ({count} calls)")
        if len(fill_sizes) > 20:
            print(f"  ... {len(fill_sizes) - 20} additional size combinations")
    if rejected_fills:
        print(
            f"rejected empty fills: {rejected_fills} tuples, "
            f"{rejected_fill_calls} calls"
        )

    if remap_sizes:
        print("remap width x height values:")
        for dimensions, count in remap_sizes.most_common(20):
            print(f"  {dimensions[0]} x {dimensions[1]} ({count} calls)")
        if len(remap_sizes) > 20:
            print(f"  ... {len(remap_sizes) - 20} additional size combinations")
    if rows_by_target[0x00D9F]:
        print(
            "far fills targeting video segments A000h/B800h: "
            f"{far_fill_video_calls} calls"
        )

    for reason in sorted(wraps):
        print(
            f"NOTICE: {reason}: {wraps[reason]} tuples, "
            f"{wrap_calls[reason]} calls"
        )

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
