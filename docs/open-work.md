# Open work

Updated 2026-09-27. Actionable open and deferred work only.

Publish/push only when explicitly requested. Scope: finish the performance
handoff, resolving each proposed optimization with verified implementation
or measured rejection, plus its verification debt. The 50 FPS goal remains
open until target measurements meet it; closing experiments is not enough.

## Performance

Goal: at most 20 ms (312 raster lines) per update on a stock PAL A1200
(68020, 2 MiB Chip RAM, no Fast RAM) in general gameplay, including
particle-heavy frames, with identical behaviour, effects, permanent marks,
audio and rendering order. Latest benchmark
(`amiga/bench_tracks.sh groups-selective-20260927`, outer-only work lines
per 603 updates / worst update): BASIC 159509/370, F1 193977/473,
CITY 159977/368, WHACKO 163236/423. Means are 17-21 ms; the
worst updates need 15-34% cuts. Particle-heavy frames remain expensive,
but live count alone does not explain the maxima. F1 is also slow without
points; the fresh F1 profile still shows significant sprite rendering and
retention overhead, with track-object motion down to 1.8% of non-wait samples.
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
  Use the correctly indexed CPU profiles in `tmp/pcprof-indexed-{0,f1,2,3}-20260927`
  as the pre-group baseline. Target BASIC updates 209/219 (maximum 370
  lines), CITY 506 (368 lines), F1 613 (473 lines) and WHACKO 685
  (423 lines). Retain F1 507 (group-rebuild regression probe), CITY 700,
  F1 551 and BASIC 409/F1 482/WHACKO 688 as regression probes.
  Pre/post particle counts and dirty regions for all four are recorded in
  the profiling evidence. Retirement and compaction can cause work despite
  a low final live count. Narrow windows have few samples: use them to
  locate groups, not to claim precise single-frame percentages.
  Use uninterrupted timings and the CIA-B sampler; debugger stops are
  only for correctness/region inspection. Keep these transitions in the
  regression set when evaluating further changes.

- **Remaining F1 sprite overhead.** Group retention now avoids repeated
  rendering of overlapping stationary track sprites, but its worst updates
  remain well over budget. Inspect repeated kept-sprite metadata writes,
  validated draw-packet checks and rebuild/late-restoration costs; prove any
  removed check redundant. Preserve atomic group retention and exact reverse
  restoration order. Refresh profiles before attributing the remaining costs
  using the older isolated-sprite baseline.

- **Exact retention of unmoved particles (design needed, larger).** Most
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
  point.

- **Remaining C inside `slicks_race_step` (~37 lines base).** Split the
  samples with `tools/prof_summary.py --inlined slicks_race_step`:
  per-car `prepare_car_motion`, `update_actor_layer`, `finish_car_update`,
  `steering_delta`, `apply_throttle`, `display_time_centiseconds`,
  checkpoint and clock code. Per the cost model, only rewrite code whose
  executed volume can shrink (hoist per-update invariants out of the
  per-car/per-tick loops, drop repeated large-offset loads, fuse the per-car
  tail into one pass over the car record); do not transliterate. Verify via
  a shadow site per replaced function.

- **Further C2P area reduction.** The rectangle converter now handles
  16-pixel columns. Investigate how often old and new car rectangles
  merge into excessive areas, especially at the worst-update transitions;
  measure alternative merge policies without losing dirty coverage.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
