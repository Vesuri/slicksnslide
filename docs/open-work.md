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
(`amiga/bench_tracks.sh hud-copy-20260927`, outer-only work lines
per 603 updates / worst update): BASIC 172347/396, F1 220839/550,
CITY 174168/403, WHACKO 178284/444. Means are 18-24 ms; the
worst updates need 21-43% cuts. Particle-heavy frames remain expensive,
but live count alone does not explain the maxima: BASIC's worst has zero
live points, CITY's 16, F1's 63 and WHACKO's 176. F1 is also slow without
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
  Reprofile BASIC update 409 (396 lines, zero live points),
  CITY 506 (403 lines, 16 points), F1 482 (550 lines, 63 points) and
  WHACKO 688 (444 lines, 176 points). Record pre/post particle counts and
  the dirty regions (BASIC geometry is recorded in the profiling evidence):
  retirement can cause work despite a low final live
  count. Use uninterrupted timings and the CIA-B sampler; debugger stops
  are only for correctness/region inspection. Include these transitions
  when evaluating C2P bounds and particle retention.


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

- **Actor ordering.** `build_actor_order` (C, ~11 lines per 100 points,
  about 69 cycles per point) and the `restore_actor_order` reversal. Check
  the compiled loop first: GCC's restore loop turned out as lean as asm. A
  native build only helps if it removes accesses, e.g. reading slot state,
  trail index and priority with fewer loads or building the draw chains as
  a by-product of the advance pass for points.

- **C2P area (~40 lines base on every track).** Dirty rectangles are
  widened to 32-pixel columns for the Kalms converter, so small car
  rectangles convert ~1.5-2x their area. Investigate how often old and new
  car rectangles are merged although far apart, and whether a 16-pixel
  column variant would win (fewer converted pixels but twice the write
  operations per converted pixel); measure before committing to either.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
