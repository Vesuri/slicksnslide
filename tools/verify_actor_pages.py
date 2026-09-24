#!/usr/bin/env python3
"""Verify live actor page cadence, excluding the capped race trace counter."""
import argparse
from collections import Counter
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("log", type=Path)
parser.add_argument("--allow-final-incomplete", action="store_true",
                    help="report and exclude a final PRE cut off by the timed emulator shutdown")
args = parser.parse_args()
pattern = re.compile(r"SLICKS_ACTOR_PAGE stage=(PRE|POST) frame=(\d+) page=(\d+) ds=([0-9a-fA-F]+)(?: ticks=(\d+))?")
pending = None
previous_page = None
previous_frame = None
pending_ticks = None
tick_counts = Counter()
counts = Counter()
total = 0
for line in args.log.read_text().splitlines():
    if "SLICKS_ACTOR_PAGE" not in line:
        continue
    match = pattern.search(line)
    if not match:
        raise SystemExit(f"Malformed page record: {line}")
    stage, frame, page, ds, ticks = match.groups()
    frame, page = int(frame), int(page)
    if page not in (0, 1):
        raise SystemExit(f"Invalid page: {line}")
    if stage == "PRE":
        if pending is not None:
            raise SystemExit("Unpaired PRE")
        if previous_page is not None and page != previous_page:
            raise SystemExit(f"Page changed between actor calls at frame {frame}")
        pending = frame, page, ds
        pending_ticks = None if ticks is None else int(ticks)
    else:
        if pending != (frame, 1 - page, ds):
            raise SystemExit(f"Missing pair or page did not toggle: {line}")
        pending = None
        previous_page = page
        total += 1
        # Existing integrator tracing stops collecting at 16384 samples.
        # Do not mistake its saturated frame counter for extra actor calls.
        if 1 <= frame <= 4000:
            counts[frame] += 1
            if pending_ticks is not None and previous_frame is not None:
                if not 1 <= pending_ticks <= 45 or frame - previous_frame != pending_ticks:
                    raise SystemExit(f"Elapsed-tick mismatch at frame {frame}: previous={previous_frame}, ticks={pending_ticks}")
                tick_counts[pending_ticks] += 1
        previous_frame = frame
if pending is not None and args.allow_final_incomplete:
    print(f"Excluded final incomplete actor call at frame {pending[0]}; no claim about its page toggle")
elif pending is not None:
    raise SystemExit("Truncated gameplay capture")
if not counts:
    raise SystemExit("Truncated or empty gameplay capture")
if any(value != 1 for value in counts.values()):
    raise SystemExit(f"Multiple actor calls in simulation frames: {[(k,v) for k,v in counts.items() if v != 1][:10]}")
if tick_counts:
    print(f"DOS actor pages: {total} paired toggles; {sum(tick_counts.values())} elapsed-tick batches matched exactly; batch sizes {dict(sorted(tick_counts.items()))}")
elif sorted(counts) != list(range(min(counts), max(counts) + 1)):
    missing = sorted(set(range(min(counts), max(counts) + 1)) - counts.keys())
    raise SystemExit(f"One-call-per-step assumption failed: {total} continuous paired toggles, "
                     f"{len(counts)} actor calls over frames {min(counts)}..{max(counts)}, "
                     f"{len(missing)} steps without actor calls; first missing: {missing[:12]}")
else:
    print(f"DOS actor pages: {total} paired toggles; one actor call per simulation frame for {len(counts)} frames ({min(counts)}..{max(counts)})")
