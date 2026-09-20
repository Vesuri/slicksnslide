# Phases and gates

## Phase 0 — scaffold and source proof

- Record the exact source hash and MZ header.
- Lock the A1200/68020/2 MiB target and direct-native architecture.
- Keep all copyrighted and byte-derived material local.

Exit: `make inspect` reproduces the recorded facts from the local source.

## Phase 1 — reference loop and true runtime image

- Run the original unattended in an instrumentable PC environment.
- Detect the Compack transfer to unpacked code.
- Capture the post-unpack memory image and complete entry state.
- Reproduce the capture independently from the file and unpacker semantics.
- Prove the selected executable reaches a named game state.

Exit: a repeatable command produces identical runtime bytes and state, and the
reference loop has a positive completion signal from guest state.

## Phase 2 — static/live map

- Import the proved runtime image into Ghidra.
- Seed direct, indirect, interrupt, callback, and observed runtime entries.
- Inventory DOS/BIOS interrupts, ports, VGA memory, executable writes, and
  overlay behavior.
- Classify remaining bytes as code, data, padding, or unresolved.

Exit: static and live evidence agree on the executable surface sufficiently to
generate without silently treating unknown targets as data.

## Phase 3 — x86 semantics

- Implement the decoded 8086/186/286 subset actually present.
- Represent overlapping registers, wrapping, segmentation, flags, faults, and
  little-endian memory explicitly in the IR.
- Differential-test every semantic operation against an independent oracle.

Exit: randomized instruction and short-block tests are clean.

## Phase 4 — native 68020 backend

- Allocate guest values to native registers by liveness.
- Keep flags virtual and reuse CCR where semantics agree.
- Inline ordinary RAM accesses and endian conversion.
- Chain all statically resolved edges directly.
- Emit auditable Motorola-syntax assembly.

Exit: generated M68k blocks match the x86 oracle and pass spill/helper/dispatch
audits.

## Phase 5 — target skeleton

- Link translated blocks with the narrow runtime.
- Implement the minimum DOS/BIOS/device surface needed to reach a named state.
- Run headlessly under FS-UAE and collect target-side counters.

Exit: an end-to-end path runs natively on the A1200 model and produces a
ground-truth-comparable milestone.

## Phase 6 — complete and optimize

- Extend subsystems only from observed demand.
- Close indirect targets, overlays, code modification, timing, video, input,
  and sound.
- Optimize from target profiles without weakening differential gates.

## Phase 7 — package

- Establish legal local-source requirements.
- Produce the Amiga package, documentation, and reproducible build instructions.
