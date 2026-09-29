# Open work

Updated 2026-09-29. Actionable open and deferred work only.
Historical experiments, rejections and verification evidence are in
[performance-profiling.md](performance-profiling.md).
Installer/packaging evidence is in [release-verification.md](release-verification.md),
with usage in [install-original-data.md](install-original-data.md).

## Active goal

Work through the actionable menu/presentation, caching and fidelity-audit
items below. Commit verified pieces and keep historical evidence separate.
The explicitly paused performance backlog and deferred manual tests remain
paused/deferred; completing the active list does not meet the 50 FPS target.

## Retained performance goal and measurements

Finish the performance handoff with verified implementation or measured
rejection of each proposal and resolve its verification debt. Closing
experiments alone is not completion: faithful general gameplay, including
particle-heavy frames, must take at most 20 ms / 312 PAL raster lines per
update on a stock A1200 (68020, 2 MiB Chip RAM, no Fast RAM).
Preserve behaviour, effects, permanent marks, audio and rendering order.

Latest accepted bounded-actor-scan benchmark, work lines per 603 updates /
worst: BASIC 142797/348, F1 167671/409, CITY 138919/327, WHACKO 146604/367.
The phase checks reach F1 413 and WHACKO 384, so the
worst observed F1 update still needs about a 24% reduction. These are sampled
maxima, not exhaustive bounds. Do not confuse average work with 50 FPS.

## Registration verification

- The optional external `webf_ord.bmp` order-form image needs a visual check
  if matching original data becomes available; it is absent from the supplied
  archive. Do not substitute another image and call this check complete.

Implementation and completed verification evidence are separate in
[registration-support.md](registration-support.md).

## Menu loading and presentation

- **Whole-port fidelity audit is active and incomplete.** Use the coverage
  matrix and discrepancy IDs in [fidelity-audit.md](fidelity-audit.md). Audit
  production callers, parameters and overrides as well as translated helpers.
  Do not claim feature completeness from isolated or historical passes.
- Close F08's remaining title cadence/reference comparison (the normal
  selected-label pulse is restored and its complete native cycle verified). Audit the
  remaining title shortcuts against original callers: F12/idle-timeout demo
  setup is currently ignored by the native owner.
  Implement their configuration backup/restore and race-return lifecycle;
  the isolated original demo setup/restore helper now passes 960 paired
  original-instruction comparisons, but is not connected to the live owner.
  Audit the signed demo flag's race input, AI, results and persistence consumers
  before enabling it; the setup profile IDs precede Arcade selection override.
  The demo exit-key classifier passes all 65,536 byte pairs, but is not wired
  into production. Preserve F11/F12's exceptional track-data views; they are
  not exit keys or file captures. Their native display route remains missing.
  do not merely map them to ordinary GO. The added mouse activation path
  has been removed following the original keyboard-reader audit.
  F9 now reaches normal race preparation independently of the highlighted row.
  Finish F10's caller/transition font-alias lifetime and input-route audit.
  The Arcade body now passes original full-screen and font-state comparisons.
  The separate Arcade renderer, original arrow/action
  routing and mutable DS:0f1a player override are connected; native Options
  round-trip and two-human/two-computer race handoff pass. Isolated rendering
  comparisons do not establish remaining caller and mouse/shortcut coverage.
  F03–F05 now have direct original-instruction layout/navigation comparisons;
  native interactive sequence coverage remains to be extended.
- Recheck save/edit fixtures which previously assumed the extra intermission
  rows. Keep the original hidden Save Game route covered.
- Audit every remaining subsystem in the coverage matrix, distinguishing
  confirmed defects from unverified coverage and user-authorized adaptations.

- Finish disk-free ordinary navigation using the startup-resident inventory in
  [menu-resident-assets.md](menu-resident-assets.md). The 57-resource cache and
  keyboard snapshot are implemented; title Help, Options, Players, Controllers,
  main Tracks and pause paths have been migrated. Evidence is separate in
  [menu-cache-verification.md](menu-cache-verification.md).
  Cache the SLICKS.TRK and saved-game catalogues, preserving transactional
  save/delete updates and recovery/error handling. Migrate remaining chooser,
  results/return and RAM-only transition owners. Shop and registration Help
  use the cache, with native tests recorded in the evidence document. Keep explicit
  track/exit loads and saves as disk boundaries. Extend no-I/O/no-teardown
  assertions across every owner; rerun full 2 MiB/default-stack release gates.

## Remaining performance work

Optimization is stopped at the user's request. No experiment is active;
the production build is restored to the latest accepted implementation.
The remaining items below are a backlog, not work currently being executed.
The 50 FPS target remains unmet.

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
   Before another rewrite, quantify the selected block's cost and expected
   savings, and explain the regressions recorded in the profiling document.
   Do not repeat the rejected standalone or wider finishing integrations
   unchanged. The tested isolated `src/game/car_progress.s` register entry
   remains available but is not linked into gameplay. Keep emission,
   progress, collision, surface/damage/effects and final contact-latch order.
   Integration still needs real flag/Arcade/finish side-effect comparisons,
   stateful lap-limit query order and external reward callback handling in
   the shadow harness. Screen performance before those expensive runs.

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
  `tmp/pcprof-post-bound-{f1,whacko}-20260928`, captured from 7eeeb1f
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
