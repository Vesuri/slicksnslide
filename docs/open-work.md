# Open work

Updated 2026-09-27. Actionable open and deferred work only.

Publish/push only when explicitly requested.

## Performance

Goal: at most 20 ms (312 raster lines) per update on a stock PAL A1200
(68020, 2 MiB Chip RAM, no Fast RAM) in general gameplay, including
particle-heavy frames, with identical behaviour, effects, permanent marks,
audio and rendering order. Status after native sparse pruning
(`amiga/bench_tracks.sh resume-nativeprune-20260927`, outer-only work lines
per 603 updates / worst update): BASIC 172561/414, F1 228718/563,
CITY 181515/432, WHACKO 178931/468. Means are 18-24 ms; the
worst updates need 25-45% cuts. BASIC, CITY and WHACKO worst updates are
particle-heavy (130-170 live points, about 1.1 lines per point on top of a
~270-line base); F1 is slow even without points (~150 lines per update of
track-sprite work). Method, tools and the cost model (Chip data access ~7
cycles, cached instruction ~2.5, uncached code fetched from Chip) are in
`docs/performance-profiling.md`. Every change: `amiga/bench_tracks.sh`
against a control of the parent commit (FINAL_STATE must be identical),
Unicorn/host oracle for any native routine, `amiga/shadow_check.sh` for
simulation sites, display audits for rendering changes
(`SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 ./debug.sh ""
diag_dirty_sprites.gdb` for F1/CITY/WHACKO) and the retention check
(`make RETCHECK=1`, `diag_retention_check.gdb`) when drawing order or
retention is touched.

Candidate fixes, roughly in order of expected value per effort:

- **Reload the copper palette only when colours change.** The race view's
  copper list (`build_copper` in `src/platform/amiga/amiga_platform.cpp`,
  `COPPER_LONGS` 558) spends 528 moves on all 256 AGA colours (two nibble
  halves plus BPLCON3 bank switches) every frame, although colour
  registers persist; the only in-race palette update found is colours
  199..203 (`slicks_amiga_platform_update_palette` from the boundary
  animation in `slicks_diag.c`, applied after `wait_display_blank`; confirm
  no other in-race caller). By estimate the block occupies the copper for
  ~9 lines per frame, contending with Chip-only CPU accesses (unmeasured). First a throwaway measurement: overwrite the first three longs
  of the palette block (`palette_at`) with MOVE COP2LCH/COP2LCL (0x084/0x086)
  = address of the list's final WAIT and MOVE COPJMP2 (0x08A), colours
  wrong, and benchmark. If it is worth ~1%, implement exactly: keep the
  jump by default; `update_palette` (and `set_view`) write the colour words
  as now and restore the three original longs so the next frame executes
  the whole block; after one vertical blank has passed with the block
  enabled (checked in the main loop while the beam is in the display blank,
  never while the copper runs the block), put the jump back. Colour changes
  then take effect on exactly the same frame as today. Verify with a
  palette-change fixture (boundary animation frames) that the copper list
  words and the displayed colours match the current build frame by frame.

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

- **F1/CITY: stop rewriting stationary track objects every update.**
  `slicks_update_track_actor_motion` (`src/game/track_motion.s`, reference
  in `track_actor_motion.inc`) samples the material map, sets the layer and
  calls the configure step (slot state, motion x/y, vx/ax/lifetime/frame,
  priority, occlusion: ~12 writes) for every kind-1/3 object each update,
  even when its velocity is zero, then tests car contacts. About 20
  lines/update on F1 (~600 cycles per object). The material maps are
  immutable during a race, so a stationary object's sample and layer
  cannot change; the configure writes are idempotent unless something else
  wrote those actor fields since the last update. Plan: list every writer of
  a track-object handle's slot state and motion fields (advance, weapon
  hits/explosions, retention, pause handoff); if none can change them for a
  stationary object, keep a per-object key (x, y, layer, kind) and skip the
  sample and the configure writes when the key and velocity are unchanged,
  keeping the car-contact test every update. Similarly check whether
  `slicks_advance_weapon_actors` (~14 lines on F1) is a provable no-op for
  zero-motion, single-frame, unlimited-lifetime objects and skip them.
  Shadow sites 3/4 verify; also run the jump, ice and zone fixtures.

- **Emission slot scan.** `.add_scan` in `src/game/car_emission.s` walks
  slot-state bytes one at a time from `emission_slot_cursor` to the first
  free slot (~8% of emission samples; up to ~200 bytes when the pool is
  full of points). Scan four state bytes per longword with the zero-byte
  test ((x - $01010101) & ~x & $80808080), then locate the byte, handling
  alignment and the high-water end exactly as now, so the allocation order
  is unchanged. `make verify-actor-slots` plus shadow site 2 verify.

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

- **Car integration.** `slicks_integrate_car_motion` (~25 lines) runs a
  ~700-byte quantum loop (larger than the 256-byte I-cache) and a `divs.w`
  per ray step in the `PROBE` macro. Replace the per-step division with an
  exact incremental quotient/remainder (truncating signed division
  semantics, including negative deltas) and hoist quantum-invariant values;
  verify with shadow site 1 and the jump/ice/zone fixtures.

- **C2P area (~40 lines base on every track).** Dirty rectangles are
  widened to 32-pixel columns for the Kalms converter, so small car
  rectangles convert ~1.5-2x their area. Investigate how often old and new
  car rectangles are merged although far apart, and whether a 16-pixel
  column variant would win (fewer converted pixels but twice the write
  operations per converted pixel); measure before committing to either.

- **Decision for the user: diagnostic bookkeeping in the timed work.**
  `main()` in `slicks_diag.c` spends ~13 lines per update on per-rectangle
  and per-frame diagnostic statistics inside the measured intervals. If the
  shipped build is meant to be SlicksDiag, trim it; if not, the benchmark
  overstates shipped cost and could exclude it.

## Verification debt

- `diag_fuel.gdb` and `diag_damage_race.gdb` fail identically on 5fa9458 and
  later builds (finished car reaches lap 6; fuel fixture sees no finishers).
  Decide whether the fixtures or the finish/lap behaviour are stale.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
