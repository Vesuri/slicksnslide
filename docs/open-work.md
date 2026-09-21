# Open work

1. Extend the first proved application-level translation from the recovered
   checker rectangle at runtime offset `A498h` to an observed BASIC-path routine
   that consumes original game data and produces a recognizable
   reference-derived frame region. Keep the translated-to-native-helper path
   free of a generated-C CPU layer.
2. Convert the recovered contracts for all 12 live-named VGA functions into a
   native 68020 graphics ABI and differential test corpus. The plane-selected
   pixel read/write cores are complete and pass 2,040 x86-versus-68020 cases;
   the opaque and transparent sprite blitters pass 256 whole-plane cases each.
   Readback passes another 256 whole-segment cases, and sub-rectangle copy
   passes 256 whole-plane cases. The direct plotter passes 512 cases across all
   four caller-selected planes. Next are the remapper, fills, clear, and
   mode-setup boundaries. Dynamic range and buffer checks already cover those
   routines. The rectangular span-fill layout is statically recovered and
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
