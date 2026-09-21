#!/usr/bin/env python3
"""Verify stable frames and live advancement in a BASIC.SS reference run."""

import argparse
import hashlib
import subprocess
from pathlib import Path


EXPECTED_SPLASH = "ce0b18140fffbf11454f39bed9fc2a278f8ae7269373598932abd8b1f52b95cf"
EXPECTED_PLAYER_READY = "00ddb29b4e525e1b3f4194a80e1738b1143ef029bf12ce08210aaf99cd5da8a2"
EXPECTED_TRACK_CROP = "b2e5e678e8a49ef080739d38ea8470594cfa55925e7ad65eecece4a1b5e023d0"


def frame(video: Path, seconds: int, crop: str | None = None) -> bytes:
    command = [
        "ffmpeg", "-v", "error", "-ss", str(seconds), "-i", str(video),
        "-frames:v", "1",
    ]
    if crop:
        command.extend(["-vf", f"crop={crop}"])
    command.extend(["-f", "rawvideo", "-pix_fmt", "rgb24", "-"])
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE)
    if not result.stdout:
        raise SystemExit(f"no frame at {seconds} seconds")
    return result.stdout


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def expect(label: str, actual: str, expected: str) -> None:
    if actual != expected:
        raise SystemExit(f"{label}: expected {expected}, got {actual}")
    print(f"{label}: {actual}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("video", type=Path)
    args = parser.parse_args()

    expect("splash", digest(frame(args.video, 8)), EXPECTED_SPLASH)
    expect(
        "human player ready",
        digest(frame(args.video, 20, "100:30:400:185")),
        EXPECTED_PLAYER_READY,
    )

    live_hashes = []
    for seconds in (25, 28, 32):
        expect(
            f"BASIC label at {seconds}s",
            digest(frame(args.video, seconds, "80:20:0:380")),
            EXPECTED_TRACK_CROP,
        )
        live_hashes.append(digest(frame(args.video, seconds)))
    if len(set(live_hashes)) != len(live_hashes):
        raise SystemExit("race framebuffer did not advance at every checkpoint")
    print("race framebuffer advances at 25s, 28s, and 32s")


if __name__ == "__main__":
    main()
