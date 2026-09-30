#!/usr/bin/env python3
"""Check HOLDT step ticks against original 36ce0 with the title's argument 2:
first step at the press, then one whenever ticks > last+2, none after release.
Polling happens every vblank, so a step may land in the tick it becomes due
or (rarely) the next one; each interval must be exactly 3 ticks."""
import re, sys
text = open(sys.argv[1]).read()
m = re.search(r'HOLD start=(\d+) release=(\d+) steps=(\d+)', text)
start, release, count = map(int, m.groups())
steps = [int(x) for x in re.findall(r'^HOLD_STEP (\d+)', text, re.M)]
assert len(steps) == count and count >= 2, 'no repeats recorded'
assert steps[0] - start <= 1, 'first press not immediate'
gaps = [b - a for a, b in zip(steps, steps[1:])]
assert all(g == 3 for g in gaps), f'intervals {gaps}'
assert steps[-1] <= release, 'step after release'
assert release - steps[-1] <= 3, 'repeat stopped early'
print(f"PASS: {count} title Down dispatches over {release-start} BIOS ticks, every interval 3 ticks (argument 2), none after release")
