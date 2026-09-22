#!/usr/bin/env python3
"""Verify semantic car-state facts recovered from the DOS BASIC race."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


# The breakpoint records each car as its update returns.  Car 0 is idle in the
# first captured frame; the AI cars have already applied their first steering
# step, so their first heading is 4651 rather than the grid heading 4800.
FIRST_SAMPLE = {
    0: (26326, 5274, 4800),
    1: (26326, 6126, 4651),
    2: (25474, 6126, 4651),
    3: (25474, 5274, 4651),
}

# Default vehicle lineup 5,2,0,0.  FUN_2000_aeb6 constructs DS:53fe as
# 0x7bdd - ((omi[23] - 100) * 2), giving these Q15 coast multipliers.
COAST_FACTOR = {0: 31789, 1: 31773, 2: 31709, 3: 31709}


def scalar(row: dict[str, str]) -> int:
    state = bytes.fromhex(row["car_state_hex"])
    if len(state) != 0x36:
        raise ValueError("car_state_hex is not 54 bytes")
    return int.from_bytes(state[0x10:0x14], "little", signed=True)


def signed_word(state: bytes, offset: int) -> int:
    return int.from_bytes(state[offset : offset + 2], "little", signed=True)


def trunc_div(numerator: int, denominator: int) -> int:
    """Match 286 IDIV truncation toward zero (Python // rounds down)."""
    quotient = abs(numerator) // abs(denominator)
    return -quotient if (numerator < 0) != (denominator < 0) else quotient


def steering_step(row: dict[str, str]) -> int:
    state = bytes.fromhex(row["car_state_hex"])
    amount = int(row["steering_input"])
    amount *= trunc_div(int(row["steering_scale"]), 10)
    amount = trunc_div(amount, 155)
    amount *= 80 - trunc_div(signed_word(state, 0x26), 25)
    amount = trunc_div(amount, 100)
    amount *= int(row["steering_property"])
    amount = trunc_div(amount, 50)
    return amount * int(row["timestep"])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    with args.trace.open(newline="") as source:
        rows = list(csv.DictReader(source))
    if not rows:
        raise SystemExit("empty race-state trace")

    by_car: dict[int, list[dict[str, str]]] = {i: [] for i in range(4)}
    for row in rows:
        car = int(row["car"])
        if car in by_car:
            by_car[car].append(row)

    coast_matches = 0
    steering_matches = 0
    steering_samples = 0
    for car, samples in by_car.items():
        if not samples:
            raise SystemExit(f"missing car {car}")
        first = samples[0]
        observed = (int(first["x"]), int(first["y"]), int(first["heading"]))
        if observed != FIRST_SAMPLE[car]:
            raise SystemExit(
                f"car {car} first sample {observed!r}, "
                f"expected {FIRST_SAMPLE[car]!r}"
            )
        factor = COAST_FACTOR[car]
        previous = samples[0]
        for current in samples[1:]:
            before = scalar(previous)
            after = scalar(current)
            if (
                previous["accelerate"] == "0"
                and previous["brake"] == "0"
                and before > 0
                and after == before * factor // 0x8000
            ):
                coast_matches += 1
            left = current["left"] == "1"
            right = current["right"] == "1"
            if left != right:
                steering_samples += 1
                turn = steering_step(current)
                expected = (
                    int(previous["heading"]) + (turn if right else -turn)
                ) & 0xFFFF
                if int(current["heading"]) == expected:
                    steering_matches += 1
            previous = current

    if coast_matches < 100:
        raise SystemExit(
            f"only {coast_matches} exact Q15 coast transitions; expected at least 100"
        )
    if steering_matches < 1000:
        raise SystemExit(
            f"only {steering_matches}/{steering_samples} exact steering "
            "transitions; expected at least 1000"
        )
    print(
        f"DOS race state: {len(rows)} samples, exact first states and "
        f"{coast_matches} Q15 coast transitions, plus "
        f"{steering_matches}/{steering_samples} literal steering transitions "
        "verified"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
