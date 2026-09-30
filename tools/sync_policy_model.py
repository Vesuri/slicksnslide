#!/usr/bin/env python3
"""Replay benchmark WORK_SAMPLE sequences under publication policies.

Usage: tools/sync_policy_model.py tmp/LABEL-{0,1,2,3}.log
Models each update as finishing WORK lines after the previous publication.
hard: publish at the first display-end edge at or after completion (current).
adaptive: publish at the next edge if completion precedes it, else at once
(late publication; single-buffered, so a torn frame in dirty regions).
The hard model is checked against the measured cadence_lines of the log.
"""
import re, sys
F = 312
for log in sys.argv[1:]:
    text = open(log).read()
    w = [int(x) for x in re.findall(r'^WORK_SAMPLE index=\d+ lines=(\d+)', text, re.M)]
    meas = int(re.search(r'cadence_lines=(\d+)', text).group(1))
    def run(adaptive):
        t = 0; pubs = []; late = 0
        for x in w:
            fin = t + x; nxt = (t // F + 1) * F
            if fin <= nxt: p = nxt
            elif adaptive: p = fin; late += 1
            else: p = -(-fin // F) * F
            pubs.append(p); t = p
        iv = [b - a for a, b in zip(pubs, pubs[1:])]
        return sum(iv), late, max(iv)
    n = len(w) - 1
    fps = lambda s: F * n * 50 / s
    h, a = run(False), run(True)
    print(f"{log}: measured {fps(meas):.1f} fps | model hard {fps(h[0]):.1f} fps"
          f" | adaptive {fps(a[0]):.1f} fps, late {a[1]}/{len(w)}, worst {a[2]*20/F:.1f} ms")
