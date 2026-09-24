#!/usr/bin/env python3
"""Reject the recursive strlen optimization that overflowed the Amiga stack."""
import re
import subprocess
import sys


def verify(disassembly):
    match = re.search(r"^[0-9a-f]+ <strlen>:\n(.*?)(?=\n\n|\Z)",
                      disassembly, re.MULTILINE | re.DOTALL)
    if not match:
        raise ValueError("strlen was not found in the runtime object")
    instructions = re.findall(r"^\s*[0-9a-f]+:\s+(\S+)",
                              match.group(1), re.MULTILINE)
    if not instructions or "rts" not in instructions:
        raise ValueError("strlen has no recognized return")
    # This runtime's strlen must stay a leaf; reject tail calls as well.
    if any(op.split('.')[0] in ("jsr", "bsr", "jmp") for op in instructions):
        raise ValueError("strlen contains a call/jump: possible runtime recursion")


if __name__ == "__main__":
    output = subprocess.check_output(
        [sys.argv[1], "-d", "--no-show-raw-ins", "--disassemble=strlen",
         sys.argv[2]], text=True)
    verify(output)
    print("AMIGA_RUNTIME_STRLEN_LEAF_OK")
