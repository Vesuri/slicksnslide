#!/usr/bin/env python3
"""Create the minimal deterministic Amiga LHA distribution (LH5)."""
import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
from installer_icon import installer_icon, drawer_icon, readme_icon

ORIGINAL_HASHES = {
    "d12114fccdd86b10e9d77d652acec35ea0a45230fcbe3fb32970431c18b55c2c",
}
PREFIX = "Slicks Install"

def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc

def member(name, data):
    # Use the established UNIX LHa encoder, but own the generic header so host
    # timestamps, permissions and paths cannot affect release reproducibility.
    encoder = os.environ.get("LHA", shutil.which("lha-compress") or str(Path.home()/".local/opt/lha/bin/lha"))
    with tempfile.TemporaryDirectory(prefix="slicks-lh5-") as directory:
        work = Path(directory)
        (work / "payload").write_bytes(data)
        try:
            subprocess.run([encoder, "ao5g0", "member.lha", "payload"],
                           cwd=work, check=True, capture_output=True)
        except (OSError, subprocess.CalledProcessError) as error:
            raise SystemExit("LH5 packaging requires LHa for UNIX (not Lhasa); "
                             "set LHA to its absolute path. See docs/install-original-data.md") from error
        raw = (work / "member.lha").read_bytes()
    header = raw[2:2 + raw[0]]
    if header[:5] != b"-lh5-" or header[18] != 0:
        raise ValueError("compressor did not produce a level-zero LH5 member")
    packed, unpacked = struct.unpack_from("<II", header, 5)
    payload = raw[2 + raw[0]:2 + raw[0] + packed]
    if unpacked != len(data) or len(payload) != packed:
        raise ValueError("invalid compressor output lengths")
    name = name.replace("/", "\\").encode("ascii")
    stamp = (((2026 - 1980) << 9) | (9 << 5) | 28) << 16
    body = b"-lh5-" + struct.pack("<III", len(payload), len(data), stamp)
    body += bytes((0x20, 0, len(name))) + name + struct.pack("<H", crc16(data))
    return bytes((len(body), sum(body) & 255)) + body + payload

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_directory", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    version = (root / "VERSION").read_text().strip()
    files = {
        "Slicks": args.executable.read_bytes(),
        "Slicks.slave": (root / "build/whdload/Slicks.slave").read_bytes(),
        "Slicks.inf": installer_icon(game=True),
        "SlicksInstallData": (root / "build/install-data/SlicksInstallData.exe").read_bytes(),
        "Install": (root / "release/Install").read_bytes(),
        "Install.info": installer_icon(),
        "ReadMe.info": readme_icon(),
        "ReadMe": (root / "release/ReadMe").read_bytes(),
        "puff-license.txt": (root / "tools/install-data/puff.h").read_bytes(),
        "CREDITS.txt": (root / "docs/release-credits.txt").read_bytes(),
    }
    for name, data in files.items():
        if hashlib.sha256(data).hexdigest() in ORIGINAL_HASHES:
            raise SystemExit("refusing to package original game data: " + name)
    args.output_directory.mkdir(parents=True, exist_ok=True)
    archive = args.output_directory / f"Slicks-{version}.lha"
    if archive.exists():
        raise SystemExit("Refusing to replace an existing release archive: " + str(archive))
    temporary = archive.with_suffix(".lha.part")
    temporary.write_bytes(member(PREFIX + '.info', drawer_icon())
        + b"".join(member(PREFIX + '/' + name, data) for name, data in sorted(files.items())) + b"\0")
    temporary.replace(archive)
    print(f"{archive}: {len(files)} drawer contents plus drawer icon, {archive.stat().st_size} bytes")

if __name__ == "__main__":
    main()
