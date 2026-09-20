# Slicks — DOS 286 to Amiga native static recompiler

Translate the supplied DOS executable directly to native 68020 code for an
A1200 with 2 MiB of memory. This is an ahead-of-time binary translator, not a
C transliteration and not a whole-PC emulator.

## Decisions (locked)

| Question | Decision |
|---|---|
| Target | Amiga A1200, 68020, 2 MiB |
| Fidelity | Original executable under the PC reference loop is ground truth |
| Translation | 286 machine code -> semantic IR -> native 68020 assembly |
| Generated C | None in the translated CPU path |
| Runtime | Narrow DOS/BIOS/device surface actually exercised by this executable |
| Dynamic gaps | Explicit dispatcher and fallback only where static/live analysis cannot close the target set |
| Optimization | Native register residency, lazy flags, direct block chaining, measured native replacements |
| Source policy | Original and byte-derived artifacts remain local and ignored |

## Measured source facts

Source: local `SLICKS.EXE`, copied from the user-supplied file.

| Property | Value |
|---|---|
| Size | 100,412 bytes |
| SHA-256 | `17e5a2a2daba0f3fcc280dc1b3711a753497bdb956ba74ee88b7e3db095d98eb` |
| Container | DOS MZ executable |
| Wrapper | Compack compressed |
| Header paragraphs | 2 (32 bytes) |
| Relocations | 0 |
| Visible entry | `CS:IP = 0000:0000` (the Compack stub) |
| Visible stack | `SS:SP = 30BF:0800` |
| Minimum allocation | `1BC9h` paragraphs |

The header and first code bytes identify the 1991 W. Collis Compack wrapper.
The visible entry point is not the game. `tools/unpack_compack.c` now executes
only that wrapper and stops immediately before its far transfer to the real
program.

## Measured runtime facts

| Property | Value |
|---|---|
| Compack instructions to handoff | 1,280,415 |
| Runtime image size | 214,048 bytes (`34420h`) |
| Runtime SHA-256 | `d6717daa23f40f0e968e610ee901ce8075c0f92b58240361b5541eedc8b65f0e` |
| Recovered relocation sites | 4,192 |
| Reconstructed MZ SHA-256 | `bba3b44eee288d50de5c63871d9a7a6f56c7faa0f96eb0267ae3ad254fdbe130` |
| Entry | `1010:0000` |
| Stack | `4442:0100` |
| Initial data segments | `DS = ES = 1000h` (PSP) |
| Toolchain evidence | Borland C++ 1994 runtime |

The captured bytes are the loaded, relocated memory image. Ghidra therefore
imports them at physical address `10100h`; it may display the same address as
`1000:0100` rather than `1010:0000`.
`tools/rebuild_runtime_mz.py` reverses all captured relocation adjustments,
checks a byte-exact round trip, and packages the normalized image as a local,
ignored MZ executable.

The runtime is primarily 16-bit and contains a CPU-level probe. A reachable
library routine has both an older-CPU implementation and an optional 386 path
using 32-bit register operations plus FS/GS. The reference configuration's CPU
identity is consequently part of the compatibility contract. The first target
policy is a 286-class guest; live traces must prove that no 386-only branch is
then exercised.

## Intended pipeline

```text
SLICKS.EXE
  -> MZ header/load reconstruction
  -> PC reference run and post-Compack runtime capture
  -> Ghidra headless + static/live entry sweep
  -> curated symbols and hardware/DOS surface
  -> x86 decoder + semantic IR
  -> block/superblock optimizer and register allocator
  -> Motorola-syntax 68020 assembly
  -> existing Amiga cross-toolchain and runtime
  -> FS-UAE/GDB and real-target measurement
```

## Repository layout

```text
SLICKS.EXE             local original, ignored
CLAUDE.md              standing agent rules
PROJECT.md             decisions, measured source facts, status
docs/                  design, evidence, gates, and work queue
tools/                 MZ, reference, analysis, translator, and audit tools
ghidra_scripts/        headless analysis scripts
disasm/                generated local artifacts; curated maps may be tracked
src/ir/                target-independent x86 semantics
src/m68k/              register allocator and 68020 backend
src/runtime/           dispatch, DOS/BIOS/device services, fallback boundary
src/platform/amiga/    target integration
amiga/                 build, run, debug, and diagnostic scripts
```

## Status

- [x] Phase 0a: repository initialized; source fingerprinted; direct-native
      architecture and source policy recorded.
- [x] Phase 0b: install the PC reference tools and reproduce the Compack
      handoff and unpacked runtime image with a bounded host capture.
- [ ] Phase 1: unattended reference loop and runtime-image proof.
- [ ] Phase 2: exhaustive entry-point and external-surface map.
- [ ] Phase 3: semantic IR and differential instruction corpus.
- [ ] Phase 4: native 68020 backend and block differentials.
- [ ] Phase 5: end-to-end Amiga skeleton.
- [ ] Phase 6: subsystem completion and measured optimization.
- [ ] Phase 7: packaging.

## Immediate next step

Validate the host-produced runtime image against an independent Bochs or DOSBox
run of the complete game distribution. Then establish an unattended reference
milestone and collect its executed blocks, indirect targets, interrupts, ports,
and executable-memory writes.
