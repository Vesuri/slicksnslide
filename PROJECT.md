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
| Normalized runtime SHA-256 | `c4b8ecdc9e350d782de1cac0019a0a0ad8feb8290fa29f545ae5115b41c88408` |
| Reconstructed MZ SHA-256 | `49ec0277299635a73c177465deef4f9eaeca38236b25ab6dce5b123b0395003f` |
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

An instrumented DOSBox Staging run independently loaded the executable at
`01A3:0000` and captured the same 214,048-byte handoff image. After reversing
the relocation delta and five load-specific words in Compack's dead copied
decoder, its normalized image is byte-identical to the Unicorn capture.

The runtime is primarily 16-bit and contains a CPU-level probe. A reachable
library routine has both an older-CPU implementation and an optional 386 path
using 32-bit register operations plus FS/GS. The reference configuration's CPU
identity is consequently part of the compatibility contract. The first target
policy is a 286-class guest; live traces must prove that no 386-only branch is
then exercised.

## First live execution surface

The official DOSBox-X `dosbox-x-v2026.08.31` 286 interpreter is patched to
recognize the Compack handoff and record instruction-entry offsets relative to
the relocated runtime base. It also records unique consecutive transitions
while both endpoints remain inside the runtime image. Thus traces from
different DOS load addresses use the same normalized offsets.

Two bounded BASIC.SS race runs reached the same `00000h..2C724h` span. The
shorter observed 15,099 instruction starts and 16,753 transitions; the longer
observed 22,671 starts and 25,742 transitions. Of the shorter run's starts,
14,989 (99.27%) also occurred in the longer run; 16,415 transitions (97.98%)
overlapped. Live race timing changes total coverage, so these files are
validated coverage evidence rather than fixed-hash golden outputs.

Capstone decodes every one of the shorter trace's 15,099 executed starts. Its
16,753 transitions comprise 13,346 fall-throughs, 37 repeat iterations, 801
call edges (16 indirect), 1,049 jump edges (2 indirect), 1,189 returns, and
331 unexplained non-sequential transfers. Of the latter, 329 converge on
offset `27A24h` and two on `26D29h`; the many unrelated source instructions
make these strong asynchronous-interrupt-entry candidates. Explicit interrupt
instrumentation must prove that interpretation. That shorter trace's
conservative live seed set contains 3,014 possible basic-block entries.

Explicit interrupt instrumentation now proves that interpretation on a larger
bounded run: all 666 otherwise unexplained transitions correlate exactly with
an interrupt event's normalized resume address and destination. Timer vector
`08h` entered `27A24h` 3,135 times and keyboard vector `09h` entered `26D29h`
twice. The run recorded 9,197 boundary events in total: 3,162 hardware events
and 6,035 software interrupts across vectors `10h`, `21h`, `2Fh`, and `33h`.
Its larger coverage produces 22,725 decoded instruction starts, 25,820 edges,
and 4,702 conservative block-entry candidates. Register snapshots at each
software interrupt provide the first measured DOS/BIOS service inventory.

Direct I/O tracing captures 10,290,225 BASIC-race operations as 885 compact
aggregates. The surface includes PIT/PIC/keyboard, joystick, Sound Blaster DSP
and DMA, and VGA attribute/sequencer/graphics/palette/CRTC/status ports. Its
millions of timer/retrace polls and dense VGA register traffic identify native
routine replacement—not generic port emulation—as a performance requirement.

Memory tracing reduces millions of accesses to compact aggregates. Across the
bounded BASIC runs, 19 instruction sites in 12 functions directly access VGA
memory, making them concrete native drawing-replacement boundaries. A
byte-level comparison of every runtime write against all dynamically decoded
instruction extents finds no write to executed code on the BASIC path after
the Compack handoff.

The live-seeded Ghidra workflow now disassembles 4,674 conservative block
entries without promoting them all to functions. It names 12 curated VGA
primitive candidates in both the relocated raw image and normalized MZ. The
auditable raw-image listing expands from 56,706 to 59,663 instructions (687 to
693 discovered functions); the normalized live listing contains 59,610
instructions and 692 functions. `live_vga_planar_subrect_blit` and
`live_vga_plot` now have proved far-cdecl stack contracts and dynamically
checked source/destination bounds, making them the first native 68020
replacement targets.

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
- [x] Phase 1: independent runtime-image proof and repeatable BASIC.SS reference
      race with stable-state and advancement gates.
- [ ] Phase 2: exhaustive entry-point and external-surface map.
- [ ] Phase 3: semantic IR and differential instruction corpus.
- [ ] Phase 4: native 68020 backend and block differentials.
- [ ] Phase 5: end-to-end Amiga skeleton.
- [ ] Phase 6: subsystem completion and measured optimization.
- [ ] Phase 7: packaging.

## Immediate next step

Recover the calling signatures and state contracts of the remaining live-named
VGA primitives while expanding trace coverage beyond BASIC.SS to distinguish
common engine code from track- and mode-specific paths.
