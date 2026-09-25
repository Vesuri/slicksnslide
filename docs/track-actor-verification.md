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
- All 200 possible aliased finish-flag style indices, including the original
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
556, with four flag activations and 2,036 emitted marks, then passed normal
menu return and system restoration. An earlier run trapped after results;
that did not reproduce on this rebuilt run and is not claimed as a diagnosed
production fix. Representative-track and startup checks remain pending.
