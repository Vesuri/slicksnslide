#!/usr/bin/env python3
"""Verify semantic car-state facts recovered from the DOS BASIC race."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


# The breakpoint records each car as its update returns.  Car 0 is idle in the
# first captured frame; the AI cars have already applied their first steering
# step, so their first heading is 4651 rather than the grid heading 4800.
GRID_POSITIONS = {
    (26326, 5274),
    (26326, 6126),
    (25474, 6126),
    (25474, 5274),
}


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
    nonzero_velocity_samples = 0
    first_positions: set[tuple[int, int]] = set()
    for car, samples in by_car.items():
        if not samples:
            raise SystemExit(f"missing car {car}")
        first = samples[0]
        observed_position = (int(first["x"]), int(first["y"]))
        first_positions.add(observed_position)
        expected_heading = 4800 if car == 0 else 4651
        if int(first["heading"]) != expected_heading:
            raise SystemExit(
                f"car {car} first heading {first['heading']}, "
                f"expected {expected_heading}"
            )
        previous = samples[0]
        for current in samples[1:]:
            before = scalar(previous)
            after = scalar(current)
            factor = int(current["q15_5"])
            if (
                current["accelerate"] == "0"
                and current["brake"] == "0"
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
            if int(current["velocity_x"]) or int(current["velocity_y"]):
                nonzero_velocity_samples += 1
            previous = current

    if first_positions != GRID_POSITIONS:
        raise SystemExit(
            f"first grid positions {sorted(first_positions)!r}, "
            f"expected {sorted(GRID_POSITIONS)!r}"
        )

    if coast_matches < 100:
        raise SystemExit(
            f"only {coast_matches} exact Q15 coast transitions; expected at least 100"
        )
    if steering_matches < 1000:
        raise SystemExit(
            f"only {steering_matches}/{steering_samples} exact steering "
            "transitions; expected at least 1000"
        )
    if nonzero_velocity_samples < 1000:
        raise SystemExit(
            f"only {nonzero_velocity_samples} nonzero velocity samples; "
            "expected at least 1000"
        )
    print(
        f"DOS race state: {len(rows)} samples, exact first states and "
        f"{coast_matches} Q15 coast transitions, plus "
        f"{steering_matches}/{steering_samples} literal steering transitions "
        f"and {nonzero_velocity_samples} post-integrator velocity samples "
        "verified"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
