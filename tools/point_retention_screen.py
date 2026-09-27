#!/usr/bin/env python3
"""Offline design screen, NOT a retention implementation or timing measurement.

Reads diag_point_retention_screen.gdb output. Uses future geometry, treats all
sprite rectangles as writes (even retained ones), and conservatively blocks
equal-priority conflicts irrespective of handle order. It cannot establish
runtime correctness or account for the cost of predicting future writes.
"""
import argparse
import json
import re
from pathlib import Path


def parse(text):
    snapshots = {}
    current = None
    for line in text.splitlines():
        kind = line.split(" ", 1)[0]
        if kind not in {"SNAPSHOT", "POINT", "CAR", "SHADOW", "SPRITE"}:
            continue
        fields = {}
        for name, value in re.findall(r"(\w+)=(-?\d+(?:,-?\d+)*)", line):
            values = tuple(map(int, value.split(",")))
            fields[name] = values if len(values) > 1 else values[0]
        if kind == "SNAPSHOT":
            phase = fields["phase"]
            if phase in snapshots:
                raise ValueError("duplicate snapshot phase")
            current = {"header": fields, "points": {}, "rects": []}
            snapshots[phase] = current
        elif current is None:
            raise ValueError("record before snapshot")
        elif kind == "POINT":
            handle = fields["h"]
            if handle in current["points"]:
                raise ValueError("duplicate particle handle")
            # Reject old/incomplete captures rather than assuming unchanged colour.
            for key in ("old", "priority", "saved", "life", "state", "permanent", "colour", "mask"):
                if key not in fields:
                    raise ValueError(f"missing point field {key}")
            current["points"][handle] = fields
        elif fields.get("saved", 1):
            current["rects"].append(fields)
    if set(snapshots) != {0, 1} or "INSPECTION_OK" not in text:
        raise ValueError("incomplete inspection")
    before, after = snapshots[0], snapshots[1]
    if after["header"]["frame"] != before["header"]["frame"] + 1:
        raise ValueError("snapshots are not adjacent updates")
    for snapshot in (before, after):
        if len(snapshot["points"]) != snapshot["header"]["count"]:
            raise ValueError("particle count does not match snapshot")
    return before, after


def visible(point):
    x, y = point["old"]
    return point["state"] > 0 and point["saved"] & 1 and 0 <= x < 320 and 0 <= y < 184


def screen(before, after, cell):
    old, new = before["points"], after["points"]
    candidates = {h for h in old.keys() & new.keys()
                  if visible(old[h]) and visible(new[h])
                  and all(old[h][k] == new[h][k]
                          for k in ("old", "priority", "colour", "mask", "permanent"))}
    width, height = (320 + cell - 1) // cell, (184 + cell - 1) // cell
    minimum = [256] * (width * height)
    writes = 0

    def block(rect, priority):
        nonlocal writes
        x, y, w, h = rect
        left, top, right, bottom = max(x, 0), max(y, 0), min(x + w, 320), min(y + h, 184)
        if right <= left or bottom <= top:
            return
        for cy in range(top // cell, (bottom - 1) // cell + 1):
            for cx in range(left // cell, (right - 1) // cell + 1):
                index = cy * width + cx
                minimum[index] = min(minimum[index], priority)
                writes += 1

    for snapshot in (before, after):
        for rect in snapshot["rects"]:
            block(rect["rect"], rect["priority"])
        for h, point in snapshot["points"].items():
            if h not in candidates and visible(point):
                block((*point["old"], 1, 1), point["priority"])
    # Baking occurs outside the forward draw order: it blocks every priority.
    for point in old.values():
        if visible(point) and point["permanent"] and point["life"] == 1 and before["header"]["page"]:
            block((*point["old"], 1, 1), -1)
    for h, point in new.items():
        if h not in old and point["permanent"] and point["state"] < 0:
            # New points can expire on their first advance. Saved bits are
            # cleared then; conservatively include the coordinate regardless.
            x, y = point["xy"]
            block((x // 64, y // 64, 1, 1), -1)
    kept = []
    for h in candidates:
        p = old[h]
        x, y = p["old"]
        if minimum[(y // cell) * width + x // cell] > p["priority"]:
            kept.append(h)
    return {"cell": cell, "stationary": len(candidates), "kept": len(kept),
            "grid_bytes": width * height, "cell_visits": writes}


def self_test():
    def point(x=20, priority=3, **kwargs):
        p = dict(old=(x, 20), xy=(x * 64, 1280), priority=priority, saved=1,
                 life=4, state=1, permanent=0, colour=7, mask=0)
        p.update(kwargs)
        return p
    def snap(points, rects=(), page=1):
        return dict(points=points, rects=rects, header=dict(page=page))
    a = snap({1: point()})
    assert screen(a, a, 1)["kept"] == 1
    assert screen(a, snap({1: point(colour=8)}), 1)["kept"] == 0
    for priority, expected in ((2, 0), (3, 0), (4, 1)):
        b = snap({1: point()}, [dict(rect=(20, 20, 1, 1), priority=priority)])
        assert screen(a, b, 1)["kept"] == expected
    b = snap({1: point(), 2: point(x=21, priority=0)})
    assert screen(a, b, 1)["kept"] == 1
    assert screen(a, b, 4)["kept"] == 0
    a = snap({1: point(priority=0), 2: point(priority=7, life=1, permanent=1)})
    b = snap({1: point(priority=0), 2: point(priority=7, life=0, permanent=1, state=-6, saved=0)})
    assert screen(a, b, 1)["kept"] == 0
    # Stable points sharing a pixel may remain together if their order and
    # backgrounds remain unchanged; conflict from any lower write rejects both.
    a = snap({1: point(), 2: point(priority=5)})
    assert screen(a, a, 1)["kept"] == 2
    print("Point retention screen: colour, priority, baking, overlap and grid-boundary checks pass")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="*", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    for path in args.logs:
        before, after = parse(path.read_text())
        print(json.dumps({"log": str(path), "frame": after["header"]["frame"],
                          "old_count": len(before["points"]), "new_count": len(after["points"]),
                          "grids": [screen(before, after, n) for n in (1, 2, 4, 8, 16)]}))


if __name__ == "__main__":
    main()
