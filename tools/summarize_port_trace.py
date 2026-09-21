#!/usr/bin/env python3
"""Validate and summarize aggregated Slicks x86 port-I/O events."""

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from summarize_execution_trace import RUNTIME_SIZE


FIELDS = ["direction", "width", "port", "value", "resume_offset", "count"]
PORT_NAMES = {
    0x0002: "DMA channel 1 address",
    0x0003: "DMA channel 1 count",
    0x000A: "DMA channel mask",
    0x000B: "DMA mode",
    0x000C: "DMA flip-flop reset",
    0x0020: "PIC command",
    0x0021: "PIC interrupt mask",
    0x0040: "PIT channel 0",
    0x0043: "PIT control",
    0x0060: "keyboard data",
    0x0061: "PC speaker/PPI",
    0x0083: "DMA channel 1 page",
    0x0201: "joystick",
    0x0226: "Sound Blaster DSP reset",
    0x022C: "Sound Blaster DSP write/status",
    0x03C0: "VGA attribute controller",
    0x03C4: "VGA sequencer index/data",
    0x03C5: "VGA sequencer data",
    0x03C8: "VGA DAC write index",
    0x03C9: "VGA DAC data",
    0x03CE: "VGA graphics index/data",
    0x03CF: "VGA graphics data",
    0x03D4: "VGA CRTC index/data",
    0x03D5: "VGA CRTC data",
    0x03DA: "VGA input status",
}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("ports", type=Path)
    args = parser.parse_args()

    totals = Counter()
    values = defaultdict(set)
    sites = defaultdict(set)
    rows = 0
    previous = None
    with args.ports.open(newline="") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != FIELDS:
            raise SystemExit(
                f"{args.ports}: unexpected CSV header {reader.fieldnames}"
            )
        for line_number, row in enumerate(reader, start=2):
            try:
                direction = row["direction"]
                width = int(row["width"], 0)
                port = int(row["port"], 0)
                value = int(row["value"], 0)
                offset = int(row["resume_offset"], 0)
                count = int(row["count"], 0)
            except (TypeError, ValueError) as error:
                raise SystemExit(
                    f"{args.ports}:{line_number}: malformed aggregate"
                ) from error
            if direction not in ("read", "write") or width not in (1, 2, 4):
                raise SystemExit(f"{args.ports}:{line_number}: invalid operation")
            if not 0 <= port <= 0xFFFF or not 0 <= offset < RUNTIME_SIZE:
                raise SystemExit(f"{args.ports}:{line_number}: value out of range")
            if not 0 <= value < 1 << (width * 8) or count <= 0:
                raise SystemExit(f"{args.ports}:{line_number}: invalid value/count")
            aggregate = (direction, width, port, value, offset)
            if previous is not None and aggregate <= previous:
                raise SystemExit(
                    f"{args.ports}:{line_number}: aggregates not strictly sorted"
                )
            previous = aggregate
            key = (direction, width, port)
            totals[key] += count
            values[key].add(value)
            sites[key].add(offset)
            rows += 1

    if not rows:
        raise SystemExit(f"{args.ports}: no port aggregates")
    print(f"port aggregate rows: {rows}")
    print(f"port events represented: {sum(totals.values())}")
    for (direction, width, port), count in sorted(totals.items()):
        name = PORT_NAMES.get(port, "unclassified")
        print(
            f"{direction:5} {width * 8:2}-bit 0x{port:04x}: {count} events, "
            f"{len(sites[(direction, width, port)])} sites, "
            f"{len(values[(direction, width, port)])} values — {name}"
        )


if __name__ == "__main__":
    main()
