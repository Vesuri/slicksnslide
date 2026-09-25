# Natural race-completion verification

## Final standings translation

`verify-championship-standings` passes 65,536 original signed-score sorting,
inactive-slot, tie, shared-profile and statistic-wrap cases. It executes the
original ordering and match/win increments, not a reference reimplementation.
`verify-standings-draw` passes 20,736 complete original draw traces including
gradient colours/stripes, font colour slots, names, points and tied rank
suppression. These typed routines still need their platform screen owner.

## Original record insertion translation

`make verify-post-race-records` passes 25,272 complete comparisons with DOS
`255ff..25803`, including the real `2e000` qualification call and original
memory-copy helper. All 319 record bytes, four rank markers, display/change
flags and the trailer counter match. Cases cover all 81 participation
patterns, negative/zero/sentinel lap times, signed upgrade boundaries,
profile settings above 100, full/empty/tied tables and counter wraparound.
The typed routine is in `src/game/post_race_records.h`; this test alone is
not evidence of platform display/save integration.

The Amiga track writer now accepts post-race record tables as well as the
existing Clear Top 10s operation. Its 1,225 single/double-fault cases preserve
the caller's table and either the old file or the fully published new file;
the original Clear operation's 1,225 cases still pass. The original record
encoder passes 2,048 full-file/working-table comparisons, including promotion
of record one into record zero. Amiga cross-build passes. The new API alone
does not establish that the race caller invokes it.

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

## Natural return and restoration regression

`amiga/diag_completion_flow.gdb` continues beyond the results checkpoint:
the diagnostic supplies ordinary Return/Next input, returns to the title,
supplies Escape, and checks restored hardware status `0x1f`. It rejects race
errors, status-pixel failures, missing finishes/service activity and a
14,400-update timeout. No finish flag, car position or timer is debugger-written.

2026-09-25 rebuilt target results:

| Diagnostic / port | Natural result | Return and restoration |
| --- | --- | --- |
| `CONFIGDR` / 25161 | BASIC update 3648; clock 6641, deadline 6640; four finishers; repairs 1/42/23/25; refuelling 98/82/98/97 | Title reached; `0x1f` |
| `FUELR` / 25162 | BASIC update 2921; clock 5318, deadline 5316; four finishers on laps 3/5/4/4; refuelling 96/84/97/98 | Title reached; `0x1f` |
| `OPTIONSB` / 25163 | BASIC update 1408, clock/deadline 2563/2561; BASICTRK update 1535, clock/deadline 2794/2792; three finishers in each timed race | Next track through intermission, then title; `0x1f` |

The earlier single-track `OPTIONSA` display test (port 25160) also passed its
timer/LAST/LAP phases and original long grace: clock 2982, deadline 2981,
three finishers. The PAL-to-PIT accumulator can advance two ticks per update;
strict deadline expiry does not imply every result occurs at deadline+1.

Reproduce from the repository root after `. amiga/env.sh` and an Amiga build:

```sh
FSUAE_RUN=.run/completion-damage-v1 DEBUG_PORT=25161 SLICKS_DAMAGE_RACE=2 amiga/debug.sh "" diag_completion_flow.gdb
FSUAE_RUN=.run/completion-fuel-v1 DEBUG_PORT=25162 SLICKS_FUEL_RACE=2 amiga/debug.sh "" diag_completion_flow.gdb
FSUAE_RUN=.run/completion-arcade-v1 DEBUG_PORT=25163 SLICKS_OPTIONS_MENU=8 amiga/debug.sh "" diag_completion_flow.gdb
```

Debug output is muted without disabling emulated Paula DMA. These are
functional, not performance measurements. The two legacy service fixtures
also emitted FS-UAE BPL refresh-conflict warnings during the title transition;
the Arcade/original-setup path did not. Hardware restoration passed in all
three; this does not establish tear-free legacy diagnostic transitions.

## Results audit: not yet original-complete

The current `race_runtime.c:draw_results` is a custom black rectangle labelled
RESULTS with driver numbers and elapsed times. It is **not** a translation
of the original post-race screens. `results_drawn` only proves that this
existing overlay was drawn; the completion tests do not establish original
results pixels or complete post-race sequencing.

Original caller recovery identifies the missing sequence precisely:

- `255ff..257fe`: check each entrant's profile setting and best lap, apply
  qualification at `2e000`, insert lap records and update record positions.
- `25803..258da`: show the record table with the original backdrop/tint,
  using the already-ported `1aabc` record renderer; original wait is 300.
- `258dd..25934`: write changed records through the track-record writer.
- `25937..25965`: refresh player selection; show next-track intermission
  only when another effective playlist entry remains. The existing native
  continuation and the tested two-track path implement this branch.
- `25965..259d7`: at match end (including End Match), show final standings
  if any championship points are nonzero. Original `2a63e..2aad5` sorts
  signed points, draws profile names and gradient rows, handles tied places,
  updates profile match/win statistics and waits 1200 with fades.

Consequently the first open item remains open for this original results
integration. The verified finish arithmetic, service completion and menu
transitions must not be presented as completion of the whole item.

The intermission dispatcher/draw/preparation, renderer, setup-session and
Arcade-HUD tests passed on this run. The old whole-screen intermission pixel
oracle failed at (128,90): the championship work intentionally exposed two
DOS-hidden action rows, but the oracle still compares that extended screen
with the original hidden-row version.

The renderer now has an explicit `expose_actions` policy. Its original mode
passes all 75 DOS/native whole-screen/font comparisons; the native Amiga
owner explicitly enables the two additional actions. The composition test
checks both policies in 60 cases and verifies that the extended mode emits
both extra labels. No pixels are masked out of the DOS comparison, and the
production Save Game/Change Cars actions remain visible. The eight Change
Cars open/close comparisons and 20 row redraws also pass. The Amiga rebuild
passes its runtime check. Muted A1200 `UIMENU2` (port 25164) passes all 17
owner phases, including open failures, reopen, Change Cars, an explicit
visible-action policy assertion and restored hardware status `0x1f`.
