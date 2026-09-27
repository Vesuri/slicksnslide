# Target-side profiling and native-replacement verification

Measured evidence for the 2026-09-27 performance work. Current actionable
work stays in [open-work.md](open-work.md).

## Debugger sampling is phase-locked

FS-UAE's `barto_gdbserver` only services a GDB interrupt at vsync. Reading
`VPOSR`/`VHPOSR` in every SIGINT sample showed beam line 0 or 312 each time.
Because the race loop waits for display blank before C2P, SIGINT sampling
(the Rescue on Fractalus `diag_sample.sh` method) over-weighted C2P about
2x and attributed no samples at all to car physics. It is invalid here and the
script was not kept.

## CIA-B PC sampler

`SLICKS_BENCHMARK_DETAIL=8` selects `NATURALS<track>`: the outer-only
benchmark plus `src/platform/amiga/pc_sampler.s`. The diagnostic allocates a
CIA-B timer through `ciab.resource` (touching only a timer whose vector was
free), enables EXTER for the measured race, and reloads the running latch
with a xorshift-jittered 900..1411 E-clock period (~614 Hz) so samples cannot
lock to the beam. The handler finds the level-6 exception frame (format
`$0078`, plausible SR, even PC) on the supervisor stack and stores PC,
measured-frame index, frame offset and SR high byte. Only measured updates
1..602 are kept; a 16384-sample buffer is dumped by `diag_pc_sample.gdb` at
the frame-700 checkpoint together with per-update work/particle samples.

    cd amiga; . ./env.sh; ./pc_profile.sh TRACK LABEL
    tools/prof_summary.py tmp/pcprof-LABEL.bin [--inlined FUNC] [--lines FUNC]
        [--particles LO:HI]

`pc_profile.sh` archives the matching ELF as `tmp/pcprof-LABEL.elf`; old
sample files cannot be resolved against a rebuilt layout. The sampler adds
about 12% uniform work (F1 265255 -> 297309 lines) and finds every frame
(`missed=0`). Final car positions and marks are unchanged, so it is valid for
shares, not absolute timings. Wait-loop samples are reported, not hidden.

## Where the work goes (baseline 5fa9458 + sampler)

Shares of non-wait samples, all measured updates (`tmp/pcprof-{t0,f1,t2,t3}`):

| Function | BASIC | F1 | CITY | WHACKO |
| --- | ---: | ---: | ---: | ---: |
| `prepare_car_motion` | 11.8 | 9.7 | 11.3 | 12.5 |
| `slicks_race_step` (inlined tails, emission, particle maps) | 13.0 | 8.0 | 9.8 | 11.5 |
| C2P | 9.5 | 7.9 | 8.3 | 11.4 |
| track sprite draw chain | 0.9 | 12.7 | 7.7 | 2.0 |
| `draw_car` C setup | 5.2 | 3.4 | 4.4 | 5.3 |
| track-object motion | 0.2 | 8.0 | 8.2 | 0.1 |
| HUD status + rectangles + timers | 6.5 | 5.4 | 5.9 | 7.0 |
| particle draw/restore/advance/emission | ~16 | ~10 | ~9 | ~13 |

BASIC updates with at least 120 particles (`--particles 120:400`) spend about
40% of work in particle-related code: native point drawing 9.1%, native
advance 7.1%, `add_trail_component` 5.9%, C point restoration 4.7%, draw-order
construction 4%, plus wheel emission and dirty-list handling. F1's 32 track
objects are 9 animated 8x7 banners, 20 static 5x5 tyres, two 4x4 markers and
one hidden flag; all were stationary through frame 700. Their total drawn
area is about 1036 pixels, yet motion, advancement, restoration and drawing
cost about 30% of F1 work: per-object overhead, not pixels.

## Cost model: instruction fetch dominates

Line-level profiles of compiled C are flat: the cost is spread over whole
functions executed once per car or per frame. `draw_car`'s setup (about 2.9 KB
of spill-heavy code with 32-bit displacements into the 245 KB race structure)
costs about 3900 cycles per call for four calls per update, more than its
pixel work. With all code and data in Chip RAM, each longword of executed
code outside the 256-byte instruction cache costs a Chip-RAM access, like a
data access. Executed code volume, not arithmetic, is the main cost.

## Shadow equivalence checks for native replacements

`make SHADOW=1` (driven by `amiga/shadow_check.sh`) defines
`SLICKS_SHADOW_CHECK`. At each wrapped call the verified C reference runs on
the live state; hashes of every kilobyte of the leading 32 KB working state
(cars, counters, RNG) record its result; the snapshot is restored and the
native routine runs; hashes are compared. A mismatch captures the first
differing kilobyte from both versions and continues from the reference.
`diag_shadow_check.gdb` reports calls, mismatches, differing bytes, final
positions and track-collision counts. Runs use warp; they are correctness,
not timing, runs. A static assertion ties the window to the wrapped write set.

## Native car motion core

`src/game/car_motion.s` replaces `integrate_car_motion` (force, velocity,
scaled positions, the probe ray of `move_car_through_track`, contact latch
and track clamp). Offsets come from `src/game/race_offsets.c`, compiled by
the 68020 compiler into `amiga/obj/race_offsets.i`. Rare blocked rays call
the C `resolve_track_velocity`. When a special state or disabled sampling
makes every sample zero, the ray is skipped after the original count and
wrap checks. The C reference remains for host oracles.

Shadow checks: 2412 calls on each benchmark track, 412 on the jump, ice,
zone and road fixtures, zero mismatches, unchanged final states; BASIC's run
exercised 25 wall hits (`tmp/shadow-motion*-*.log`). Host oracles pass,
including all eleven 7200-update DOS driving scenarios
(`tmp/verify-native-motion{,-driving}.log`).

Same-layout control (reference forced with `-DSLICKS_REFERENCE_MOTION`)
versus native, work lines over 603 updates (`tmp/refmotion-*`, `tmp/motion-*`):

| Track | Reference | Native | Change |
| --- | ---: | ---: | ---: |
| BASIC | 191318 | 190122 | -0.63% |
| F1 | 265722 | 264723 | -0.38% |
| CITY | 213347 | 212029 | -0.62% |
| WHACKO | 197170 | 195285 | -0.96% |

Maxima moved 453/585/475/509 to 451/589/471/515 lines. The gain is small
because the translation still executes about 700 bytes of largely
straight-line code per physics quantum; its samples are spread evenly over
the routine, like the C it replaced. A literal translation is not enough:
denser code with register-resident state and hoisted per-call invariants is
required. Final checkpoint control for later comparisons (`tmp/ckpt1-*`):
189881 / 264529 / 211897 / 195053 work lines.

## Status-bar and timer change detection

Without weapons, `slicks_race_draw_status` now computes each active car's
three cached row ends directly and returns when they, the background and the
active mask match the cache and no dirty rectangle touches rows 187..189.
Errors, unbounded widths, weapon HUDs and any difference take the unchanged
general path. The old per-frame scan of every queued dirty pixel is replaced
by `dirty_pixel_hud`, set by C `mark_dirty_pixel` for rows at or below
`SLICKS_POINT_HEIGHT` (native point producers cannot reach them) and cleared
with the lists; it occupies existing structure padding. `draw_timers` hoists
its fixed options byte and uses register car/cache bases.

Host HUD, Arcade HUD and dirty-tracking suites pass, including the injected
strip-pixel invalidation case (`tmp/verify-status-fast.log`). Full display
audits pass on F1 (600 updates, 32 actors, 2076 marks), CITY (18 actors, 1480
marks) and WHACKO (5 actors, 1854 marks) (`tmp/audit-hudfast-{1,2,3}.log`);
BASIC completes before update 600 in that fixture. The fuel fixture's 3
status-pixel checks and the damage fixture's 4 have zero failures.

| Track | Control (`ckpt1`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 189881 | 183574 | -3.3% | 452 -> 441 |
| F1 | 264529 | 257582 | -2.6% | 588 -> 586 |
| CITY | 211897 | 205131 | -3.2% | 469 -> 471 |
| WHACKO | 195053 | 189130 | -3.0% | 514 -> 487 |

Final positions and marks are unchanged (`tmp/hudfast-*`).

The fuel (`diag_fuel.gdb`) and damage (`diag_damage_race.gdb`) race fixtures
fail on both this build and a rebuilt 5fa9458 with byte-identical output:
a finished car's lap counter reaches 6, and the fuel fixture counts no
finished cars. The status-pixel parts of both pass.

## Native wheel emission

`src/game/car_emission.s` replaces `emit_wheel_surface` for shared-pool
races, including `emit_offroad_wheel`, `add_trail_component`, the wheel sound
request and the Borland RNG. Particle records are written with one long store
for saved/permanent/occlusion/state, so each new particle receives its final
occlusion limit at creation instead of in a trailing pass. Allocation keeps
the emission cursor semantics (and calls the native lowest-slot allocator if
the cursor is zero); out-of-range wheel samples call the shared C sampler.
In heavy BASIC frames the C prologue alone (seven arguments copied to stack
slots) was 21% of `add_trail_component`.

The shadow window grew to 56 KB to cover particles, slot tables and actor
kind/saved bytes. Emission shadow checks: 2412 calls on each benchmark track
and 412 on the ice, zone and road fixtures (2412 on the jump fixture), zero
mismatches and unchanged final states (`tmp/shadow-emit-*.log`). Host surface,
track-actor and driving oracles pass (`tmp/verify-native-emission.log`).

| Track | Control (`hudfast`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 183574 | 180871 | -1.5% | 441 -> 425 |
| F1 | 257582 | 255157 | -0.9% | 586 -> 570 |
| CITY | 205131 | 203757 | -0.7% | 471 -> 468 |
| WHACKO | 189130 | 186772 | -1.2% | 487 -> 482 |

## Native track-object motion and actor advancement

`src/game/track_motion.s` replaces `update_track_actor_motion` and the
shared-pool `advance_weapon_actors`. The motion loop keeps the common
stationary path (kind test, next-position material sample, layer update,
actor configuration and contact-rectangle test) within the instruction
cache; car-pixel setup, off-map samples, moving-object rays and car contacts
are out of line. Moving objects call `slicks_track_actor_probe`, a C wrapper
around the unchanged `slicks_moving_probe`. The actor loop skips point slots
with one word test and applies `slicks_actor_advance` exactly.

C callers can tail-call these entries (`bra.l`), which produced an
`R_68K_PC32` cross-section relocation that `elf2hunk` rejects. As with
`memory.s`, the native motion, emission and track modules now use the
compiler's `.text` section; the Amiga Makefile sets `.DELETE_ON_ERROR` so a
failed conversion cannot leave an empty executable that looks current.

Shadow checks: 700 motion and 700 advancement calls on each benchmark track
(countdown included), zero mismatches, unchanged final states; F1 exercised
27 moving-object rays after a car contact (`tmp/shadow-track-*.log`,
`tmp/shadow-advance-*.log`). Host track-actor, weapon-actor and dirty
suites pass (`tmp/verify-native-track-motion.log`).

| Track | Control (`emit`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 180871 | 181030 | +0.1% | 425 -> 426 |
| F1 | 255157 | 246131 | -3.5% | 570 -> 556 |
| CITY | 203757 | 196391 | -3.6% | 468 -> 462 |
| WHACKO | 186772 | 186763 | 0.0% | 482 -> 482 |

BASIC and WHACKO have one and five track objects respectively.

## Native dirty rectangles and prepared car drawing

`src/game/dirty_rect.s` replaces `mark_dirty_rect` (clip, widen to
32-pixel columns, fold overlapping or touching rectangles, full-list
fallback). `make verify-dirty-rect` runs the real 68020 code in Unicorn
against the C body over 240000 randomized calls (19904 merges, 587
full-list fallbacks), comparing every list entry, the count, the stack and
callee-saved registers. Offsets come from the target compiler
(`build/offsets/race_offsets.i`).

`src/game/car_render.s` replaces the prepared-cache part of `draw_car`.
All eligibility tests (bounds, cache identity, frame size, tile occlusion
maxima, masked-style limit) precede side effects; an ineligible car
returns to the unchanged C renderer. Rendering shadow checks also snapshot
and compare the chunky surface: 2804 car draws on each of BASIC and F1,
zero mismatches (`tmp/shadow-render-{0,1}.log`). The F1 display audit
passes 600 updates, 32 actors and 2076 marks (`tmp/audit-cardraw-1.log`);
host dirty, HUD and car-draw suites pass (`tmp/verify-native-car-render.log`).

| Track | Control (`tmotion`) | Candidate | Change | p99 ms | Max lines |
| --- | ---: | ---: | ---: | --- | --- |
| BASIC | 181030 | 176112 | -2.7% | | 426 -> 419 |
| F1 | 246131 | 242290 | -1.6% | 34.23 -> 33.85 | 556 -> 564 |
| CITY | 196391 | 192284 | -2.1% | | 462 -> 454 |
| WHACKO | 186763 | 181708 | -2.7% | 29.62 -> 29.17 | 482 -> 496 |

All F1/WHACKO percentiles through p99 improve; single maxima rose slightly.

## Upper bound for unchanged-sprite retention

A throwaway build that skipped every track sprite in the native restore and
draw chains (visually wrong) reduced F1 work from 246131 to 187782 lines
(-23.7%, maximum 556 -> 458) and CITY from 196391 to 170985 (-12.9%,
462 -> 416) (`tmp/noskip-ub-{1,2}.log`). This exceeds the sampled chain
shares because it also removes chain switching and conversions. Exact
retention of unchanged, untouched sprites is therefore the main F1 lever.
