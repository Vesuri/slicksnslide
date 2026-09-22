# Open work

1. Extend the live `BASIC.SS` race from its first complete native update loop.
   Four persistent cars now decode their original directional archive sprites,
   initialize from the track start pose, steer through the track's original
   ten-byte navigation regions, leave persistent skidmarks, and update four
   on-screen timers. Cursor keys can take over car one; otherwise the original
   navigation records drive all four cars. The strict 2 MiB A1200 gate proves
   movement, route-region progress, skid output, timers, and two distinct
   rendered checksums after 200 frames. The runtime now loads each original
   34-byte `.omi` record; recovered acceleration and steering fields drive the
   cars independently, and the collision dimensions/weight are retained for
   the collision pass. Continue with collision response, countdown/lap rules,
   the remaining car-property interpretation, and the remaining race
   services. The starting grid is now oriented from each track's start heading
   and the recovered 120-tick, five-stage start sequence gates movement and
   race timing. Navigation regions are consumed in track order; wrapping the
   final region advances the lap and records current, last, and best lap
   times independently of total race time. Car-to-car contacts now use the
   recovered collision extent and
   weight, separate overlapping cars, exchange momentum, and are asserted by
   the live gate; replace the provisional native impulse with the exact DOS
   fixed-point response as that routine is recovered. Keep the translated
   path free of a generated-C CPU-context
   layer. Recover the original font resource to replace the compact 5x7 title
   renderer.
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
