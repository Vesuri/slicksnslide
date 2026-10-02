# Race performance

The target is a stock PAL A1200: 68020, 2 MiB Chip RAM, no Fast RAM.
Optimization has stopped. Mean update work is below one 20 ms frame on every
benchmark track, while the slowest updates take 21–26 ms. With the real-time
race clock and adaptive publication ([frame-pacing.md](frame-pacing.md)),
races run at correct game speed and about 46–50 updates/s. A worst case of
20 ms is out of ship scope.

## Measuring

**Whole-update timing.** Run `amiga/bench_tracks.sh LABEL [DETAIL] [TRACKS]`.
It builds the game, refuses to time a stale executable, and runs the
fixed-clock benchmark race on each track (0 BASIC, 1 F1, 2 CITY, 3 WHACKO):
603 muted racing updates, from the end of the countdown to frame 700. Each
track prints `WORK_SUM`, maximum work, cadence, `LATE_PUBLICATIONS` and
`FINAL_STATE`. Work is step, HUD, audio and C2P, in raster lines (64 µs; one
PAL frame is 312 lines), excluding the sync wait.
`tools/summarize_benchmark.sh` gives percentiles.

- Always compare against a parent-commit control from the same session. A
  behaviour-preserving change must reproduce `FINAL_STATE` exactly.
- `SLICKS_HUD_PHASE=0..3` starts the HUD status clock at each of its four
  phases. Use it when HUD redraw timing could hide a worst-update regression.
- `SLICKS_REALTIME_CLOCK=1` times real play (`NATURALB<track>RT`). Its final
  state depends on timing, so it cannot serve as a fidelity gate.
- `tools/sync_policy_model.py` predicts cadence under a publication policy.

**Where the time goes.** Run `amiga/pc_profile.sh TRACK LABEL` (after
`. ./env.sh`), which is DETAIL 8 (`NATURALS<track>`): the CIA-B PC sampler in
`src/platform/amiga/pc_sampler.s`. It takes a free CIA-B timer from
`ciab.resource`, fires at about 614 Hz with a xorshift-jittered period so
samples cannot lock to the beam, and records PC, update index, frame offset
and SR from the level-6 exception frame. `diag_pc_sample.gdb` dumps the
buffer at frame 700, and the script archives the matching ELF beside it.

    tools/prof_summary.py tmp/pcprof-LABEL.bin [--inlined FUNC] [--lines FUNC] [--particles LO:HI]
    tools/particle_slope.py tmp/pcprof-LABEL.bin

The sampler adds about 12% uniform overhead: use it for shares only. Never
sample with GDB interrupts; FS-UAE services them only at vsync, which
doubles C2P's apparent share and hides car physics entirely.

**Diagnostic build flags** (`amiga/Makefile`, default 0, never released).
Mode stamps rebuild the affected objects automatically, except for SHADOW
and RETCHECK, which need a forced rebuild (see below).

| Flag | Purpose |
| --- | --- |
| `INNER_PROFILE=1` | Stage timers for `SLICKS_BENCHMARK_DETAIL` 1–7; required for those modes |
| `SHADOW=1` | Dual execution of native replacements against their C references |
| `RETCHECK=1` | Compares every racing update with a reference that keeps nothing between frames |
| `STACKCHECK=1` | Native stack watermark |
| `TITLEPROFILE=1` | Title phase timestamps |

`GAMEPLAY_CFLAGS` defaults to `-O3` for `race_runtime.o`. With `-O2` the
maximum update got worse, and `-Os` made total work 7–9% worse.

## Cost model

All code and data are in Chip RAM, and the 68020 has no data cache. A Chip
access costs about 7 cycles, a cached instruction about 2.5, and every
longword fetched from outside the 256-byte instruction cache is a Chip
access. Executed code volume, not arithmetic, is the main cost; profiles of
compiled C are flat across routines run once per car or per update.

- A literal assembly rewrite gains under 1%. The first native car-motion
  translation still ran about 700 bytes of straight-line code per tick.
- Gains come from executing less: inner loops that fit the cache, state in
  registers, hoisted invariants, and skipping provably unchanged work.
- Per-object overhead adds up. F1's 32 stationary track objects cover about
  1000 pixels, yet once cost about 30% of F1's work.

## Current shape

Round numbers from the fixed-clock benchmark and the sampler:

- Mean work about 15 ms (BASIC, CITY, WHACKO) and 18 ms (F1); p95 19–23 ms;
  maximum 21–26 ms. Over 20 ms: about 30% of F1 updates, 2–8% elsewhere.
  Real play runs more ticks on slow updates, so F1's mean is about 20 ms.
- Busy time: simulation about 30%, particles 12–18%, dirty tracking and C2P
  10–12%, cars and shadows 10%, track sprites and retention 8–20% (highest
  on F1), wheel emission 5–7%, HUD and audio about 4% each. No single routine
  exceeds about 7% of samples.
- Particle-heavy updates cost about 1.1 raster lines per live particle on
  top of about 270 lines of fixed work.

## F1 over-budget updates (2026-10-02 profile)

Release 0.90 code, fixed-clock F1 benchmark (603 updates) without the
sampler: mean 288 lines (18.4 ms), p50 283, p90 350, p95 369, p99 398,
maximum 428 (27.4 ms); 184 updates (31%) exceed one 312-line frame.
Work is about 248 lines plus 0.81 lines per live particle (r = 0.80), so
the budget is crossed at about 79 particles: 2 of 295 updates under 25
particles are over budget, but 108 of 115 with 100–149 are. Over-budget
updates cluster in the race start burst and around updates 440–520.

A matching CIA-B sampler run (same `FINAL_STATE`; 7,889 samples) was split
by each update's un-instrumented work, and each update's lines were spread
over its samples. Lines per update, over-budget (184) versus at most 260
lines (206):

| Subsystem (main routines) | > 312 | ≤ 260 | Growth |
| --- | ---: | ---: | ---: |
| Particle draw (`slicks_draw_particle` body, `draw_trail_priority`) | 32.7 | 10.7 | +22.0 |
| Particle advance (`slicks_advance_shared_particles`) | 22.8 | 2.6 | +20.2 |
| Particle emission (`slicks_emit_wheel_surface`, `add_trail_component`) | 25.3 | 11.7 | +13.7 |
| Sprite retention (`slicks_prepare_sprite_retention`, touch, rebuild) | 25.2 | 14.6 | +10.6 |
| Particle restore (`slicks_restore_point_chain`, trail restore) | 14.5 | 5.8 | +8.8 |
| Dirty bookkeeping (`slicks_mark_dirty_rect`, pixel pruning) | 15.0 | 8.3 | +6.7 |
| Actor draw order (`slicks_build_draw_order`) | 14.0 | 8.3 | +5.7 |
| Audio (engine speeds, effect starts) | 11.9 | 7.1 | +4.7 |
| C2P (`c2p16_interleaved`, sparse pixels, rectangles) | 21.4 | 18.8 | +2.6 |
| Track-object and car sprite restore + draw | 54.5 | 50.1 | +4.4 |
| Car physics and race logic (`slicks_race_step`, `slicks_integrate_car_motion`) | 49.7 | 49.4 | +0.3 |
| AI, car pairs, track objects and weapons | 28.8 | 31.1 | −2.3 |
| HUD | 10.0 | 9.3 | +0.7 |
| Main loop, sound dispatch, other | 18.1 | 12.8 | +5.3 |
| **Total** | **344** | **241** | **+103** |

- Particles (advance, emission, draw, restore) are 95 lines of an
  over-budget update and 65 of the 103-line growth; with retention, dirty
  bookkeeping and draw order, which scale with the same effects, the growth
  is 88 lines. The fixed per-update work (cars, AI, sprites, C2P, HUD) is
  about the same in fast and slow updates and is itself about 240 lines.
- Particle advance: the cost is the 24-byte record `movem` load and the
  compaction store, about 12 µs per live particle per update.
- Particle emission: the hottest instructions are `.add_scan`, the
  lowest-free slot search in `car_emission.s` (about 3 lines per update).
- The single worst update (index 409, 428 lines, 40 particles) is about
  100 lines above its neighbours because of one `slicks_retention_rebuild`
  (pairwise overlap pass plus map `memset`s) after a track sprite settled;
  it occurred once in the race. Updates 376/389/399 add weapon projectile
  sampling (`slicks_track_projectile_sample`).
- The sampler resolves about 15 samples per update, so single-update
  attributions are indicative; subsystem rows rest on about 2,700 samples
  and have roughly ±10% relative error.


Native replacements keep their C references; sites are `SHADOW_SITES` bits.

- `src/game/car_motion.s`: car motion core (force, velocity, probe ray,
  track clamp) with pre-scaled direction vectors. Site 1.
- `src/game/car_emission.s`: wheel emission, trail components, wheel sound
  and Borland RNG; particles get their final occlusion at creation. Site 2.
- `src/game/track_motion.s`: track-object motion and shared-pool actor
  advance, skipping inert actors with a direct state test. Sites 3 and 4.
- `src/game/car_collision.s`: whole-loop car-pair resolver. Site 8.
- `race_runtime.c`: exact per-car steering cache (site 7); one fused
  surface-effect switch (site 9); lazy AI velocity direction; actor scans and
  priority-bucket clears bounded by their populated range.
- Division: direct 68020 constant division, and direct C coordinate/speed
  division (`slicks_div100`, oracle `make verify-signed-div100`).
- `src/game/particle_runtime.s`: one-pass particle advance, compaction and
  handle publication. Site 6.
- `src/game/particle_draw.s`: cache-sized point drawing loop, precomputed
  visibility and bounds-proven addressing.
- `point_restore.s`, `actor_order.s` (bidirectional draw chains),
  `actor_allocate.s`: consecutive point restore, chain build, allocator.
- `src/game/sprite_retention.{s,inc}`: unchanged, isolated track sprites stay
  in the chunky surface (about 20 of 31 on F1). It uses a geometry cache
  invalidated by its producers and overlapping stationary groups
  ([rendering.md](rendering.md)).
  `retention_touch.s` is its native conflict walk.
- `src/game/track_sprite_fast.s`, `sprite_opaque.s` and
  `track_sprite_animation{,_publish}.s`: unchanged, opaque and
  changed-frame sprite painting inside the chain. Address lookups are
  direct, and empty layers are skipped without wrapper calls.
- `src/game/car_render.s`, `car_draw.s`: prepared-car drawing. Site 5.
- `dirty_rect.s` (strict-overlap merging), `dirty_prune.s` (union-bounds
  pruning), `src/platform/amiga/c2p16_interleaved.s` (single-pass 16-pixel
  C2P from Kalms c5, dirty bounds rounded to 16 pixels).
- HUD change detection, exact row repainting, direct bar dirty bounds and
  `hud_restore.s`; incremental copper palette updates; opt-in statistics.

## Rejected approaches

Rejected native routines, removed from the tree but kept in git history with
their oracles: `actor_restore.s` (one restoration chain: mixed total, maximum +23 lines),
`car_prepare.s` (whole-car preparation: worse maximum), `car_progress.s`
(clock/checkpoint/lap block: 0.03–0.4%), `particle_compact_trial.s` (20-byte
particle record: regressed when integrated), and `c2p8_interleaved.s` /
`c2p8_16_interleaved.s` (8-pixel and hybrid C2P: no gain in game).

- Wider native finishing pass: regressed every track.
- Point retention (stationary, sparse-hash, direct-cell) and the first
  overlap-aware sprite retention: bookkeeping cost more than the redraws
  saved.
- Velocity reciprocal table: slower than DIVS. Surface-limit cache/tables:
  neutral or slower. Unchanged-pixel visibility cache: 1.2–2.0% slower.
- Area-aware dirty merging, composed HUD cells, HUD covered-row shortcut:
  slower on all tracks.
- Slower, or under 0.2% with a worse maximum: stack-cached force
  coefficients, tighter particle draw chain, two-argument chain entry,
  four-byte emission slot scan, hoisted emission bound, single-rectangle
  prune, incremental car ray, deferred particle loads, reused car pixel
  coordinates, palette-reload gating (upper bound under 0.8%), shared C
  checkpoint coordinates, register-only particle conflicts.
- Double buffering, several steps per frame: [frame-pacing.md](frame-pacing.md).

## Correctness guards

Fail fast: quick oracles, then timing against the parent; only a promising
candidate earns the expensive gates and a final timing confirmation.

- **Oracles.** Every native routine has a Unicorn or host oracle that runs
  the real 68020 code against the C or DOS-derived reference, including ABI,
  stack and callee-saved registers. Examples: `make verify-dirty-rect`,
  `verify-dirty-prune`, `verify-particle-advance`, `verify-actor-advance`,
  `verify-emission-scan`, `verify-car-probe`.
- **SHADOW** (simulation changes):
  `. ./env.sh; ./shadow_check.sh LABEL [0 1 2 3 | VAR=VALUE ...]`. At each
  wrapped call it snapshots the leading 56 KB of race state (plus the
  chunky surface at render sites). It runs the C reference, restores the
  snapshot, runs the native routine and compares 1 KB block hashes; a
  mismatch records the first differing block and continues from the
  reference. `SHADOW_SITES` selects sites (default 0x3fe = all; 16 actor
  advance, 256 car pairs, 512 surface tail). It fails on a mismatch, race
  error or unexercised site; runs use warp and restore the normal build.
- **RETCHECK** (drawing-order or retention changes). Remove
  `obj/race_runtime.o` and `obj/slicks_diag.o`, build `make RETCHECK=1`, then
  run `SLICKS_SHADOW_TRACK=N ./debug.sh "" diag_retention_check.gdb` for each
  track. Every racing update first runs from a snapshot with retention off,
  then for real. It compares the chunky surfaces, particle records, geometry
  cache, immutable maps and HUD status cache; a mismatch prints
  `RETENTION_CHECK_FAILED`. Rebuild normally afterwards.
- **Display audits** (rendering changes) on F1, CITY and WHACKO:
  `SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 ./debug.sh "" diag_dirty_sprites.gdb`.

Never rebuild an ELF in use by an emulator; close every session you start
([development-verification.md](development-verification.md)).
