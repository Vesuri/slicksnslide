# Open work

Updated 2026-09-28. Actionable open and deferred work only.

Publish/push only when explicitly requested. Scope: finish the performance
handoff, resolving each proposed optimization with verified implementation
or measured rejection, plus its verification debt. The 50 FPS goal remains
open until target measurements meet it; closing experiments is not enough.

## Performance

Goal: at most 20 ms (312 raster lines) per update on a stock PAL A1200
(68020, 2 MiB Chip RAM, no Fast RAM) in general gameplay, including
particle-heavy frames, with identical behaviour, effects, permanent marks,
audio and rendering order. Latest benchmark
(`amiga/bench_tracks.sh native-car-pairs-20260928`, outer-only work lines
per 603 updates / worst update): BASIC 147023/347, F1 172830/416,
CITY 143524/333, WHACKO 150951/374. Means are 15.3-18.4 ms; the
worst updates in that run need 6-25% cuts. The latest matched F1 HUD-clock
phase sweep still reaches 429 lines (27.5 ms, 27% cut needed), with matching
final states across all four phases. These are
sampled maxima, not exhaustive upper bounds for every gameplay situation.
Particle-heavy frames remain expensive, but live count alone does not
explain the maxima. The latest F1/WHACKO CPU captures are
`tmp/pcprof-post-div100-{f1,whacko}-20260928`, with the accepted visibility,
inverse-chain and division changes present, but before native car-pair
integration. Refresh those captures before claiming current stage costs.
Their sampler overhead is not part of
the acceptance numbers above.
F1 is also slow without points. Detailed inner profiling now requires
`INNER_PROFILE=1`; the benchmark runner selects this for detail levels 1..7.
Method, tools and the cost model (Chip data access ~7
cycles, cached instruction ~2.5, uncached code fetched from Chip) are in
`docs/performance-profiling.md`. Every change: `amiga/bench_tracks.sh`
against a control of the parent commit (FINAL_STATE must be identical),
Unicorn/host oracle for any native routine, `amiga/shadow_check.sh` for
simulation sites, display audits for rendering changes
(`SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 ./debug.sh ""
diag_dirty_sprites.gdb` for F1/CITY/WHACKO) and the retention check
(`make RETCHECK=1`, `diag_retention_check.gdb`) when drawing order or
retention is touched.

Candidate fixes, roughly in order of expected value per effort:

- **Resolve worst-update transitions, not just particle-count averages.**
  Use the post-div100 F1 and WHACKO CPU captures for further changes.
  Keep WHACKO 685's latest regression in scope. Older archived captures
  predate recent visibility/order/division work; do not present their stage
  percentages as current. Recent small changes alter
  which updates incur the largest work.
  The paired update-613 capture establishes a different fuel-blink phase;
  measure its cost separately rather than tuning cadence. Raster contention
  remains an unproven additional explanation. Keep F1 551 and 613 in scope.
  Use the correctly indexed CPU profiles in `tmp/pcprof-indexed-{0,f1,2,3}-20260927`
  as the pre-group baseline; refreshed post-group F1/WHACKO captures are
  `tmp/pcprof-postgroups-{f1,whacko}-20260927`. Target BASIC updates 209/219,
  CITY 146/506, F1 481/613 and WHACKO 685. Retain F1 507
  (group-rebuild regression probe), CITY 700,
  F1 551 and BASIC 409/F1 482/WHACKO 688 as regression probes.
  Pre/post particle counts and dirty regions for all four are recorded in
  the profiling evidence. Retirement and compaction can cause work despite
  a low final live count. Narrow windows have few samples: use them to
  locate groups, not to claim precise single-frame percentages.
  Use uninterrupted timings and the CIA-B sampler; debugger stops are
  only for correctness/region inspection. Keep these transitions in the
  regression set when evaluating further changes.

- **Remaining F1 sprite overhead.** Use the latest CPU profiles.
  A register-only shared-cell particle scan replacing the C call is measured
  effectively neutral (<0.04% on F1/CITY), archived rather than accepted.
  Do not repeat that narrow boundary change; any native redesign must cover
  more executed work or reduce the candidate scan itself.
  Whole-list reuse is screened out as a worst-update fix: every current
  worst update changes membership/priorities, and almost no >=100-actor update
  is unchanged. Do not implement an unchanged-list cache on average eligibility
  alone. Incremental chain maintenance is a separate possible design, requiring
  allocation/retirement/priority writer coverage, the initial saved-actor
  fallback and a measured break-even against the simple builder. It is not
  yet justified; busy updates change 17-27 handles on average. The read-only
  screen and evidence are in `docs/performance-profiling.md`.
  Inspect conflict processing, final
  keep/restore decisions, draw-packet validation and group rebuild/late
  restoration at the worst updates. Preserve atomic group retention and
  exact reverse restoration order; prove any removed check redundant.
  Archived post-group profiles predate the geometry cache and must not be
  presented as current stage costs. Keep the independent geometry-cache
  audit enabled in RETCHECK when changing any source-field writer.
  Focus further wrapper work on eliminating repeated sprite validation or
  whole traversals, not merely moving argument setup across the call.
  Do not repeat the shared-cell early-return micro-change in
  `slicks_retention_touch`: matched repeats show more total work on F1/CITY.
  Its full-state reference comparison remains in `verify-retention-groups`.
  Preserve legacy traversal and sprite/overflow boundaries; any removed
  checks need explicit invariant coverage in the native oracle.

- **Exact retention of unmoved particles (design needed, larger).**
  Do not repeat the direct-cell C policy: after fixing reserved-slot
  eligibility, its actual conflict processing costs more than the saved
  redraws. Earlier direct-grid and sparse-hash timing runs had an erroneous
  bypass and cannot measure active retention. Any further design must
  remove whole scans/late-release traversals or establish a substantially
  lower bookkeeping cost before another implementation. A fused native
  register-oriented path is distinct from this measured C design.
  Most
  points stay on the same pixel for several updates (velocities are at
  most 11/64 pixel per tick). Restoring and redrawing such a point is a
  no-op when (a) its pixel, visibility and occlusion are unchanged, (b) no
  actor drawn before it in the current order (lower priority, or earlier
  in its own priority chain, plus shadows/cars/sprites below it) is
  restored or drawn over that pixel, and (c) the background under it is not
  rewritten (permanent-mark baking in the advance pass, which runs between
  restore and draw). Actors drawn after it are harmless: they save and
  restore its colour. A design mirroring sprite retention: keep a per-point
  "kept" bit; before the restore pass, mark a coarse cell grid (e.g. 8x8)
  with the minimum priority of every actor that will be restored or drawn
  this update (car rectangles, non-kept sprites, moved/new points) and with
  every baking pixel; a point is kept only if its cell's minimum priority is
  above its own (or its cell is unmarked); kept points are skipped by both
  chains but still advanced; baking onto a kept point's pixel must instead
  update its saved_under (or cancel the keep). Priority-0 static marks
  (long-lived off-road marks) are the easiest first case because only other
  priority-0 points and baking can precede them. Verify with the RETCHECK
  pattern (reference update without retention on a snapshot, compare
  chunky and state every update) plus display audits. Only worthwhile if
  the grid maintenance costs well under the ~250-350 cycles saved per kept
  point. Use `tools/point_retention_screen.py` and its read-only capture
  script to screen current expensive updates before choosing cell size.
  Prefer sparse clearing/generation tags or an exact sparse pixel index;
  include shadow bounds and baking conflicts. Require measured overhead
  below the saved-point budget and adaptive bypass on low-yield updates.
  Designs, counts and limitations are in `docs/point-retention-design.md`;
  the offline screen is not a runtime proof. Any further attempt must reduce
  bookkeeping and bypass-path overhead substantially; do not simply repeat
  the archived sparse-hash prototype.

- **Remaining C inside `slicks_race_step`.**
  Preserve the native car-pair loop and its site-8 diagnostic comparison
  (`SHADOW_SITES=256`). Keep F1 phase 3/update 613 as a regression probe:
  reduced total work did not improve the four-phase worst-update envelope.
  Refresh and split the
  samples with `tools/prof_summary.py --inlined slicks_race_step`:
  per-car `prepare_car_motion`, `update_actor_layer`, `finish_car_update`,
  `apply_throttle`, `display_time_centiseconds`,
  checkpoint and clock code. Per the cost model, only rewrite code whose
  executed volume can shrink (hoist per-update invariants out of the
  per-car/per-tick loops, drop repeated large-offset loads, fuse the per-car
  tail into one pass over the car record). After algorithmic candidates,
  evaluate coherent hand-written assembly sections that keep car/state
  pointers and intermediate values in registers across helper boundaries,
  avoid stacked parameters and repeated saves/restores, and combine byte
  accesses where valid. Small isolated assembly replacements retaining the
  original C call structure do not settle the value of that larger design.
  Investigate
  repeated signed X/Y-to-pixel divisions across helper calls, caching only
  if measured savings outweigh exact input-key checks. Verify via a shadow
  site per replaced function.
  The C-only shared-coordinate checkpoint/layer/lap block is measured
  neutral/slightly slower and archived; do not repeat it unchanged.
  Reuse coordinates as part of a broader register-resident native block,
  preserving checkpoint word wrapping versus lap full-long bounds and
  refreshing after callbacks that may mutate state.
  Do not repeat the measured per-quantum reciprocal-table velocity division:
  it increases work on all four tracks. Its isolated arithmetic proof and
  expanded native integration coverage remain available, but production uses
  DIVS. A different design needs a concrete reduction in lookup/instruction
  overhead before another target experiment.

- **Further C2P area reduction.**
  An isolated eight-pixel native converter now exists as
  `src/platform/amiga/c2p8_interleaved.s`, with `make verify-c2p8` covering
  final-only byte stores, complete pixels, bounds, empty inputs and ABI.
  The isolated hybrid `c2p8_16_interleaved.s` now uses eight-pixel edges
  around sixteen-pixel interiors, with an unchanged-converter fast path
  for already aligned rectangles. `make verify-c2p-hybrid` passes all
  820 valid horizontal spans plus randomized, single-bit and empty cases.
  Neither candidate is linked into gameplay. The complete eight-pixel
  publication/hybrid integration is measured slower on every track and
  reverted; do not repeat it unchanged. The isolated converters and finer
  independent coverage oracle remain available. A different C2P design
  must reduce split-call/byte-store overhead, not merely converted area.
  The production converter handles
  16-pixel columns. Lower priority than simulation/particle work: even the
  full measured C2P phase is smaller than the excess budget on each current
  worst update. Four current publication replays show little merge inflation;
  do not repeat an area-aware merge experiment without new evidence. An
  8-pixel-aligned converter reduces BASIC/CITY/WHACKO area, but the measured
  hybrid integration does not reduce total work. Use
  `diag_dirty_publications.gdb` and `tools/dirty_publications.py --policy strict`
  on newly identified maxima or different phases to distinguish necessary
  changed area from merge inflation before designing another policy.
  Include initial rectangles and platform-side publications, not only
  calls inside the race step. Keep update 481 as a regression probe.
  Avoid another area-arithmetic merge policy without new evidence.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
