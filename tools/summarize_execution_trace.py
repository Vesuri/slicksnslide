#!/usr/bin/env python3
"""Validate and summarize a normalized DOSBox-X instruction trace."""

import argparse
import csv
import hashlib
from pathlib import Path


RUNTIME_SIZE = 214_048


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_bitmap(path: Path) -> set[int]:
    data = path.read_bytes()
    if len(data) != RUNTIME_SIZE:
        raise SystemExit(
            f"{path}: expected {RUNTIME_SIZE} bytes, found {len(data)}"
        )
    invalid = {value for value in data if value not in (0, 1)}
    if invalid:
        raise SystemExit(f"{path}: bitmap contains values other than 0 and 1")
    return {offset for offset, value in enumerate(data) if value}


def load_edges(path: Path, executed: set[int]) -> set[tuple[int, int]]:
    edges: set[tuple[int, int]] = set()
    previous: tuple[int, int] | None = None
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != ["from_offset", "to_offset"]:
            raise SystemExit(f"{path}: unexpected CSV header {reader.fieldnames}")
        for line_number, row in enumerate(reader, start=2):
            try:
                edge = (int(row["from_offset"], 0), int(row["to_offset"], 0))
            except (TypeError, ValueError) as error:
                raise SystemExit(f"{path}:{line_number}: malformed edge") from error
            if not all(0 <= offset < RUNTIME_SIZE for offset in edge):
                raise SystemExit(f"{path}:{line_number}: edge outside runtime: {edge}")
            if not all(offset in executed for offset in edge):
                raise SystemExit(
                    f"{path}:{line_number}: edge endpoint is not an executed start: {edge}"
                )
            if edge in edges:
                raise SystemExit(f"{path}:{line_number}: duplicate edge: {edge}")
            if previous is not None and edge <= previous:
                raise SystemExit(f"{path}:{line_number}: edges are not strictly sorted")
            edges.add(edge)
            previous = edge
    return edges


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("bitmap", type=Path)
    parser.add_argument("edges", type=Path)
    parser.add_argument("--compare-bitmap", type=Path)
    parser.add_argument("--compare-edges", type=Path)
    args = parser.parse_args()
    if bool(args.compare_bitmap) != bool(args.compare_edges):
        parser.error("--compare-bitmap and --compare-edges must be used together")

    executed = load_bitmap(args.bitmap)
    if not executed:
        raise SystemExit(f"{args.bitmap}: no executed instruction starts")
    edges = load_edges(args.edges, executed)
    print(f"runtime bytes: {RUNTIME_SIZE}")
    print(
        f"executed starts: {len(executed)} "
        f"({len(executed) / RUNTIME_SIZE:.2%})"
    )
    print(f"executed range: 0x{min(executed):05x}..0x{max(executed):05x}")
    print(f"unique transitions: {len(edges)}")
    print(f"bitmap SHA-256: {sha256(args.bitmap)}")
    print(f"edges SHA-256: {sha256(args.edges)}")

    if args.compare_bitmap:
        compared = load_bitmap(args.compare_bitmap)
        compared_edges = load_edges(args.compare_edges, compared)
        start_union = executed | compared
        edge_union = edges | compared_edges
        print(
            f"start intersection/union: {len(executed & compared)}/"
            f"{len(start_union)} ({len(executed & compared) / len(start_union):.2%})"
        )
        print(
            f"edge intersection/union: {len(edges & compared_edges)}/"
            f"{len(edge_union)} ({len(edges & compared_edges) / len(edge_union):.2%})"
        )


if __name__ == "__main__":
    main()
