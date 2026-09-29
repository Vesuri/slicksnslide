"""Summarize emulated-time title traces; never interpret host run time as FPS."""
import argparse
from collections import Counter
from pathlib import Path
import re
import statistics

parser = argparse.ArgumentParser()
parser.add_argument("log", type=Path)
args = parser.parse_args()
text = args.log.read_text()
pc = re.findall(r"SLICKS_TITLE_TIMING sample=(\d+) ms=([\d.]+) counter=(\d+) phase=(\d+) mode=(\d+) queued=(\d+)", text)
if pc:
    if len(pc) != 128 or [int(r[0]) for r in pc] != list(range(128)):
        raise SystemExit("Expected 128 consecutive DOS title samples")
    if any(int(b[2]) != (int(a[2])+4)&255 for a, b in zip(pc, pc[1:])):
        raise SystemExit("DOS pulse counter did not advance by four")
    if len({r[4:] for r in map(tuple, pc)}) != 1:
        raise SystemExit("Mode/publication path changed during capture")
    times = [float(r[1]) for r in pc]
    # First entry draws before title-entry setup/delay; don't call that steady state.
    intervals = [b-a for a, b in zip(times[1:], times[2:])]
    print(f"DOS mode={pc[0][4]} queued={pc[0][5]}: {len(intervals)} steady intervals; "
          f"min/median/max={min(intervals):.3f}/{statistics.median(intervals):.3f}/{max(intervals):.3f} ms; "
          f"mean={statistics.mean(intervals):.3f} ms ({1000/statistics.mean(intervals):.2f} updates/s)")
    print(f"Initial setup interval: {times[1]-times[0]:.3f} ms")
else:
    native = re.findall(r"TITLE_TIMING_INTERVAL (\d+) (\d+)", text)
    if "TITLE_TIMING_OK updates=64" not in text or [int(r[0]) for r in native] != list(range(1, 65)):
        raise SystemExit("Expected a completed 64-interval native timing capture")
    ticks = [int(r[1]) for r in native]
    if min(ticks) < 1:
        raise SystemExit("Native interval did not cross a refresh")
    print(f"Amiga PAL: {len(ticks)} intervals, {sum(ticks)} refreshes; "
          f"refresh histogram={dict(sorted(Counter(ticks).items()))}; "
          f"mean={20*statistics.mean(ticks):.3f} ms ({50/statistics.mean(ticks):.2f} updates/s)")
