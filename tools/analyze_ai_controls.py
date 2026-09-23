#!/usr/bin/env python3
"""Compare the recovered e204 route controller with a DOS semantic trace."""

import argparse
import csv
from pathlib import Path


def read_zones(path: Path):
    data = path.read_bytes()
    at = 6 + 0x165 + 5 + 10
    for _ in range(2):
        while data[at]:
            at += 1
        at += 1
    at += 8
    count = int.from_bytes(data[at : at + 2], "big")
    at += 2 + count * 5
    count = int.from_bytes(data[at : at + 2], "big")
    at += 2 + count * 6
    count = int.from_bytes(data[at : at + 2], "big")
    at += 2
    zones = []
    for _ in range(count):
        points = []
        for _ in range(3):
            points.append((int.from_bytes(data[at : at + 2], "big"),
                           data[at + 2]))
            at += 3
        at += 1
        zones.append(points)
    return zones


def direction(dx, dy):
    if dy == 0:
        return 12 if dx < 0 else 4
    if dx < -900 or dx > 900:
        dx >>= 6
        dy >>= 6
    ratio = int((dx << 6) / dy)
    if dy < 0:
        limits = ((-322, 4), (-96, 3), (-43, 2), (-13, 1),
                  (13, 0), (43, 15), (96, 14), (322, 13))
        fallback = 12
    else:
        limits = ((-322, 12), (-96, 11), (-43, 10), (-13, 9),
                  (13, 8), (43, 7), (96, 6), (322, 5))
        fallback = 4
    for limit, result in limits:
        if ratio < limit:
            return result
    return fallback


def wrap(value):
    if value > 8:
        value -= 16
    if value < -8:
        value += 16
    return value


def route_controls(row, target):
    x = int(row["x"]) // 100 - 3
    y = int(row["y"]) // 100 - 3
    target_direction = direction(target[0] - x, target[1] - y)
    heading_delta = wrap(int(row["heading"]) // 1200 + 4 -
                         target_direction)
    vx = int(row["velocity_before_x"])
    vy = int(row["velocity_before_y"])
    speed = (abs(vx) + abs(vy)) // 2
    velocity_delta = 0
    if speed > 700:
        velocity_delta = wrap(direction(int(vx * 10 / (speed + 1)),
                                            int(vy * 10 / (speed + 1))) -
                              target_direction)
    accelerate = brake = left = right = 0
    if heading_delta > 0:
        left = 1
    if heading_delta < 0:
        right = 1
    if abs(heading_delta) <= 1:
        accelerate = 1
    elif abs(heading_delta) > 5 and speed > 700:
        brake = 1
    if velocity_delta == 0:
        accelerate = 1
        brake = 0
    return accelerate, brake, left, right


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("track", type=Path)
    args = parser.parse_args()
    zones = read_zones(args.track)
    rows = []
    with args.trace.open(newline="") as source:
        for row in csv.DictReader(source):
            if (int(row["car"]) and int(row["contact"]) == 0 and
                    int(row["ai_state_0"]) < len(zones)):
                rows.append(row)
    by_car = {car: [row for row in rows if int(row["car"]) == car]
              for car in range(1, 4)}
    selected_drive = 0.0
    for point in range(3):
        for shift in range(-2, 3):
            for swap in (False, True):
                matched = steering = drive = total = 0
                for sequence in by_car.values():
                    for index, row in enumerate(sequence):
                        source_at = index + shift
                        if source_at < 0 or source_at >= len(sequence):
                            continue
                        source = sequence[source_at]
                        expected = tuple(int(row[name]) for name in
                                         ("accelerate", "brake", "left",
                                          "right"))
                        target = zones[int(source["ai_state_0"])][point]
                        actual = route_controls(source, target)
                        if swap:
                            actual = actual[:2] + (actual[3], actual[2])
                        matched += actual == expected
                        drive += actual[:2] == expected[:2]
                        steering += actual[2:] == expected[2:]
                        total += 1
                if shift == 0 and not swap:
                    print(f"point={point} all={matched / total:.2%} "
                          f"drive={drive / total:.2%} "
                          f"steer={steering / total:.2%}")
                    if point == 2:
                        selected_drive = drive / total
    if selected_drive < 0.97:
        raise SystemExit("third-point DOS drive replay fell below 97%")


if __name__ == "__main__":
    main()
