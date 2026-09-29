#!/usr/bin/env python3
"""Generate synthetic, non-game-derived profiles for a native scroll audit."""
import pathlib
import sys

destination = pathlib.Path(sys.argv[1])
data = bytearray([0x97, 0, 97])
for index in range(3, 100):
    name = f"SCROLL PROFILE {index:02d}".encode().ljust(21, b"\0")
    record = name + bytes(18) + bytes([0, 100]) + bytes([40] * 9)
    record += bytes([0, 7, 63, 0, 0, 0, 0, 63])
    assert len(record) == 58
    data.extend(record)
with destination.open("xb") as output:
    output.write(data)
