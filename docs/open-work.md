# Open work

1. Recover calling signatures and semantic contracts for the remaining
   live-named VGA functions. The sub-rectangle blitter, three pixel helpers,
   opaque/transparent sprite blits, and readback helper now have stack layouts;
   dynamic range and buffer checks cover the plotter and four blit/readback
   primitives.
2. Turn the captured DOS/BIOS, port, and 15-writer VGA-memory inventories into
   service and native drawing-replacement contracts. The BASIC path has no
   writes overlapping executed instruction bytes; extend that proof to other
   tracks and modes. Interrupt vectors `08h` and `09h` are proved to enter
   `27A24h` and `26D29h`.
3. Replace wall/video-time input with a guest-state-triggered Enter event.
4. Define a compact application-state signature for race checkpoints so dynamic
   AI/timer differences can be compared semantically.
5. Classify the optional 386 Borland-runtime path and prove it remains dormant
   under the selected CPU identity.
6. Expand the normalized live-seeded Ghidra map with additional tracks and
   modes, preserving common versus path-specific coverage provenance.
