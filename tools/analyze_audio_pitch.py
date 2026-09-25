#!/usr/bin/env python3
"""Measure loop recurrence in a single-engine emulator recording (not synthesis)."""
import argparse
import wave
from pathlib import Path

import numpy as np


def sample_length(archive, block):
    data = archive.read_bytes()
    assert data[:3] == b"MF\x1a"
    count = int.from_bytes(data[3:5], "big")
    for i in range(count):
        entry = data[5 + i * 19:24 + i * 19]
        if entry[:16].split(b"\0", 1)[0] != b"samples.dat":
            continue
        at = int.from_bytes(entry[16:19], "big")
        for index in range(block + 1):
            if data[at:at + 4] == b"RIFF":
                end = at + 8 + int.from_bytes(data[at + 4:at + 8], "little")
                chunk = at + 12
                size = None
                while chunk < end:
                    length = int.from_bytes(data[chunk + 4:chunk + 8], "little")
                    if data[chunk:chunk + 4] == b"data":
                        size = length
                    chunk += 8 + length + (length & 1)
                assert size is not None
            else:
                assert data[at:at + 2] == b"tS"
                size = int.from_bytes(data[at + 3:at + 6], "big")
                header = 10 if data[at + 6] == 0 else 8
                end = at + header + size
            if index == block:
                return size
            at = end
    raise ValueError("Missing sample")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recording", type=Path)
    parser.add_argument("--start", type=float, required=True)
    parser.add_argument("--seconds", type=float, default=2)
    parser.add_argument("--frequency", type=float, required=True)
    parser.add_argument("--raw-rate", type=int)
    parser.add_argument("--archive", type=Path, default=Path("ref/SLICKS.000"))
    parser.add_argument("--block", type=int, default=17)
    args = parser.parse_args()
    if args.raw_rate:
        rate = args.raw_rate
        pcm = np.fromfile(args.recording, dtype="<i2").reshape(-1, 2)
    else:
        with wave.open(str(args.recording), "rb") as f:
            assert f.getsampwidth() == 2
            rate = f.getframerate()
            pcm = np.frombuffer(f.readframes(f.getnframes()), dtype="<i2").reshape(-1, f.getnchannels())
    pcm = pcm[round(args.start * rate):round((args.start + args.seconds) * rate)].astype(float)
    assert len(pcm) >= round(args.seconds * rate), "Recording too short"
    channel = np.argmax(np.sum(pcm * pcm, axis=0))
    signal = pcm[:, channel] - pcm[:, channel].mean()
    assert np.max(np.abs(signal)) > 10, "Silent recording"
    n = len(signal)
    fft_size = 1 << (2 * n - 1).bit_length()
    spectrum = np.fft.rfft(signal, fft_size)
    ac = np.fft.irfft(spectrum * spectrum.conj(), fft_size)[:n]
    energy = np.concatenate(([0.0], np.cumsum(signal * signal)))
    lag = np.arange(n)
    denom = np.sqrt(energy[n - lag] * (energy[n] - energy[lag]))
    ac = np.divide(ac, denom, out=np.zeros_like(ac), where=denom > 0)
    length = sample_length(args.archive, args.block)
    expected = rate * length / args.frequency
    lo, hi = max(1, int(expected * .90)), min(n - 1, int(expected * 1.10))
    assert lo < hi
    peak = lo + int(np.argmax(ac[lo:hi + 1]))
    measured = rate * length / peak
    print(f"sample_bytes={length} channel={channel} requested_hz={args.frequency:g} "
          f"measured_loop_rate_hz={measured:.3f} lag={peak} correlation={ac[peak]:.6f} "
          f"error_percent={(measured / args.frequency - 1) * 100:.4f}")
    if ac[peak] < .90:
        raise SystemExit("No strong loop recurrence near the requested pitch; do not accept this measurement")


if __name__ == "__main__":
    main()
