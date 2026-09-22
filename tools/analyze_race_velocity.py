#!/usr/bin/env python3
"""Compare candidate reconstructions of the DOS fixed-point velocity update."""

from __future__ import annotations

import csv
import sys
from collections import Counter
from pathlib import Path


DIR_X = (100, 92, 71, 38, 0, -38, -71, -92,
         -100, -92, -71, -38, 0, 38, 71, 92)
DIR_Y = (0, 38, 71, 92, 100, 92, 71, 38,
         0, -38, -71, -92, -100, -92, -71, -38)


def s32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def mul32(left: int, right: int) -> int:
    return s32(left * right)


def div0(numerator: int, denominator: int) -> int:
    if not denominator:
        return 0
    quotient = abs(numerator) // abs(denominator)
    return -quotient if (numerator < 0) != (denominator < 0) else quotient


def word(state: bytes, offset: int) -> int:
    return int.from_bytes(state[offset : offset + 2], "little", signed=True)


def scalar(state: bytes) -> int:
    return int.from_bytes(state[0x10:0x14], "little", signed=True)


def force_expression(row: dict[str, str], direction: int, constant: int) -> int:
    state = bytes.fromhex(row["car_state_hex"])
    value = mul32(int(row["drive4"]), int(row["drive1"]))
    value = mul32(value, div0(word(state, 0x20), 70) + 10)
    value = mul32(value, constant)
    value = mul32(value, direction)
    return mul32(value, scalar(state))


def force_operands(row: dict[str, str], direction: int, constant: int) -> tuple[int, int]:
    state = bytes.fromhex(row["car_state_hex"])
    divisor = mul32(int(row["drive4"]), int(row["drive1"]))
    divisor = mul32(divisor, div0(word(state, 0x20), 70) + 10)
    divisor = mul32(divisor, constant)
    drive_scalar = scalar(state)
    if int(row["drive_branch"]):
        drive_scalar = s32(mul32(drive_scalar, int(row["q15_5"]))) >> 15
    numerator = mul32(direction, drive_scalar)
    numerator = mul32(numerator, 200)
    return numerator, divisor


def main() -> int:
    path = Path(sys.argv[1])
    with path.open(newline="") as source:
        rows = list(csv.DictReader(source))
    previous: dict[int, dict[str, str]] = {}
    matches: Counter[str] = Counter()
    branch_matches: Counter[tuple[int, str]] = Counter()
    examples = 0
    tested = 0
    closed_matches = 0
    closed_mismatches = 0
    force_division_matches = 0
    force_division_mismatches = 0
    force_operand_matches = 0
    force_operand_mismatches = 0
    for row in rows:
        car = int(row["car"])
        old = previous.get(car)
        previous[car] = row
        if old is None or old["sample"] == row["sample"]:
            continue
        branch = int(row["drive_branch"])
        q15 = int(row["q15_4"] if branch else row["q15_3"])
        divisor = 0x8000 + int(row["drive2"])
        constant = 23 if branch else 38
        heading = int(row["heading"]) % 0x4B00
        direction = heading // 0x4B0
        for axis, table in (("x", DIR_X), ("y", DIR_Y)):
            old_velocity = int(row[f"velocity_before_{axis}"])
            observed = int(row[f"velocity_{axis}"])
            expression = force_expression(row, table[direction], constant)
            decay = div0(mul32(old_velocity, q15), divisor)
            force = int(row[f"force_{axis}"])
            expected_numerator, expected_divisor = force_operands(
                row, table[direction], constant
            )
            if (
                expected_numerator == int(row[f"force_numerator_{axis}"])
                and expected_divisor == int(row[f"force_divisor_{axis}"])
            ):
                force_operand_matches += 1
            else:
                force_operand_mismatches += 1
            force_from_operands = div0(
                int(row[f"force_numerator_{axis}"]),
                int(row[f"force_divisor_{axis}"]),
            )
            if force_from_operands == force:
                force_division_matches += 1
            else:
                force_division_mismatches += 1
            closed = s32(decay + force)
            if closed == observed:
                closed_matches += 1
            else:
                closed_mismatches += 1
            candidates = {
                "expression_over_200": s32(div0(expression, 200) + decay),
                "200_over_expression": s32(div0(200, expression) + decay),
                "expression_over_200_q15": s32(
                    div0(expression, 200 * divisor) + decay
                ),
            }
            for name, candidate in candidates.items():
                if candidate == observed:
                    matches[name] += 1
                    branch_matches[(branch, name)] += 1
            if not any(candidate == observed for candidate in candidates.values()) and examples < 8:
                print(
                    f"sample={row['sample']} car={car} axis={axis} branch={branch} "
                    f"old={old_velocity} observed={observed} expression={expression} "
                    f"q15={q15} divisor={divisor} candidates={candidates}"
                )
                examples += 1
            tested += 1
    print(f"velocity transitions tested: {tested}")
    print(f"closed update matches: {closed_matches}")
    print(f"closed update mismatches: {closed_mismatches}")
    print(f"force division matches: {force_division_matches}")
    print(f"force division mismatches: {force_division_mismatches}")
    print(f"force operand matches: {force_operand_matches}")
    print(f"force operand mismatches: {force_operand_mismatches}")
    for name, count in matches.most_common():
        print(f"{name}: {count}")
    for (branch, name), count in sorted(branch_matches.items()):
        print(f"branch {branch} {name}: {count}")
    return int(
        closed_mismatches != 0
        or force_division_mismatches != 0
        or force_operand_mismatches != 0
    )


if __name__ == "__main__":
    raise SystemExit(main())
