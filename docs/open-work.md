# Open work

1. Extend the observed BASIC-path translations beyond the proved title-page
   initializer (`195F0h..19639h`) and fixed crop redraw
   (`196ECh..19718h`) into their surrounding UI routine, then onward to an
   interactive menu and game-loop entry. Keep the translated-to-native-helper
   path free of a generated-C CPU layer. The palette matcher at `26EAEh` and
   the complete `19653h..19718h` prefix, colour-slot helper at `1FD63h`, and
   six-entry title-menu loop at `19719h..19825h` and observed BASIC title-status
   slice at `19828h..199F9h` now run natively. Continue with the observed shared
   wrapper suffix at `19E4Ah`; `199FAh` is its alternate, unreached renderer.
   Recover the original font resource to replace the temporary compact 5x7
   title and numeric vocabulary.
2. Convert the recovered contracts for all 12 live-named VGA functions into a
   native 68020 graphics ABI and differential test corpus. The plane-selected
   pixel read/write cores are complete and pass 2,040 x86-versus-68020 cases;
   the opaque and transparent sprite blitters pass 256 whole-plane cases each.
   Readback passes another 256 whole-segment cases, and sub-rectangle copy
   passes 256 whole-plane cases. The direct plotter passes 512 cases across all
   four caller-selected planes, and the half-open span filler passes 256
   whole-framebuffer cases. Next are the remapper, remaining fills, clear, and
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
