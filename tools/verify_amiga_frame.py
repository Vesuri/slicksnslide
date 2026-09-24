#!/usr/bin/env python3
"""Compare target-dumped 320x200 chunky and eight interleaved Amiga planes.

Read-only verification: no generated or captured frame becomes a game asset.
The target fixture must check BytesPerRow=320 and Depth=8 before dumping.
"""
import argparse
from pathlib import Path


def compare(chunky: bytes, planar: bytes) -> int:
    if len(chunky) != 64000 or len(planar) != 64000:
        raise ValueError("Both target dumps must contain exactly 64000 bytes")
    mismatches = 0
    for y in range(200):
        for x in range(320):
            mask = 0x80 >> (x & 7)
            at = y * 320 + (x >> 3)
            actual = sum(1 << p for p in range(8) if planar[at + p * 40] & mask)
            expected = chunky[y * 320 + x]
            if actual != expected:
                if mismatches < 10:
                    print(f"Mismatch x={x} y={y}: chunky={expected} bitplanes={actual}")
                mismatches += 1
    return mismatches


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("chunky", type=Path)
    parser.add_argument("planar", type=Path)
    args = parser.parse_args()
    try:
        mismatches = compare(args.chunky.read_bytes(), args.planar.read_bytes())
    except (OSError, ValueError) as error:
        parser.error(str(error))
    if mismatches:
        print(f"FAIL: {mismatches} pixels differ")
        return 1
    print("PASS: all 64000 displayed pixels match the authoritative chunky surface")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
