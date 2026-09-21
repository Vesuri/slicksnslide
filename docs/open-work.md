# Open work

1. Extend the observed BASIC-path translations beyond the proved title-page
   initializer (`195F0h..19639h`) and fixed crop redraw
   (`196ECh..19718h`) into their surrounding UI routine, then onward to an
   interactive menu and game-loop entry. Keep the translated-to-native-helper
   path free of a generated-C CPU layer. The palette matcher at `26EAEh` and
   the complete `19653h..19718h` prefix, colour-slot helper at `1FD63h`, and
   six-entry title-menu loop at `19719h..19825h` and observed BASIC title-status
   slice at `19828h..199F9h` and shared wrapper suffix at `19E4Ah..19EFEh` now
   run natively. Their common caller and its eight-key dispatch table at
   `1A2B2h..1A2C8h` are recovered; the scan-code classifier now runs natively
   and the BASIC Enter edge is proved to target `1A3CCh`. The original
   one-key reference had no human player and returned to the menu; the corrected
   script now adds a human player and reaches a sustained BASIC race. Regenerate
   the trace on that path, then translate the newly observed activation and
   game-entry blocks. The selected-zero path is proved to return zero to
   `16252h`, and its following 4-by-13 player state initialization at
   `16272h..162E4h` is native and differential-tested;
   The A1200 build now crosses that activation boundary and displays an exact
   sustained-race checkpoint through the native VGA blitter, captured palette,
   and Kalms conversion. Continue through the genuine race setup and first live
   update so the checkpoint becomes an executing, interactive race loop.
   `199FAh` is the alternate, unreached renderer. Recover the original
   font resource to replace the temporary compact 5x7 title and numeric
   vocabulary.
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
