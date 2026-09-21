#!/usr/bin/env python3
"""Validate and summarize Slicks real-mode interrupt boundary events."""

import argparse
import csv
from collections import Counter
from pathlib import Path

from summarize_execution_trace import RUNTIME_SIZE


FIELDS = [
    "sequence", "vector", "type", "source_cs", "resume_ip", "target_cs",
    "target_ip", "source_offset", "target_offset", "ax", "bx", "cx", "dx",
    "si", "di", "bp", "sp", "flags",
]


def load_interrupts(path: Path) -> list[dict[str, int]]:
    events = []
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != FIELDS:
            raise SystemExit(f"{path}: unexpected CSV header {reader.fieldnames}")
        for line_number, row in enumerate(reader, start=2):
            try:
                event = {
                    field: int(value, 0) for field, value in row.items()
                }
            except (TypeError, ValueError) as error:
                raise SystemExit(f"{path}:{line_number}: malformed event") from error
            if event["sequence"] != len(events):
                raise SystemExit(
                    f"{path}:{line_number}: expected sequence {len(events)}"
                )
            for field in ("source_offset", "target_offset"):
                offset = event[field]
                if offset != -1 and not 0 <= offset < RUNTIME_SIZE:
                    raise SystemExit(
                        f"{path}:{line_number}: {field} outside runtime: {offset}"
                    )
            events.append(event)
    if not events:
        raise SystemExit(f"{path}: no interrupt events")
    return events


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("interrupts", type=Path)
    args = parser.parse_args()
    events = load_interrupts(args.interrupts)

    vector_types = Counter(
        (event["vector"], event["type"]) for event in events
    )
    boundaries = Counter(
        (event["source_offset"] >= 0, event["target_offset"] >= 0)
        for event in events
    )
    targets = Counter(
        (event["vector"], event["target_offset"])
        for event in events if event["target_offset"] >= 0
    )

    print(f"interrupt events: {len(events)}")
    for (vector, event_type), count in sorted(vector_types.items()):
        print(f"vector 0x{vector:02x} type 0x{event_type:02x}: {count}")
    print(f"runtime -> external: {boundaries[(True, False)]}")
    print(f"runtime -> runtime: {boundaries[(True, True)]}")
    print(f"external -> runtime: {boundaries[(False, True)]}")
    for (vector, offset), count in sorted(targets.items()):
        print(f"vector 0x{vector:02x} runtime target 0x{offset:05x}: {count}")
    services = Counter()
    for event in events:
        if not event["type"] & 1:
            continue
        function = event["ax"] if event["vector"] == 0x33 else event["ax"] >> 8
        services[(event["vector"], function)] += 1
    for (vector, function), count in sorted(services.items()):
        width = 4 if vector == 0x33 else 2
        register = "AX" if vector == 0x33 else "AH"
        print(
            f"software vector 0x{vector:02x} {register}=0x{function:0{width}x}: "
            f"{count}"
        )


if __name__ == "__main__":
    main()
