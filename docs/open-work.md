# Open work

1. Continue from the genuine native `BASIC.SS` course frame into the first live
   car/sprite update and HUD. The current target decodes 110 original
   `SLICKS.DAT` images, parses and draws all 233 track records, loads the race
   palette from `SLICKS.000`, and passes a strict 2 MiB A1200 gate without any
   captured framebuffer. Port car-state initialization, sprite selection,
   countdown, input, collision, and the repeating update/present loop. Keep the
   translated path free of a generated-C CPU-context layer. Recover the
   original font resource to replace the compact 5x7 title renderer.
2. Convert the recovered contracts for all 12 live-named VGA functions into a
   native 68020 graphics ABI and differential test corpus. The plane-selected
   pixel read/write cores are complete and pass 2,040 x86-versus-68020 cases;
   the opaque and transparent sprite blitters pass 256 whole-plane cases each.
   Readback passes another 256 whole-segment cases, and sub-rectangle copy
   passes 256 whole-plane cases. The direct plotter passes 512 cases across all
   four caller-selected planes, and the half-open span filler passes 256
   whole-framebuffer cases, and the composed title-wrapper tail passes 256
   state-and-palette cases. The remapper now passes 256 whole-framebuffer
   cases; the full-plane clear and observed mode-zero setup are native,
   whole-framebuffer tested, and active in the A1200 path. The shared far-byte
   fill's portable RAM semantics are native; its VGA and text-memory call sites
   remain caller-level platform boundaries. The corrected race trace exposed one screen-transition
   sub-rectangle call (`source_y=105`, `height=150`, declared height `200`) that
   crosses the declared sprite and relies on 16-bit DOS offset wrap; either
   translate that caller at a higher level or add a circular 64-KiB source
   arena before claiming complete sub-rectangle coverage. The remaining
   dynamic range and buffer checks cover the bounded calls. The rectangular span-fill layout is statically recovered and
   traced, but the BASIC.SS fixture makes no calls; another mode must supply its
   dynamic range checks.
3. Turn the captured DOS/BIOS, port, and 16-writer VGA-memory inventories into
   service and native drawing-replacement contracts. The BASIC path has no
   writes overlapping executed instruction bytes; extend that proof to other
   tracks and modes. Interrupt vectors `08h` and `09h` are proved to enter
   `27A24h` and `26D29h`.
4. Replace wall/video-time input with a guest-state-triggered Enter event.
5. Define a compact application-state signature for race checkpoints so dynamic
   AI/timer differences can be compared semantically.
6. Classify the optional 386 Borland-runtime path and prove it remains dormant
   under the selected CPU identity.
7. Expand the normalized live-seeded Ghidra map with additional tracks and
   modes, preserving common versus path-specific coverage provenance.
