#!/usr/bin/env python3
"""Check a native post-race table was saved without changing other track data.

Arguments: captured first.records, original version-2 track, saved track.
The captured 68020 struct contains 11*29 entry bytes, one alignment byte,
then a big-endian unsigned-short trailer. Compare this run's table rather
than another run's lap times. Original writer semantics are independently
covered by verify-track-record-write.
"""
from pathlib import Path
import sys


def verify(capture, original, saved):
    if len(capture) != 322:
        raise ValueError("Expected the 322-byte native record struct")
    if len(original) < 363 or original[5] != 2:
        raise ValueError("Expected a version-2 source track")
    if len(saved) != len(original) or saved[:8] != original[:8] or saved[363:] != original[363:]:
        raise ValueError("Track bytes outside the record block changed")
    entries = [capture[i*29:(i+1)*29] for i in range(11)]
    first = int.from_bytes(entries[0][20:22], "little", signed=True)
    second = int.from_bytes(entries[1][20:22], "little", signed=True)
    # Original writer promotes the first ranked entry into the overall record.
    if second < first or first <= 1:
        entries[0] = entries[1]
    total = 0
    for i, expected in enumerate(entries):
        at = 8+32*i
        stored = saved[at:at+29]
        if stored != expected:
            raise ValueError(f"Stored entry {i} differs from this run's calculated record")
        checksum = sum((value ^ 0x7b)+3 for value in stored) & 0xffff
        if int.from_bytes(saved[at+29:at+31], "big") != checksum or saved[at+31] != (checksum+0x66)&255:
            raise ValueError(f"Stored entry {i} checksum is invalid")
        total = (total+((checksum | 0x24)^0x64)) & 0xffff
    if saved[360:362] != capture[320:322] or saved[362] != total & 255:
        raise ValueError("Record trailer or total checksum differs")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("Usage: check_record_save.py first.records original-track saved-track")
    verify(*(Path(name).read_bytes() for name in sys.argv[1:]))
    print("All 11 saved records match this run's table; checksums and other track bytes are intact")
