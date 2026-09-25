# Track actor and boundary verification

## Animated boundary and original assets — 2026-09-25

`make verify-animated-boundary` executes original `1fec8..1ff97` for
20,000 consecutive updates and compares level, signed timer, direction,
random seed and every palette write. The race now advances this state before
car motion, including countdown frames. The platform applies pending colours
199..203 through the framework copper list during display blanking.

`make verify-track-actor-assets` decodes the five base images and nine appended
animation frames from `SLICKS.DAT`, and compares every pixel and dimension
against the original `2e51a` loader. All fourteen match. Truncations before
the final image ends are rejected. No extracted asset bytes are committed.

The base image indices are 79, 80, 81, 82 and 89. The appended bank follows
the `12 34 00` marker after the material data; its nine frames belong to
kinds 0, 0, 0, 2, 2, 2, 4, 4 and 4 respectively. The largest logical image
has 110 pixels.

The Amiga build passes. A muted A1200, 2 MiB/no Fast RAM run has passed 4,800
updates with the integrated boundary state; this is a bounded runtime check,
not proof of race completion or the remaining actor integration.

## Shared-pool integration — 2026-09-25

The decoded assets now render from the authoritative chunky surface. Cars,
track objects, weapon objects and points share the 200-slot allocation domain.
The renderer respects signed original priorities, handle order at equal
priority, foreground masks and reverse saved-background restoration. Permanent
marks are committed after restoration, before the next actor draw.

`make verify-track-actors` additionally verifies:

- 20,000 original track-object motion/layer/wall/car-contact updates.
- All 200 retained actor-slot finish-flag style indices, including the original
  shared scratch variable, coordinates, masks, priority, animation and RNG.
- 1,792 original full-screen actor/frame/page/mask/edge comparisons, with exact
  background restoration; 64 saturated mixed-priority pool comparisons.
- Permanent-mark survival beneath all five kinds at eight priorities.
- 1,920 original off-road emission cases spanning material, speed thresholds,
  layers and zero/one/two available slots. This found and corrected dust X/Y
  RNG order and the extra lifetime RNG draw when allocation fails. The old
  hand-recorded surface-effect expectations had the same X/Y error.

The weapon firing, projectile simulation, actor allocation/animation and weapon
renderer oracles also pass with the integrated pool. The moving probe, car
probe, track response, collision burst and damage smoke original-code checks
remain green. Track motion is the newly connected moving-probe caller; the
navigation, car and projectile callers already have their own oracles.

A muted 2 MiB/no-Fast A1200 BASIC test reached natural completion at update
556, with four flag activations and 2,036 emitted effect actors, then passed normal
menu return and system restoration. An earlier run trapped after results;
that did not reproduce on this rebuilt run and is not claimed as a diagnosed
production fix. Representative-track and startup checks remain pending.

## Startup and effect handoffs — 2026-09-25

`verify_track_actor_setup` executes original `24ee4..25044` for 808 combinations
of object count (0..100), kinds and seed. It compares source-order handles,
Q4-to-pixel placement, hidden flags, appended frame counts, priority, masking,
period and RNG against full native race startup. Synthetic initial pixels
also prove restoration returns to loaded scenery rather than stale chunky
memory. Countdown checks exercise animation while car motion and particle
emission remain gated.

This exposed a platform handoff that discarded initialization RNG: native
menus now supply the seed before actor construction, retaining those draws
before initial weapon selection. Track objects are drawn in the initial grid
frame, not one update later. The scenery-to-chunky transfer occurs before
saved-under capture; initial presentation converts that authoritative surface.

Collision and damage-smoke tails also update the original aliased scratch
read by later finish flags. The original collision-burst oracle now covers
4,800 cases including zero through seven shared-pool slots; the smoke oracle
covers 1,728 cases including full and one-free-slot pools, and both compare
the scratch handoff as well as emitted tuples and RNG.

The preceding integration build passed 600 audited updates and clean system
restoration on F1 (32 track objects), CITY (18) and WHACKO (5). Together with
BASIC these exercise all five kinds. Final-build repetitions are required
after the startup handoff changes above.

## Final integrated checks — 2026-09-25

The final production code in `504dd1b` passes muted FS-UAE tests configured
as A1200/68020, 2 MiB Chip RAM and no Fast RAM. Each checks initial grid state,
shared handles, hidden flags, animation periods, every update's planar dirty
coverage, collision errors, and restoration status `0x1f` on exit.

| Track | Track objects | Checked updates | Pool high-water | Emitted effects |
| --- | ---: | ---: | ---: | ---: |
| BASIC | 2 | 556, natural completion and menu return | 198 | 2,038 |
| F1 | 32 | 600, bounded diagnostic exit | 200 | 2,076 |
| CITY | 18 | 600, bounded diagnostic exit | 165 | 1,480 |
| WHACKO | 5 | 600, bounded diagnostic exit | 155 | 1,854 |

The emitted-effect counter includes transient particles; it is not a count
of surviving permanent marks. Their survival is checked separately by the
original-renderer comparisons and overlapping-object retirement tests.
The bounded runs are not claims of race completion on those three tracks.
A second final-build BASIC run again finished at update 556, explicitly
verified all four finishers and four flag activations, and restored the
system normally after the menus. The earlier post-results trap did not
recur in either final-build natural-completion run; its cause is not claimed
to have been isolated.

All moving-probe call sites are accounted for:

| Original caller | Native path | Original-code verification |
| --- | --- | --- |
| `1b15e`, pit route visibility | Navigation/pit routing | `verify-dos-ai`, 768 full pit-routing cases |
| `20066`, movable track objects | `update_track_actor_motion` | `verify-track-actors`, 20,000 object updates |
| `2131b`, car motion | Native motion and track response | `verify-dos-damage`, synthetic and BASIC rays; `verify-drive-trajectory` |
| `21bce`, projectiles | Weapon simulation | `verify-weapon-simulation`, 4,096 loop cases |

The negative-owner wall-response callback is intentionally empty: original
`1c64b` returns immediately in this case. Actual car responses and their
subsequent collision effects are separately verified, not bypassed.

Final regressions also pass `verify-moving-probe`, `verify-car-collision`,
`verify-projectile-map`, `verify-dos-ai`, `verify-dos-damage`,
`verify-drive-physics`, `verify-drive-trajectory`, `verify-track-actors`,
`verify-animated-boundary`, `verify-surface-effects`, `verify-dirty-tracking`,
`verify-planar-writes`, `verify-actor-slots`, `verify-weapon-fire`,
`verify-weapon-actors`, and `verify-weapon-simulation`. The trajectory suite
includes nine 7,200-update scenarios plus BRIDGES and BUMPS. The planar suite
checks 1,394 write cases and 1,920 consecutive original/68020 point updates.

For target repetition, source `amiga/env.sh` and run `amiga/debug.sh` with
`amiga/diag_track_actors.gdb`, `SLICKS_TRACK_ACTOR_TEST=1` and
`SLICKS_TRACK_ACTOR_CASE=0..3` (BASIC, F1, CITY, WHACKO). Use a separate owned
`FSUAE_RUN` directory and debugger port for concurrent runs. Require both
`TRACK_ACTOR_GRID_OK` and `TRACK_ACTOR_RESTORE_OK`, plus the target/finish
marker; a debugger process exiting by itself is not a passing result.

These checks close the animated-boundary/track-actor/probing/collision-effect
completion item. Broad release regressions, source-wrap boundaries, audio
listening and deferred performance work remain separate work items.
