# Race frame pacing

Two parts set the race's timing. The race clock decides how much game time
each rendered update runs. The publication policy decides when a prepared
update reaches the display. Both match the original's behaviour on a slow
PC: game speed is correct at any frame rate, and only the number of drawn
updates per second varies.

## The original's model

The DOS game programs PIT channel 0 to about 91 Hz (count 13107 at speed
100). Each rendered update runs as many physics ticks as have elapsed since
the previous one, capped at 45 (`1000:fe5e..fe98`; see
[rendering.md](rendering.md)).

- Car physics, the countdown and the lap clocks run in real time on any CPU.
- Particle ageing, actor animation and the page toggle advance once per
  rendered update, so their rate depends on frame rate on DOS too. The
  Amiga's rate of up to 50 updates/s is within the DOS range.

## Real-time race clock (B1)

`next_physics_ticks` (`src/game/race_runtime.c`) follows `fe5e..fe98`
whenever the race has a raster clock:

- **Source.** `slicks_amiga_platform_raster_time`
  (`src/platform/amiga/amiga_platform.cpp`) returns PAL raster lines as
  `vblank_count*313 + line`, reread until the frame count and beam position
  agree. `slicks_physics_clock_lines` (`src/game/race_timing.h`) adds 1193182
  phase units per line, and one tick consumes `divisor*15625`. The rate is
  therefore exact for the real 15625 Hz line rate, not the nominal 50 Hz.
- **First update.** It records the clock origin, waits for one tick and
  integrates a batch of one.
- **Later updates.** Each takes every tick elapsed since the previous read,
  waits while none has elapsed, and caps the batch at
  `SLICKS_PHYSICS_BATCH_MAX` (45).
- **Lagging read.** A read just after line 0, before the VBI has counted
  the frame, appears to go backwards. `physics_clock_lines` adds no time
  then and keeps its reference. Single reads are clamped to 40000 lines,
  which is more than 45 ticks at the slowest divisor.
- **Speed changes.** `slicks_race_set_timer` restarts only the phase, so
  game time stays monotonic. This is adaptation D4: the original refunds
  time here.
- **Lap clocks** advance two units per tick (`advance_car_clock`).

**Pause.** As in the original, the clock keeps running while paused. The
first update after a pause therefore runs a capped batch of 45 ticks
(about half a second of game time) and then continues normally.

**Installation.** `slicks_diag.c` sets `race->raster_clock` on every plain
launch. Benchmarks opt in with `NATURAL{B,S}<track>RT`
(`SLICKS_REALTIME_CLOCK=1`, DETAIL 0 or 8, no HUD phase).

**Fixed-clock diagnostic mode.** Without a raster clock, `next_physics_ticks`
calls `slicks_physics_clock_advance`. This credits exactly 1193182/50 PIT
input cycles per update, however long the update took. All deterministic
fixtures, benchmarks, shadow and RETCHECK runs, and the WHDLoad diagnostic
races use this mode. Their `FINAL_STATE` therefore depends only on the
inputs, not on timing, so a performance change must reproduce it exactly.
In real play, the fixed clock caused slow motion: an update that spanned
two frames ran game time at half speed. That is why B1 replaced it on
plain launches.

The HUD status clock follows vblanks on its own (`slicks_status_clock_advance`).

Checks: `make verify-race-timing` (line clock against 64-bit arithmetic at
all 151 UI speeds) and `make verify-drive-physics` (first batch, zero-tick
wait, long-run total, 45 cap, lagging read, disabled timer). Real-time
benchmarks print `REALTIME_CLOCK`; ticks match raster time within one tick.

## Adaptive publication (B2)

The display is single-buffered. C2P writes the dirty regions of the shown
bitmap. The race loop in `slicks_diag.c` prepares an update and then calls
`slicks_amiga_platform_wait_publication`:

- If the update finished before the first display-end edge (line $100)
  after the previous publication, it waits for that edge. C2P then writes
  during the lower and upper borders, as it did before B2.
- Otherwise it publishes at once and returns 1. The update is late and may
  tear, but only where the beam crosses a dirty region while C2P writes it.
  Unchanged background and HUD cells cannot tear.
- The next deadline is the first edge strictly after the publication time.
  `slicks_amiga_platform_begin` clears it. Missed slots are not queued, and
  the loop never waits a second time between audio and C2P.

Late-publication side effects are bounded:

- Effect requests are interrupt-locked and staged for the VBI (`play_effect`,
  `slicks_amiga_audio_tick`), so a mid-frame start is safe.
- A boundary palette change can split one frame.
- The audio-in-blank diagnostic skips late publications.

`LATE_PUBLICATIONS` in benchmark logs counts late updates in the measured
window. `diag_display_end_limit.gdb` checks the one-publication-per-VBlank
limit and the on-time publication row. Publication timing never feeds back
into simulation: a fixed-clock run with adaptive publication keeps its
`FINAL_STATE`.

**Result.** With both B1 and B2, the four benchmark tracks run at about
46–50 updates/s. Without B2 the rate was 33–48, because an over-budget
update snapped to 40 ms. F1 is the heaviest track, and about half its
updates publish late. The user watched an F1 race on the normal build and
saw no visible tearing.

## Alternatives not taken

- **Several simulation steps per drawn frame.** `slicks_race_step` interleaves
  restore, simulation, particle ageing, permanent-mark commits and drawing.
  Running it twice doubles restore and draw on frames that are already slow,
  drops the first step's sound events, and ages particles faster than the
  original ever does. Variable ticks per update is the original's own model.
- **Double buffering to remove tearing.** It needs a second 64 KB Chip bitmap,
  and the 10,000-track fixture leaves about 95 KB free. Every dirty region
  would also have to reach both buffers.
- **A 20 ms worst-case update.** Mean work is below 20 ms on every track, but
  the slowest updates reach 21–26 ms. Fixing that would need a 4–24% cut,
  most of it on F1. Busy time is spread thinly: no routine exceeds about 7%
  of samples. Dozens of later experiments each moved total work by
  0.03–1% (see [performance.md](performance.md)). With B1 and B2, game speed
  is correct whatever the frame rate, so worst-case 20 ms updates are out of
  ship scope ([open-work.md](open-work.md)).
