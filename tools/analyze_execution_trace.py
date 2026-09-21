#!/usr/bin/env python3
"""Decode a normalized live trace and classify its observed transitions."""

import argparse
import csv
from collections import Counter
from pathlib import Path

from capstone import (
    CS_ARCH_X86,
    CS_GRP_CALL,
    CS_GRP_IRET,
    CS_GRP_JUMP,
    CS_GRP_RET,
    CS_MODE_16,
    Cs,
)
from capstone.x86 import X86_OP_IMM
from capstone.x86_const import (
    X86_INS_JCXZ,
    X86_INS_JECXZ,
    X86_INS_LOOP,
    X86_INS_LOOPE,
    X86_INS_LOOPNE,
)

from summarize_execution_trace import load_bitmap, load_edges


LOOP_INSTRUCTIONS = {
    X86_INS_JCXZ, X86_INS_JECXZ, X86_INS_LOOP, X86_INS_LOOPE, X86_INS_LOOPNE,
}


def classify(instruction, destination: int) -> str:
    source = instruction.address
    if destination == source:
        return "repeat"
    if destination == source + instruction.size:
        return "fallthrough"
    if instruction.group(CS_GRP_CALL):
        return "direct-call" if any(
            operand.type == X86_OP_IMM for operand in instruction.operands
        ) else "indirect-call"
    if instruction.group(CS_GRP_JUMP) or instruction.id in LOOP_INSTRUCTIONS:
        return "direct-jump" if any(
            operand.type == X86_OP_IMM for operand in instruction.operands
        ) else "indirect-jump"
    if instruction.group(CS_GRP_RET) or instruction.group(CS_GRP_IRET):
        return "return"
    return "unexplained-nonsequential"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime", type=Path)
    parser.add_argument("bitmap", type=Path)
    parser.add_argument("edges", type=Path)
    parser.add_argument("--entries", type=Path)
    args = parser.parse_args()

    image = args.runtime.read_bytes()
    executed = load_bitmap(args.bitmap)
    edges = load_edges(args.edges, executed)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True

    instructions = {}
    failures = []
    for offset in sorted(executed):
        decoded = next(decoder.disasm(image[offset:offset + 15], offset, count=1), None)
        if decoded is None:
            failures.append(offset)
        else:
            instructions[offset] = decoded
    if failures:
        sample = ", ".join(f"0x{offset:05x}" for offset in failures[:8])
        raise SystemExit(f"failed to decode {len(failures)} executed starts: {sample}")

    categories = Counter()
    entry_reasons = {0: {"runtime-entry"}}
    incoming = Counter(destination for _, destination in edges)
    for source, destination in edges:
        category = classify(instructions[source], destination)
        categories[category] += 1
        if category != "fallthrough":
            entry_reasons.setdefault(destination, set()).add(
                f"observed-{category}"
            )

    # Instructions reached only after execution outside the runtime (typically
    # a DOS/BIOS interrupt) have no incoming in-runtime edge.
    external_resumes = {offset for offset in executed if not incoming[offset]}
    for offset in external_resumes:
        entry_reasons.setdefault(offset, set()).add("external-resume")

    for instruction in instructions.values():
        if instruction.id in LOOP_INSTRUCTIONS or any(
            instruction.group(group)
            for group in (CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET, CS_GRP_IRET)
        ):
            fallthrough = instruction.address + instruction.size
            if fallthrough in executed:
                entry_reasons.setdefault(fallthrough, set()).add(
                    "post-control-flow"
                )

    print(f"decoded executed starts: {len(instructions)}")
    for category in (
        "fallthrough", "repeat", "direct-call", "indirect-call",
        "direct-jump", "indirect-jump", "return",
        "unexplained-nonsequential",
    ):
        print(f"{category}: {categories[category]}")
    print(f"external/runtime re-entry starts: {len(external_resumes)}")
    print(f"conservative observed block entries: {len(entry_reasons)}")
    unexplained_destinations = Counter(
        destination for source, destination in edges
        if classify(instructions[source], destination) ==
        "unexplained-nonsequential"
    )
    if unexplained_destinations:
        leaders = ", ".join(
            f"0x{offset:05x} ({count})"
            for offset, count in unexplained_destinations.most_common(5)
        )
        print(f"leading unexplained destinations: {leaders}")
    if args.entries:
        args.entries.parent.mkdir(parents=True, exist_ok=True)
        with args.entries.open("w", newline="") as output:
            writer = csv.writer(output, lineterminator="\n")
            writer.writerow(("offset", "reasons"))
            for offset, reasons in sorted(entry_reasons.items()):
                writer.writerow((f"0x{offset:05x}", ";".join(sorted(reasons))))
        print(f"wrote block-entry candidates: {args.entries}")


if __name__ == "__main__":
    main()
