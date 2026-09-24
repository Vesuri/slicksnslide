#!/usr/bin/env python3
"""Check bounded live DOS pool snapshots; never treat a partial pair as success."""
import re
import sys
from collections import Counter
from pathlib import Path


def snapshots(text):
    current = None
    for line in text.splitlines():
        if "SLICKS_POOL stage=" in line:
            if current is not None:
                yield current
            fields = dict(re.findall(r"(\w+)=([^ ]+)", line.split("SLICKS_POOL ", 1)[1]))
            current = {k: v if k == "stage" else int(v) for k, v in fields.items()}
            current["slots"] = {}
        elif "SLICKS_POOL_SLOT " in line:
            if current is None:
                raise ValueError("slot without snapshot header")
            fields = dict((k, int(v)) for k, v in re.findall(
                r"(\w+)=(-?\d+)", line.split("SLICKS_POOL_SLOT ", 1)[1]))
            slot = fields.pop("slot")
            if slot in current["slots"]:
                raise ValueError("duplicate slot")
            current["slots"][slot] = fields
    if current is not None:
        yield current


def after_update(state, life, page):
    if state > 0 and life:
        life -= 1
        if not life:
            if not page:
                life = 1
            else:
                state = -state
    if state < 0:
        state = ((state - 1 + 128) % 256) - 128
        if state in (-3, -7):
            state = 0
        elif state == -4:
            state = -3
    return state, life


def verify(items):
    if not items or len(items) % 2:
        raise ValueError("missing or incomplete PRE/POST pair")
    transitions = Counter()
    for i in range(0, len(items), 2):
        pre, post = items[i:i + 2]
        if pre["stage"] != "PRE" or post["stage"] != "POST":
            raise ValueError("wrong snapshot order")
        if pre["call"] != i // 2 + 1 or post["call"] != pre["call"]:
            raise ValueError("missing/duplicate call")
        if post["page"] != pre["page"] ^ 1:
            raise ValueError("page did not toggle")
        for snapshot in (pre, post):
            if snapshot["capacity"] != 200 or not 1 <= snapshot["high"] <= 200:
                raise ValueError("unexpected pool bounds")
            if set(snapshot["slots"]) != set(range(1, snapshot["high"])):
                raise ValueError("incomplete slot inventory")
        if pre["high"] != post["high"]:
            raise ValueError("actor pass changed high-water mark")
        for slot, before in pre["slots"].items():
            after = post["slots"][slot]
            expected = after_update(before["state"], before["life"], pre["page"])
            if (after["state"], after["life"]) != expected:
                raise ValueError(f"call {pre['call']} slot {slot}: "
                                 f"{before} -> {after}, expected {expected}")
            transitions[before["state"], after["state"]] += 1
    return transitions


if __name__ == "__main__":
    try:
        records = list(snapshots(Path(sys.argv[1]).read_text(errors="replace")))
        changes = verify(records)
        print(f"DOS pool: {len(records)//2} complete actor calls; "
              f"{sum(changes.values())} slot lifecycle comparisons passed")
        print("First PRE slots:", records[0]["slots"])
        print("State transitions:", dict(sorted(changes.items())))
    except (IndexError, OSError, ValueError) as error:
        sys.exit(f"DOS pool verification failed: {error}")
