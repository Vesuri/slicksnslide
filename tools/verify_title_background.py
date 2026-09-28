"""Check keyless native title's lower background against the original asset.

Use diag_title_background.gdb with a fresh keyless REGCHECK fixture. This
checks only removal of the invented footer, not whole-title fidelity.
Original data and debugger captures remain local-only.
"""
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("vga", type=Path)
    args = parser.parse_args()
    archive = args.archive.read_bytes()
    count = int.from_bytes(archive[3:5], "big")
    entries = [archive[5+i*19:24+i*19] for i in range(count)]
    matches = [i for i, entry in enumerate(entries)
               if entry[:16].split(b"\0")[0] == b"mainmenu.@I"]
    if len(matches) != 1:
        raise SystemExit("Missing or ambiguous title asset")
    index = matches[0]
    start = int.from_bytes(entries[index][16:19], "big")
    end = (int.from_bytes(entries[index+1][16:19], "big")
           if index+1 < count else len(archive))
    asset = archive[start:end]
    if len(asset) != 64003 or asset[:3] != bytes((1, 64, 200)):
        raise SystemExit("Unexpected original title geometry")
    vga = args.vga.read_bytes()
    if len(vga) != 262144:
        raise SystemExit("Expected all four 64 KiB VGA banks")
    mismatches = sum(vga[(x & 3)*65536 + y*100 + (x >> 2)]
                     != asset[3+y*320+x]
                     for y in range(175, 200) for x in range(320))
    if mismatches:
        raise SystemExit(f"FAIL: {mismatches} pixels overwrite the original lower title background")
    print("PASS: all 8000 lower-title pixels match original artwork; no invented footer")


if __name__ == "__main__":
    main()
