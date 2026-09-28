# Open work

Updated 2026-09-28. Actionable open and deferred work only.
Historical experiments, rejections and verification evidence are in
[performance-profiling.md](performance-profiling.md).

## Goal and current measurements

Finish the performance handoff with verified implementation or measured
rejection of each proposal and resolve its verification debt. Closing
experiments alone is not completion: faithful general gameplay, including
particle-heavy frames, must take at most 20 ms / 312 PAL raster lines per
update on a stock A1200 (68020, 2 MiB Chip RAM, no Fast RAM).
Preserve behaviour, effects, permanent marks, audio and rendering order.

Latest accepted bounded-clear benchmark, work lines per 603 updates /
worst: BASIC 146033/348, F1 170942/415, CITY 142376/333, WHACKO 149752/384.
The phase checks reach F1 416 and WHACKO 386, so the
worst observed F1 update still needs about a 25% reduction. These are sampled
maxima, not exhaustive bounds. Do not confuse average work with 50 FPS.

## Remaining performance work

1. **Particle pipeline and memory traffic.** Examine substantial native
   blocks spanning restoration, ordering, advancement and drawing. Reduce
   repeated scans and Chip-RAM accesses; keep pointers/intermediates in
   registers across boundaries. For unmoved-particle retention, establish
   a bookkeeping cost below the saved restore/redraw work before integration.
   Preserve underlays, permanent-mark baking, slot reuse and exact ordering.
   Design constraints and screens are in
   [point-retention-design.md](point-retention-design.md).
   Do not repeat the measured slow C retention policies or compact-pool
   integration unchanged.
   A new design must reduce per-point work or whole traversals, not merely
   combine existing restore dispatchers. Use mixed-chain oracle cases and
   RETCHECK before accepting target performance results. The isolated mixed
   restoration entry is available, but is not linked into production;
   its rejected integration must not be repeated unchanged.

2. **F1 sprite overhead.** Reduce conflict processing, repeated validation,
   group rebuilding and late restoration. Inspect complete hot paths, not
   just argument setup. Preserve atomic group retention, reverse restoration
   order, initial saved-actor fallback and sprite/overflow boundaries.
   Prove any removed check redundant. Whole-list reuse is not a justified
   worst-frame fix; incremental maintenance needs writer coverage and a
   break-even measurement.

3. **Coherent register-resident 68020 simulation blocks.** Use refreshed
   profiles to select remaining car preparation, per-car tails, checkpoint,
   layer and clock work. Avoid stacked arguments, repeated saves/restores,
   large-offset loads and redundant byte accesses across helper boundaries.
   Preserve all-car-motion-before-tails order and callback invalidation.
   Reuse coordinates only with exact signed/wrapping semantics.
   Surface-table/cache/fusion experiments are closed; do not spend further
   iterations on that narrow family. Previous isolated assembly or C
   experiments do not settle the value of a larger register-oriented design.
   Next inexpensive algorithmic screen: in ai_steering, calculate velocity
   direction only when speed > 700 and the wrapped heading difference lies
   outside [-1,1]. The existing final branch accelerates unconditionally
   inside that interval, so the pure direction result cannot affect controls.
   Preserve the target-direction calculation, steering latch and all rounding
   when velocity direction is needed. Check against verify-dos-ai, then time
   F1/WHACKO before broader validation. Do not change the running build.

4. **C2P/publication, secondary priority.** Only pursue a new design with
   evidence of reduced total memory/instruction cost. The eight-pixel/hybrid
   integration and area-aware merge variants must not be repeated unchanged.
   Include platform publications and initial rectangles in area measurements.
   Use the independent coverage oracles and exact pixel comparisons.

5. **End-to-end completion proof.** Benchmark accepted changes on BASIC, F1,
   CITY and WHACKO, including particle-heavy updates and HUD-clock phases.
   Keep known transition regressions (especially F1 481/551/613 and WHACKO
   685/688) in scope. Refresh CPU profiles after substantial changes; sparse
   samples at a single update cannot establish precise cost percentages.
   Continue optimization until measured worst updates meet 312 lines with
   fidelity intact; do not redefine the target around means or easy cases.

## Verification and working rules

- Fail-fast order: build/quick correctness smoke tests, parent performance
  comparison, then expensive correctness/rendering validation only for
  promising candidates, followed by final timing confirmation. Do not pay
  for full validation of a candidate already rejected by performance.
- Current profiling baseline is
  `tmp/pcprof-post-surface-{f1,whacko}-20260928`, captured from 10f12e1
  with exact companion ELFs and zero missed samples.
  The profiling document records all earlier measurements and rejected work.
- Compare `amiga/bench_tracks.sh` against an exact parent-build control;
  FINAL_STATE must match. Use uninterrupted timing, not debugger-stopped runs.
  Include HUD phases when timing variation could obscure a regression.
- Native routines require independent Unicorn/host oracles. Simulation
  changes also require `amiga/shadow_check.sh` at the relevant sites.
  All-site mask is 0x3fe; surface tail is site 9 / SHADOW_SITES=512.
- Rendering changes require F1/CITY/WHACKO display audits:
  `SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 ./debug.sh "" diag_dirty_sprites.gdb`.
  Drawing-order/retention changes additionally require RETCHECK and
  `diag_retention_check.gdb`, with the independent geometry/legacy path.
- Expensive diagnostics remain opt-in; time all normal-game work.
  Force relevant object rebuilds when changing SHADOW/RETCHECK flags.
- Never rebuild an ELF in use by an emulator. Debug runs are muted; keep
  run.sh audible. Close every emulator session started for this work.
- Commit each verified piece as Vesa Halttunen <vesuri@jormas.com>, with
  hooks/signing disabled and no co-author trailer. Push only when asked.
  Never commit original executables, dumps, traces, screenshots or other
  byte-derived game material.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
