# Natural race-completion verification

## Original finish branch

`make verify-race-completion` executes original instructions at physical
`22b50..22d1b` against native `advance_lap_checkpoints`, carrying state through
successive line crossings. Only the external sound-player call at `3989b`
returns through a boundary hook. Original mode/time checks, lazy lap limit,
rank assignment and lap adjustment, deadlines and cash/points execute intact.

2026-09-25: 6,480 sequences / 55,296 crossings passed: all six modes, all
nonempty active-driver masks, all 24 finishing orders, and three lap spreads.
The comparisons cover pre-expiry crossings, the first post-expiry crossing,
winner/lapped entrants, subsequent crossings by finished cars, the complete
rank array, cash/points, winner sound and deadline after every event. This
test deliberately does not claim full driving-trajectory equivalence.

Also rerun successfully: `verify-arcade-setup`, `verify-race-lap-limit`,
`verify-race-timing`, `verify-race-rewards`, `verify-dos-ai` and
`verify-dos-damage`. These include signed deadline wrap, strict expiry,
drive suppression at 269/270/271 ticks remaining, frozen completed-race state,
one-time track awards, original playlist indexing and the captured upper-pit
AI/movement/collision slices.

## Current-build A1200 baseline

Muted FS-UAE, 68020, 2 MiB Chip RAM, no Fast RAM; no debugger writes to game
state, no forced finishes. Baseline executable is the `9426004` build.

- `CONFIGD` / `diag_completion_baseline.gdb`, port 25158: native menu sets
  fuel 10 and maximum damage 300. BASIC finishes naturally at update 3648,
  clock 6641, deadline 6640, all four entrants finished, no race error.
  Repair-frame counts are 1/42/23/25; refuel-frame counts 98/82/98/97.
  No upper-pit stall recurred. This supersedes the old 3,600/7,200-update
  stall observations; it is not a claim that every possible pit trajectory
  has been proven.
- `OPTIONSB` / `diag_track_sequence.gdb`, port 25159: ordinary native menu
  input selects timed Arcade (5 seconds) and BASIC/BASICTRK. Natural results
  at updates 1408 and 1535; real results acknowledgement and intermission
  load the next track with `new_game=0`, preserving session RNG. The second
  results acknowledgement returns to the title. No skip-race action is used.

Old fuel/damage fixtures that require every entrant to reach exactly lap 5
are not the definition of success: the original winner resets the lap target
and lapped entrants finish their current lap. The composed oracle covers it.
