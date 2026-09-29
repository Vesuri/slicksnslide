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

- Audit every menu's repaint bounds and C2P publication. Preserve horizontal
  as well as vertical dirty bounds, merge overlapping rectangles, and convert
  only the covered blocks. Check ordinary selection changes, nested dialogs,
  restoration, scrolling and transitions; reserve full-screen conversion for
  actual full-screen replacements. Verify producer coverage and native pixels.
  Remaining publication coverage: additional Tracks storage/format
  failures, other Help owners,
  shop refresh selectors/driver changes/failures, remaining saved-game recovery,
  intermission/results error routes and remaining registration warning failures
  (archive/surface failures and malformed navigation; viewer-allocation failure
  now has native publication/return coverage).
  Title timing, shortcuts and demo fidelity remain separate open items below.
  Audit remaining full-width text callbacks, full-screen modal
  restores and direct row-converter bypasses separately from the shared
  publisher. The registered title-owner pulse now uses verified glyph bounds;
  the ordinary menu-text callback also uses native-store-verified glyph bounds,
  with current Players-menu publication coverage. Its other owners and other
  callbacks still need their own caller coverage.
  Direct full-screen callers are classified in menu-cache-verification.md;
  preserve screen-entry initialization where the destination background is
  stale, and verify the remaining error-route/lifetime cases natively.
  Extend failure-path coverage where only normal transitions have
  been checked. Completed publication and restoration evidence is kept in
  [menu-cache-verification.md](menu-cache-verification.md); do not infer
  whole-menu correctness from a subset of its fixtures.
- **Whole-port fidelity audit is active and incomplete.** Use the coverage
  matrix and discrepancy IDs in [fidelity-audit.md](fidelity-audit.md). Audit
  production callers, parameters and overrides as well as translated helpers.
  Do not claim feature completeness from isolated or historical passes.
- Close F08's remaining title cadence/reference comparison (the normal
  selected-label pulse is restored and its complete native cycle verified). Audit the
  remaining title shortcuts against original callers. Finish the demo lifecycle:
  connect the original loading painter and verified filename/suffix builder to
  race preparation, including the actual caller font alias and display lifetime.
  The original startup assignment of `/KIRJ.@F` to DS:0680 is now executed by
  the loading oracle; later caller/transition alias lifetime remains to be audited.
  Its isolated drawing oracle is in `verify-loading-pixels`; this is not live
  integration. The current platform-end disk boundary restores the OS display,
  so merely drawing before that call does not preserve the loading screen.
  The diagnostic-only `begin_io`/`end_io` handoff now exercises disk reads while
  retaining the custom display allocation; verify visible scanout before
  connecting it to real loading, then cover preparation failures and cleanup.
  A short manual visual check has been requested; `diag_loading_io_visual.gdb`
  pauses after 50 OS-serviced refreshes. Desktop capture did not expose this
  FS-UAE instance, and the installed binary did not execute the Lua hook.
  Extend idle-timer coverage to synchronous modal returns; long Options/Help
  visits, ordinary navigation reset and repeated automatic entry are covered.
  Race-view validation failure has native error/restore/retry coverage;
  startup display-allocation cleanup is verified separately below.
  Clean and dirty exit, direct save retry, save-failure cancellation, demo re-entry and
  subsequent successful save have native coverage.
  Compare the complete return presentation against the
  original saved-image/title sequence. DS:4c1c is captured from the prepared
  startup background before subsequent text/menu draws, not immediately before
  a demo; use that producer when constructing the pixel comparison.
  The missing startup background tints (F14) are fixed and independently
  pixel-verified; the subsequent complete return composition remains open.
  Keep finish-event rewards distinct from
  the later skipped track/results branch; do not rewind the shared RNG.
  Keyboard entry, track-data views, repeated key return and natural deadline
  return are integrated;
  their bounded native verification is recorded in fidelity-audit.md, not
  proof of all persistence/error routes or exact return presentation. Extend normal
  GO/F9 integration regressions alongside the remaining demo routes. Normal
  title F1 and ignored-F2 navigation now have a current-build native check;
  extend caller coverage to other modes/rows without adding an F2 title action.
  Finish F10's caller/transition font-alias lifetime and input-route audit.
  The Arcade body now passes original full-screen and font-state comparisons.
  The separate Arcade renderer, original arrow/action
  routing and mutable DS:0f1a player override are connected; native Options
  round-trip and two-human/two-computer race handoff pass. Isolated rendering
  comparisons do not establish remaining caller and mouse/shortcut coverage.
  F03–F05 now have direct original-instruction layout/navigation comparisons;
  native interactive sequence coverage remains to be extended.
- Recheck save/edit fixtures which previously assumed the extra intermission
  rows. The hidden Save Game cancel/name/save/exit sequence is covered;
  recheck load/edit entry routes without restoring invented visible rows.
  The original title-owner reachability check cannot select Load Game; exposing
  it as an extension is awaiting the user's choice. Audit other original entry
  points separately. Repeated-save/edit testing now uses a genuine first
  intermission; load/rejection fixtures still need their entry route resolved.
  `CHAMPLOAD` still queues the obsolete four-Down title selection and cannot
  serve as a release gate until that route is resolved; do not repeat it as-is.
- Audit every remaining subsystem in the coverage matrix, distinguishing
  confirmed defects from unverified coverage and user-authorized adaptations.

- Finish disk-free ordinary navigation using the startup-resident inventory in
  [menu-resident-assets.md](menu-resident-assets.md). The 57-resource cache and
  keyboard snapshot are implemented; title Help, Options, Players, Controllers,
  main Tracks and pause paths have been migrated. Evidence is separate in
  [menu-cache-verification.md](menu-cache-verification.md).
  Extend native saved-game cache coverage to mid-enumeration `ExNext` I/O
  failures and
  remaining recovery cases (including post-save backup cleanup failures).
  A real startup directory-Lock failure now has native warning, publication,
  retained-display return and exit coverage; it does not cover a partial scan.
  Creation, overwrite/delete, cancellation, read-only save failure,
  catalogue overflow, blocked/read-only deletion and pre-existing `.new`/`.bak` save obstructions have
  real-intermission coverage.
  Startup snapshots and
  explicit transaction refresh are implemented for SLICKS.TRK and saved-game
  names; external repairs currently require
  restarting, not hidden rereads on navigation. Track-information and its
  warning close and RAM-only track-list chooser input now retain hardware
  ownership. Saved-game picker-to-name, notice closes and RAM-only returns also
  retain ownership. Verify the existing cached Load-owner transition after its
  entry route is resolved; migrate remaining RAM-only owners. Championship
  results retain ownership except for their explicit on-demand cup load.
  Post-race record assets/close and recovery returns now use the cache
  and retain ownership outside explicit record I/O boundaries.
  Normal intermission and retry/end-match warning returns retain ownership;
  rerun their lifetime checks as part of the broader release gates.
  Shop and registration Help
  use the cache, with native tests recorded in the evidence document. Keep explicit
  track/exit loads and saves as disk boundaries. Extend no-I/O/no-teardown
  assertions across every owner; rerun full 2 MiB/default-stack release gates.
  Current stripped-binary title, Options-to-race, Options Help navigation,
  intermission championship save/exit and clean demo-exit gates
  plus 600-update F1/CITY/WHACKO full-frame display audits pass with a
  confirmed 4 KiB stack; the other release workflows still need
  current-build coverage (see release-verification.md).
  Startup display-allocation failure checks now cover all ten allocation
  sites, partial-create cleanup and repeat destruction on the target with a
  4 KiB stack (see release-verification.md). Both bitmap/copper allocations
  are startup-resident; demo preparation does not allocate another display.

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
