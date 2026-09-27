# Stationary track-object cache

Verified design and measurements, 2026-09-27.

Kind-1/3 track objects keep their normal car-contact checks every update.
The native loop skips only material/layer sampling and repeated actor
configuration when the previous update configured that object while it was
stationary and its velocity is still zero. A 100-byte validity array lives
beside the permanent handle array, inside the shadow-checked working state.
The reference loop always performs the original sampling and configuration;
it derives the same validity metadata but never takes the shortcut.

## Write-set and lifetime proof

- `initialize_weapon_actors` allocates cars, then permanent track handles,
  then temporary intro/notice and reserved shadow slots. It resets every
  validity byte. Full race initialization also clears the entire structure.
- `track_actor_motion.inc` / `track_motion.s` are the only race-time writers
  of kind-1/3 navigation positions, layers and velocities. The navigation
  kinds and handle associations come from setup and do not change mid-race.
- `configure_weapon_actor` establishes state 1, zero lifetime, velocity,
  acceleration, age, frame and period, plus the appropriate position,
  priority and occlusion. Initial randomized periods must be cleared by a
  real first configuration before a validity byte can be set.
- Actor advancement cannot change these configured stationary fields:
  lifetime is unlimited and all motion/animation inputs are zero. It cannot
  retire or release the permanent slot. The exact no-op advancement path
  is independently verified; the same invariant also holds without it.
- Weapon flash, explosions, impact points and projectiles configure their
  own allocated handles. The allocator only reuses free slots, not these
  permanently positive track handles. Finish-flag activation addresses
  kind 2 only. Car contacts alter navigation/car velocities, not the actor
  motion fields. The next update sees a nonzero navigation velocity and
  invalidates the cache before sampling or probing.
- Sprite restoration, drawing, visibility caching and retention write
  saved-background/geometry/render metadata, not the configured motion,
  priority, occlusion or slot ownership. Pause presentation invalidation
  likewise does not rewrite those fields. A new race resets the actor pool.
- Material/surface maps are built in `track_scene.c` before racing; the
  simulation and rendering code only reads them. Boundary animation changes
  palette/counter state rather than rewriting these maps.

## Why current zero velocity is insufficient

A moving object samples the prospective destination's material before the
ray can be blocked. Its layer may therefore describe that sampled point,
not its final position. Damping can then make velocity zero. The following
stationary update must resample the actual position. Consequently every
moving pass clears validity, even when the move is blocked, the position
does not change, or damping reaches zero. Only a subsequent fully processed
stationary pass sets validity. Retained-sprite flags alone do not establish
this condition, so they are not used as a substitute.

## Measurement trail

The first layout appended the cache after the maps. Its four-track benchmark
(`tmp/stationary-cache-20260927-*`) preserves all final states, with work /
worst lines 172455/413, 220721/549, 173917/423 and 178389/444. That version
was superseded: the cache moved into the hot, shadow-checked working state
and the reference now derives matching metadata for byte-level comparison.
These numbers must not be attributed to the final layout.

The final build also adds an explicit statistics-off audit argument because
debugger assignments did not change the live flag. This is selected by
`SLICKS_LIVE_STATS=0` with the track-actor fixture; the audit asserts the flag
at the first update and again at completion. Ordinary launches and `run.sh`
are unchanged. The original failed override runs do not establish this
coverage.

| Track | Parent work / 603 | Final work / 603 | Worst: parent -> final |
| --- | ---: | ---: | --- |
| BASIC | 172131 | 172409 | 414 -> 413 |
| F1 | 227283 | 220834 | 563 -> 550 |
| CITY | 180000 | 174249 | 436 -> 420 |
| WHACKO | 178179 | 178390 | 442 -> 445 |

Parent is 4ad3129 (`tmp/inert-advance-20260927-*`). Final logs:
`tmp/stationary-final-20260927-*`. F1/CITY total work improves 2.84%/3.20%,
about 10.7/9.5 lines per update. BASIC/WHACKO regress 0.16%/0.12%; WHACKO's
worst rises three lines and moves to update 688 (176 particles). All final
positions and mark counts match. The 312-line target remains unmet.

## Verification

- `verify-actor-advance`, `verify-actor-slots`, `verify-track-actors`,
  `verify-weapon-actors` and `verify-dirty-tracking` pass. This includes 8192
  native actor-pool cases, 40000 original DOS track-object updates and the
  rendering/dirty coverage suites. Log: `tmp/stationary-hot-host-20260927.log`.
- Shadow site 3 compares the full working state, including every cache byte,
  against the unconditional reference: 700 calls each on four main tracks
  and jump, 200 each on ice and zones, zero mismatches/errors. F1 includes
  27 moving-object probes. Logs: `tmp/shadow-stationary-20260927-*`.
- Full-frame display audits pass 600 updates each on F1, CITY and WHACKO,
  with statistics explicitly off: 32/18/5 actors, 2076/1480/1854 permanent
  marks. Logs: `tmp/audit-stationary-q-20260927-{1,2,3}.log`.
- No retention policy, drawing order, physics, particle lifetime or audio
  semantics changed. Owned emulator sessions close on exit; debug audio is
  muted. The normal build is restored after reference checks.
