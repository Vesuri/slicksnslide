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
bounded run: all 637 otherwise unexplained transitions correlate exactly with
an interrupt event's normalized resume address and destination. Timer vector
`08h` entered `27A24h` 3,135 times and keyboard vector `09h` entered `26D29h`
twice. The run recorded 9,244 boundary events in total: 3,162 hardware events
and 6,082 software interrupts across vectors `10h`, `21h`, `2Fh`, and `33h`.
Its larger coverage produces 22,970 decoded instruction starts, 26,003 edges,
and 4,728 conservative block-entry candidates. Register snapshots at each
software interrupt provide the first measured DOS/BIOS service inventory.

Direct I/O tracing captures 10,619,349 BASIC-race operations as 885 compact
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

The live-seeded Ghidra workflow now disassembles 4,763 conservative block
entries without promoting them all to functions. It names 12 curated VGA
primitive candidates in both the relocated raw image and normalized MZ. The
auditable raw-image listing expands from 56,706 to 59,756 instructions (687 to
693 discovered functions); the normalized live listing contains 59,703
instructions and 692 functions. `live_vga_planar_subrect_blit` and
`live_vga_plot` now have proved far-cdecl stack contracts and dynamically
checked source/destination bounds, making them the first native 68020
replacement targets.

The same probe proves the opaque and transparent sprite-buffer ABIs and VGA
readback ABI. The readback height is a word-sized stack argument whose low byte
alone is significant, and observed coordinates include `FFFFh`. Preserving
those exact 16-bit operations is part of the native graphics contract.
It also establishes the four-word Borland far-fill ABI and the seven-word
in-place VGA color-remap ABI; observed memory, lookup-table, and framebuffer
spans pass their 16-bit bounds checks.
The final named pixel helpers are now dynamically measured as well. A bounded
race made 428,740 plane-selected pixel reads and 88,708 plane-selected writes,
making direct native replacement of these call boundaries a higher priority
than general instruction-level optimization. The two-word mode-setup call
selects mode 0 with a 400-pixel virtual width, producing the observed
100-byte stride and 32,700-byte page separation.

The first production-language replacements now exist as Motorola-syntax 68020
assembly with a register ABI and no CPU-context traffic. The native
plane-selected plot and read routines pass 2,040 deterministic differential
cases against the original unpacked x86 bodies in separate Unicorn x86-16 and
68020 engines. The opaque and transparent planar sprite blitters pass another
256 cases each, including destination-phase rotation, odd and even widths,
transparent bytes, 16-bit starting-offset wrap, and whole-plane side-effect
comparison. Native readback passes 256 more whole-segment cases covering its
header, rotated payloads, right-edge padding, and untouched destination bytes.
The sub-rectangle sprite copy passes 256 whole-plane cases, including the
observed `100 x 97` crop and randomized source/destination offsets and strides.
The direct plot helper passes another 512 cases with its formerly implicit VGA
write plane made explicit in the native register ABI.

The first bootable Amiga HUNK now closes the build-to-display loop. It runs in
FS-UAE as an A1200 with exactly 2 MiB of chip memory and no fast memory, invokes
proved native graphics routines, converts the logical VGA store to an
eight-bitplane AGA Intuition screen, installs the original 256-colour palette,
and reaches its displayed-frame marker. The launch disk runs the user's local
AmigaOS `SetPatch` first. The small C shell is platform/display glue only; no
translated CPU execution passes through generated C. See
`docs/amiga-skeleton.md`.

The first application-level control-flow translation now replaces the recovered
checker-pattern rectangle function at runtime offset `A498h`. The bounded BASIC
trace has not reached this caller, but its callee is the heavily exercised live
plane-selected plot primitive. Its signed nested loops pass 256
whole-framebuffer x86-versus-68020 cases. With the observed title-page and crop
callers, palette utility, composed UI prefix, UI colour-slot helper, native
span filler, composed bevel, and shared title tail, the complete differential
gate now covers 6,011 cases. Linked into
the A1200 diagnostic, the translated checker block produced a repeatable
post-display checksum of `86bdc061` across fresh FS-UAE boots. The target check
also exposed and fixed the first platform ABI issue:
callee-saved M68k registers must survive the temporary C/display boundary.

Call-time primitive instrumentation now records caller offsets, exact planar
sprite blobs, and the active VGA DAC palette. A bounded BASIC.SS trace captured
the complete 320 by 200 title frame (`a4fc8a1cbea08a30`) and its palette
(`9b17b223ef7f93e3`) at full-page calls `1960Dh`/`19632h` and the later
sub-rectangle call `19711h`. The Amiga build extracts those ignored bytes by
hash. A direct 68020 translation of the observed `195F0h..19639h` caller now
sends the frame through the native opaque blitter to both original page bases
(`7FBCh`, then `0000h`) and displays the recognizable original title screen.
The original relocated x86 caller and composed 68020 caller+callee agree in 33
whole-framebuffer cases. A second direct translation covers the observed
`196ECh..19718h` fixed `100 by 97` title redraw through the native sub-rectangle
blitter, passing another 34 whole-framebuffer cases. Both callers now execute
in the A1200 diagnostic. The strict target check verifies checksum `87956515`.
The surrounding routine's helper at `26EAEh` is now identified as a
nearest-colour palette search and translated directly to 68020. Its 512-case
differential exposed and preserved the original's asymmetric component
handling: requested bytes are sign-extended, while palette bytes are unsigned.
The native `sui_title_step` now composes that helper with the animation-counter
arithmetic and title crop for the complete observed `19653h..19718h` prefix.
All 256 possible initial counter bytes agree with the original relocated x86
caller across its live colour outputs, counter and stored-colour mutations,
preserved registers, and four-plane framebuffer. This composed step executes
in the strict A1200 diagnostic without changing its `87956515` frame checksum.
The observed selected-index-zero menu slice at `19719h..19825h` now follows
that prefix on target: its exact bevel geometry is differentially proved, and
a native 5x7 renderer places all six resolved English labels. Two fresh A1200
boots produced the new post-menu checksum `c61a023d`. The compact font is an
intermediate native subsystem replacement; original-font recovery remains.
The following observed BASIC-path status cluster at `19828h..199F9h` is also
native. Extended call tracing proves the four live indicator signs and exact
sprite hashes, both decimal values (`1`), their positions and styles, and the
absence of all three optional badges. The Amiga build extracts the two measured
2-byte-by-8-row sprite blobs by hash and composes them with the transparent
blitter; the strict target checksum is now `37048854`.
The shared wrapper suffix at `19E4Ah..19EFEh` now advances its 16-bit phase,
reproduces the three-part red pulse, performs the original nearest-palette
lookup, and mutates colour slot zero through the native helper. The BASIC path
proves the optional text branch inactive; its final VGA start-address call with
coordinates `(0,0)` is an Amiga no-op. A 256-case composed differential covers
the phase boundaries, wrap, palette result, state mutation, and preserved
registers.

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
- [ ] Phase 5: end-to-end Amiga skeleton. The boot/build/display foundation,
      first translated application block, and full reference-derived title
      frame are proved; original game-loop entry remains.
- [ ] Phase 6: subsystem completion and measured optimization.
- [ ] Phase 7: packaging.

## Immediate next step

Follow the observed caller continuations at `1A1E3h` and `1A26Fh` now that the
shared title wrapper is native through its return at `19EFEh`. The renderer at
`199FAh` remains the alternate, unreached wrapper branch. Recover the original
Slicks font resource to replace the compact title and numeric vocabulary
renderer. In parallel, extend the native differential corpus to remap,
remaining fill, clear, and mode-setup boundaries and expand trace coverage
beyond BASIC.SS.
