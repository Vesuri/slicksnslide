# Open work

Updated 2026-09-30. Actionable open and deferred work only.
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

## Menu loading, presentation and fidelity

The requirements below remain open. Use [menu-cache-verification.md](menu-cache-verification.md)
and [fidelity-audit.md](fidelity-audit.md) for completed evidence and its exact
scope; do not repeat a completed matrix merely because another route is open.
Source inventories and isolated painter tests do not substitute for remaining
native caller/input/error checks.

### 1. Integrate the original loading presentation

- Verify visible scanout during the existing diagnostic `begin_io`/`end_io`
  handoff before using it for production loading. Unchanged bitmap bytes and a
  null OS View are insufficient. The pending visual checkpoint is
  `diag_loading_io_visual.gdb`, after 50 OS-serviced refreshes; capture limitations
  and the unanswered manual check are recorded in the fidelity evidence.
- Connect the original loading painter and filename/suffix builder to race/demo
  preparation, preserving the actual track stem, caller font alias and display
  lifetime. `verify-loading-pixels` verifies the isolated painter, not integration.
  Audit alias lifetime after startup and later transitions.
- Keep the custom image visible while real disk I/O is serviced; drawing it
  before the existing full platform teardown does not meet this requirement.
  Verify preparation failures, retry, cleanup and final system restoration.

### 2. Finish menu publication and resident-navigation coverage

- For every menu owner and remaining input/failure route, verify producer dirty
  bounds and native pixels for selection, scrolling, nested dialogs, restoration
  and transitions. Preserve both X and Y bounds, merge overlaps, and C2P only
  covered aligned blocks. Retain full initialization when the destination view
  is stale or the whole screen is replaced.
- Complete remaining Help owners/page/language/error routes and owner-specific
  navigation outside the documented matrices. Check remaining callers of the
  shared glyph-bound text adapters and all six lifetime-tracked restore callers;
  the source inventories themselves are complete.
- Complete unverified allocation lifetimes and warning layouts in saved-game,
  setup, intermission and other results owners. Records read/view preparation,
  repeated view retries and its writer-buffer allocation already have separate
  completed matrices. Do not infer other owners from those checks.
- Verify sparse shop participation after decision D2 below. Preserve all ordinary
  selection/transaction semantics; do not reintroduce blanket background restores
  or redraws for ignored keys and rejected transactions.
- Extend no-I/O/no-display-teardown assertions across every RAM-only owner, and
  fix any offending routes found. The explicit platform-end call-site inventory
  is complete after the Clear Records fix; remaining work here is native route
  coverage, not repeating that source inventory. Use the startup-resident inventory
  in [menu-resident-assets.md](menu-resident-assets.md). Keep actual track/exit
  image loads, cup loads and saves as explicit disk boundaries.
- Verify cached Load-owner transitions and recovery after D1 is resolved.
  SLICKS.TRK and saved-name snapshots refresh only at explicit transactions;
  external repairs require restart, not hidden navigation-time rereads.

### 3. Finish title and demo fidelity checks

- F08: compare remaining title animation cadence with the original, whose rate
  is CPU-dependent. Do not invent a universal DOS rate. Measure title-pulse cost
  and missed refreshes, and cover remaining registration/language combinations
  outside the complete-cycle evidence. This is menu work, not permission to
  resume paused gameplay optimization.
  The current Finnish normal/Arcade registered/unregistered cadence refresh
  and phase attribution are recorded in the fidelity evidence. Glyph destination
  reuse improves the current registered Finnish normal cycle to 72 refreshes
  for 64 intervals. Exact Arcade palette-result reuse reduces its steady
  registered Finnish interval to 22.54 ms, still missing refreshes; its
  initial redraw is measured separately. Remaining colour/state and owner
  drawing work need further attribution/reuse assessment. Keep
  instrumentation overhead distinct from normal cadence and preserve original
  palette/state semantics; the current result does not close F08.
- F03–F05/F10/F12: finish remaining native interactive sequences and shortcuts
  against original callers, including remaining Help/demo/input routes and
  applicable mouse behavior. GO across all saved modes and F9 from every
  visible row in all six modes have completed native matrices; do not repeat
  those as remaining row coverage.
  The F2/F1 Help sequence also passes every visible row in unregistered
  Finnish modes 0 and 5, with exact parent restoration and resident publication;
  other mode/language/error routes remain separate coverage.
  Do not add an F2 title action or revive removed native-only menu rows.
- Finish remaining caller/font-alias lifetime checks for other dialogs,
  transitions, failure and language routes. Ordinary Options/Players/Tracks
  and Arcade Settings nested-Help title returns are covered in the evidence.
  Encoded font-loader references alone do not prove computed/indirect original
  paths; keep that limitation explicit.
- Extend demo return/persistence/error presentation coverage where it remains
  unverified. Construct reference title pixels from DS:4c1c's actual producer:
  the prepared startup background, not a snapshot taken immediately before a demo.
- Preserve finish-event rewards separately from later skipped track/results
  branches; never rewind the shared RNG. Existing demo entry/views/returns do not
  establish every persistence/error route.
- Once D1 provides a valid synchronous Load dialog route, verify that a long
  picker/notice visit followed by cancellation restarts the full idle interval.

### 4. Finish saved-game entry and recovery verification

- Resolve D1 before changing Load Game reachability. Recheck remaining load/edit,
  rejection and recovery routes without inventing visible title/intermission rows.
- Replace `CHAMPLOAD`'s obsolete four-Down title selection only after a valid
  entry exists; it is not a usable release gate in its present form.
- Extend native saved-game recovery/load coverage and the cached-owner checks
  in item 2. Preserve genuine intermission Save entry, cancellation/repeated
  editing, transactional file safety and retained-display returns.
- Keep static reachability evidence distinct from unproven computed targets or
  runtime patches; a reference scan alone does not establish another Load entry.

### 5. Finish localization boundaries and callers

- F15: resolve D3 and implement the negative-selector automatic default policy.
  Do not guess a host-locale mapping for DOS KEYB.
- Audit remaining label callers and native owner/language/input/error lifetimes.
  Positive saved selections, all-language painter comparisons and the direct
  language-wrapper inventory do not establish every live caller.
- Preserve the zero-selector chooser's original labels, clamped navigation and
  acceptance behavior, save/restart persistence and non-interactive rejection.
  Keep controlled API-fault checks distinct from actual console-handler recovery;
  finish any remaining real error/cleanup routes not covered by the evidence.

### 6. Complete the whole-port fidelity audit

- Review every remaining subsystem in the coverage matrix/discrepancy ledger in
  [fidelity-audit.md](fidelity-audit.md), including production callers, parameters
  and overrides, not only isolated translated helpers.
- Classify each finding as a confirmed defect, an unverified requirement or a
  user-authorized adaptation. Fix confirmed differences against the original DOS
  execution and verify the real call path.
- Keep feature completeness unclaimed until remaining coverage is established.
  Completed narrow fixtures must not be used as whole-menu or whole-port proof.

### 7. Refresh final release gates and package

- After production fixes settle, rerun the current stripped executable on a stock
  A1200 with 2 MiB Chip/no Fast and the default 4 KiB stack: startup, native menu/
  Help and return paths, race entry, championship save/exit, demo lifecycle,
  affected recovery/lifetime gates and normal system restoration.
- Refresh F1/CITY/WHACKO full-frame display audits on the release candidate.
  Apply the verification rules below for any rendering/order changes.
- Rerun Installer, Keep/WHDLoad, Reinstall, package-content audit and installed
  Play startup. Verify WHDLoad startup/idle demo, racing with and without PRELOAD,
  and normal exit at its documented configuration; the prior WHDLoad evidence
  used 4 MiB Fast RAM and is not a stock-2-MiB claim.
- Use [release-verification.md](release-verification.md) for the full workflow
  and [install-original-data.md](install-original-data.md) for installation.
  Repack the installer only after the final applicable gates pass. Historical
  runs and the existing dist archive are not a current release freeze.
- Keep private keys and original executable/data/captures out of the repository
  and installer package.

## Decisions awaiting the user

- **D1 — Load Game entry:** the supplied original hides the title row and the
  current reachability audit has found no other usable entry. Choose preserving
  that original limitation or exposing Load as an explicit Amiga adaptation.
  Implementation and load/rejection/idle-return tests depend on this.
- **D2 — Sparse shop participation:** original execution can select an invalid
  packed column and return an uninitialized driver byte. Choose retaining the
  safe active-player mapping or rejecting sparse setups. Do not reproduce
  unsafe indexing. Native sparse-participation coverage follows the decision.
- **D3 — Automatic language:** choose an Amiga policy for the original negative
  selector/DOS KEYB boundary (English default versus the explicit chooser).
  Positive saved selections and zero-selector behavior must remain intact.

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
