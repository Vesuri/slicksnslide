#!/usr/bin/env python3
"""Normalize a relocated runtime dump using the recovered MZ relocation set."""

import argparse
import hashlib
import json
from pathlib import Path

from rebuild_runtime_mz import (
    load_relocations,
    normalize_compack_scratch,
    normalize_image,
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime", type=Path)
    parser.add_argument("state", type=Path)
    parser.add_argument("relocations", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    loaded = bytearray(args.runtime.read_bytes())
    state = json.loads(args.state.read_text())
    if state["image_size"] != len(loaded):
        raise SystemExit("runtime dump size and capture state disagree")
    rows = load_relocations(args.relocations)
    if "relocation_count" in state and state["relocation_count"] != len(rows):
        raise SystemExit("relocation CSV and capture state disagree")

    normalized, _ = normalize_image(loaded, state["registers"]["cs"], rows)
    normalize_compack_scratch(normalized, state["registers"]["cs"])
    args.output.write_bytes(normalized)
    digest = hashlib.sha256(normalized).hexdigest()
    print(f"wrote {args.output}: {len(normalized)} bytes, sha256 {digest}")


if __name__ == "__main__":
    main()
