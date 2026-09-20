#!/usr/bin/env python3
"""Summarize the bounded DOSBox-X reference log into stable evidence."""

import argparse
import collections
import re
from pathlib import Path


INT21_RE = re.compile(r"Executing interrupt 21, ah=([0-9a-f]+)", re.I)
FILE_NAME_RE = re.compile(
    r"\b([A-Z0-9_~.-]+\.(?:000|BAT|CFG|COM|DAT|EXE|OUT|PLR|REK|SS|TRK)|CON)\b",
    re.I,
)
VIDEO_RE = re.compile(r"(?:INT10:)?Set Video Mode (.+)")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    args = parser.parse_args()

    lines = args.log.read_text(errors="replace").splitlines()
    interrupts: collections.Counter[str] = collections.Counter()
    file_events: collections.Counter[tuple[str, str]] = collections.Counter()
    video_modes: list[str] = []
    cpu_evidence: list[str] = []

    for line in lines:
        lower = line.lower()
        if match := INT21_RE.search(line):
            interrupts[match.group(1).upper().zfill(2)] += 1
        if "FILES:" in line and (match := FILE_NAME_RE.search(line)):
            action = next(
                (label for label, stem in (("open", "open"), ("create", "creat"),
                                           ("read", "read"), ("write", "writ"),
                                           ("seek", "seek"), ("close", "clos"))
                 if stem in lower),
                "other",
            )
            file_events[(action, match.group(1).upper())] += 1
        if match := VIDEO_RE.search(line):
            mode = match.group(1).strip()
            if not video_modes or video_modes[-1] != mode:
                video_modes.append(mode)
        if "cpu type" in lower or "cputype" in lower or re.search(r"\b286\b", line):
            evidence = line.removeprefix("LOG: ").strip()
            if evidence not in cpu_evidence:
                cpu_evidence.append(evidence)

    print(f"log lines: {len(lines)}")
    print("CPU evidence:")
    for line in cpu_evidence[:12]:
        print(f"  {line}")
    print("video modes:")
    for mode in video_modes:
        print(f"  {mode}")
    print("INT 21h functions:")
    for function, count in interrupts.most_common():
        print(f"  AH={function}: {count}")
    print("file events:")
    for (action, detail), count in file_events.most_common():
        print(f"  {action:8} {count:5}  {detail}")


if __name__ == "__main__":
    main()
