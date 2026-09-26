#!/usr/bin/env python3
"""Build a deliberately allowlisted, data-free native development package."""
import argparse
import hashlib
import json
import struct
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def package(output):
    binary = (ROOT / "amiga/out/SlicksDiag.exe").read_bytes()
    # elf2hunk output: empty resident-name list, followed by the hunk-size table.
    if len(binary) < 20:
        raise ValueError("truncated executable")
    magic, names, count, first, last = struct.unpack_from(">5I", binary)
    if magic != 0x3F3 or names or first != 0 or last + 1 != count or not 0 < count < 100:
        raise ValueError("unexpected Amiga HUNK header")
    sizes = struct.unpack_from(f">{count}I", binary, 20)
    contents = {
        "Slicks/SlicksDiag": binary,
        "Slicks/README.txt": (ROOT / "docs/release-readme.txt").read_bytes(),
        "Slicks/CREDITS.txt": (ROOT / "docs/release-credits.txt").read_bytes(),
    }
    manifest = {
        "source_commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "source_dirty": bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=ROOT, text=True)),
        "target": "PAL A1200 / 68020 / 2 MiB chip / no Fast RAM",
        "hunk_allocation_bytes": sum((size & 0x3FFFFFFF) * 4 for size in sizes),
        "note": "Hunk allocation excludes runtime allocations and OS memory.",
        "sha256": {name: hashlib.sha256(data).hexdigest() for name, data in contents.items()},
    }
    contents["Slicks/MANIFEST.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    # Refuse accidental replacement of an earlier release artifact.
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in contents.items():
            entry = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = (0o100755 if name.endswith("SlicksDiag") else 0o100644) << 16
            archive.writestr(entry, data)
    with zipfile.ZipFile(output) as archive:
        if set(archive.namelist()) != set(contents) or archive.testzip():
            raise ValueError("package membership/integrity check failed")
        for name, data in contents.items():
            if archive.read(name) != data:
                raise ValueError(f"package verification failed: {name}")
    print(f"Verified {output}: four allowlisted files, no original data or emulator artifacts")
    print(f"Executable {len(binary)} bytes; declared hunk allocation {manifest['hunk_allocation_bytes']} bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    package(parser.parse_args().output.resolve())
