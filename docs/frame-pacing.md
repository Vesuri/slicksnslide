# Race frame pacing and the 20 ms target (2026-09-30)

Measured basis for the frame-pacing items in [open-work.md](open-work.md).
Build: HEAD `538c6f5` plus the uncommitted Load-storage draft (menu-only; no
gameplay code). Stock PAL A1200, 68020, 2 MiB Chip, no Fast RAM, 603 racing
updates per track. Logs are local-only: `tmp/ship-audit-20260930-{0..3}.log`
(`amiga/bench_tracks.sh`, DETAIL 0) and
`tmp/pcprof-ship-audit-t{0..3}-20260930.{bin,elf,log}` (`amiga/pc_profile.sh`).

## Update cost

One raster line is 64 µs, and one PAL frame is 312 lines (20 ms).
"Work" is step, HUD, audio and C2P, and excludes the sync wait
(`tools/summarize_benchmark.sh`).

| Track | Mean | p50 | p90 | p95 | p99 | Max | Over 20 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| BASIC | 15.3 | 15.3 | 18.8 | 19.7 | 21.0 | 22.3 | 22 (3.6%) |
| F1 | 18.3 | 18.0 | 22.1 | 23.3 | 25.0 | 26.0 | 186 (30.8%) |
| CITY | 14.9 | 14.5 | 18.1 | 19.2 | 20.2 | 21.4 | 9 (1.5%) |
| WHACKO | 15.7 | 15.1 | 19.8 | 21.0 | 22.6 | 24.9 | 50 (8.3%) |

The mean is under 20 ms on every track. Meeting a worst case of 20 ms
would need a 24% cut on F1 and 4–20% on the other tracks.

Since 2026-09-26, dozens of accepted and rejected experiments have each moved
work by 0.03–1% (see [performance-profiling.md](performance-profiling.md)).
No remaining candidate is known to deliver double-digit percentages.

## Where the time goes

This is the target CIA-B PC sampler. Its roughly 12% uniform overhead makes it
good for shares only. "Wait" is time spent in
`slicks_amiga_platform_wait_display_end`. The other columns split the
remaining busy time, grouping routines by name.

| Track | Wait (all / over-budget updates) | Simulation | Wheel emission | Particles | Track sprites/retention | Cars+shadows | Dirty+C2P | HUD | Audio | Other |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| BASIC | 28% / 44% | 28% | 7% | 18% | 8% | 10% | 10% | 4% | 4% | 10% |
| F1 | 34% / 42% | 28% | 5% | 14% | 20% | 8% | 10% | 4% | 3% | 7% |
| CITY | 24% / 45% | 31% | 6% | 12% | 16% | 10% | 11% | 5% | 4% | 7% |
| WHACKO | 32% / 45% | 28% | 7% | 14% | 15% | 9% | 12% | 5% | 3% | 7% |

- **Busy work is flat.** No single routine exceeds 7% of samples. The largest is
  the C race-step body with inlined helpers.
- **Over-budget updates** shift about 5–10 points of busy time towards
  particles and wheel emission.
- **On F1**, track sprites and retention are the one track-specific excess.

The largest single item is the wait itself. On an over-budget update, the loop
misses the display-end edge and spins until the next one. That is up to a full
frame of idle CPU, and it snaps the update to 40 ms.

## Game-time defect: fixed time per update

**The original.** It programs PIT channel 0 to about 91.03 Hz (count 13107 at
speed 100). Each rendered update then runs as many physics ticks as have
elapsed, capped at 45 (`1000:fe5e..fe98`; [actor-layers.md](actor-layers.md)).
- Car physics, the countdown and lap clocks therefore run in real time on any
  CPU.
- Particles, actors and the page toggle advance once per rendered update,
  so their rate is frame-rate dependent on DOS too.

**The port.** `slicks_physics_clock_advance` (`src/game/race_timing.h`) credits
exactly 1/50 s (1193182 PIT input cycles / 50) to every update, however long
the update really took.
- An update that spans two frames therefore runs game time at half speed.
- At the measured cadence, cars and lap clocks run at 74% of real time on F1,
  90% on WHACKO, 94% on BASIC and 97% on CITY.
- The HUD status clock alone follows real vblanks (`slicks_status_clock_advance`).

**The faithful fix** feeds elapsed real time into the same accumulator, adding
1193182/50 per elapsed vblank. It keeps the 45-tick cap. The inner integrator
already loops per tick. Nothing else needs to change: the original's
once-per-update particle and actor work remains once per update.

## Publication policy

**Current policy** (`803fbf2`): prepare the update, then publish at the next
fresh display-end edge (line $100).
- Visible writes stay inside the lower border and upper border.
- The display is single-buffered: C2P writes the displayed bitmap's dirty
  regions.

**Model.** `tools/sync_policy_model.py` replays each log's per-update
`WORK_SAMPLE` sequence. Its prediction for the current policy lands within
2.5% of the measured cadence.

**Adaptive policy.** Publish at the next edge if the update finished before
it; otherwise publish at once.

| Track | Measured fps now | Model, current | Model, adaptive | Late (torn) publications | Worst interval |
| --- | ---: | ---: | ---: | ---: | ---: |
| BASIC | 47.2 | 48.2 | 50.0 | 35 (6%) | 22.3 ms |
| F1 | 37.0 | 38.2 | 48.9 | 239 (40%) | 26.0 ms |
| CITY | 48.6 | 49.3 | 50.0 | 14 (2%) | 21.4 ms |
| WHACKO | 45.2 | 46.2 | 49.8 | 66 (11%) | 24.9 ms |

**What tears.** A late publication can tear only where the beam crosses a
dirty region while C2P writes it. Static background and HUD cells that did not
change cannot tear.

**Late-publication side effects.**
- Boundary palette copper writes and audio effect starts, which today follow
  the wait, happen mid-frame.
- The palette change is a rare one-frame split.
- Audio uses VBI-staged DMA restarts ([audio-channel-plan.md](audio-channel-plan.md)),
  so late starts must be checked rather than assumed safe.

## Alternatives considered

- **Several simulation steps per rendered frame** (fixed-step logic, render
  skip): rejected.
  - `slicks_race_step` interleaves restore, simulation, particle ageing,
    permanent-mark commits (`actor_page` parity) and drawing.
  - Running it twice per publication doubles restore and draw on the frames
    that are already slow.
  - It drops the first step's sound events (reset per step).
  - It ages particles faster than the original ever does on a slow machine.
  - The original's own scheme is variable ticks per update, and the
    real-time accumulator above reproduces that scheme exactly.
- **Double buffering** to remove tearing: rejected for this release.
  - It needs a second 64 KB Chip bitmap. The 10,000-track fixture leaves only
    about 95 KB free.
  - Every dirty region must also reach the other buffer, so C2P traffic rises
    on every frame.
- **Further micro-optimization towards a worst case of 20 ms:** not planned.
  With the two changes above, game speed is correct at any frame rate, and
  the modelled cadence is 49–50 fps on all four benchmark tracks.
