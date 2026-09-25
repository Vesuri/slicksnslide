# Natural race-completion verification

## Extended driving matrix (2026-09-25)

The composed DOS/native matrix now passes eleven independent 7,200-update
sequences (79,200 updates total): both BASIC pit approaches, single-car and
identical-fleet starts, mixed fleets spanning all ten original vehicle
resources, human/computer/inactive selections, damaged-car repair/refuelling,
the stranded vehicle-5 approach, and BRIDGES/BUMPS routes. Cases 4 onward use
real wheel geometry. All four motion calls precede all per-car tails. Neither
side is resynchronized from the other after initialization. Finish deadlines,
collision scratch, smoke counters and shared RNG are also compared.

This found and corrected four further differences:

- Road braking emitted below rather than above the original threshold;
  damage-adjusted throttle and reverse-latch gates were missing.
- Damaged-engine smoke's update counter and two RNG draws were absent.
- Collision probes were cached across a four-car scan, although an earlier
  impulse changes the velocity used to test the next candidate.
- Wheel coordinates beyond the visible right edge were rejected instead
  of using original linear/packed material addressing. The normal row
  calculation continues to use `mult320`.

The final matrix and existing damage, AI, vehicle properties, race-completion,
driver-phase, surface-effect, dirty-region and planar-writer regressions pass.
The stale test-only entrant-count helper in `verify-drive-physics` was restored;
no removed production helper was reintroduced.

Both the vehicle-0 and vehicle-5 exhausted-fuel approaches remain stranded
in matching original-code/native trajectories. This is not evidence for
adding an Amiga-specific escape heuristic. The comparison is an execution
of the original instructions with matched initial state, not a claim that
an entire interactive DOS session or every track has been replayed.
Dynamic track actors, shared-pool allocation/rendering and all-track coverage
remain the separate actor/integration work; the driving oracle bounds
HUD/audio/actor allocation and uses a full point-particle pool.

Muted 68020/2 MiB/no-Fast A1200 integration, port 25252: normal native setup,
one-lap mixed fleet, four natural finishers at update 556, results and clean
restoration `0x1f`. No car positions or finish states were injected.

## Composed driving comparison (2026-09-25)

`verify-drive-trajectory` executes original motion `202f0..214df` and
per-car tail `221ac..23d8b`, including their real AI, arithmetic, RNG and
terrain probes. Each side advances independently for 7,200 updates; only
initial semantic state and elapsed ticks are shared. BASIC navigation and
material masks come from the original assets. Four initial scenarios cover
both exhausted-fuel pit approaches, one starting driver and four drivers.
Audio/HUD output and actor allocation are boundaries (full particle pool,
absent wheel graphics); this is not yet a complete rendered PC race replay.

The sustained comparison exposed native checkpoint/lap handling occurring
after collisions and stopping for finished entrants. DOS handles checkpoints
before layer sampling and laps before pair collisions, even after finishing.
Restoring that order fixes the first divergence at update 3,202. All four
7,200-update comparisons then match the asserted motion, AI, contact, damage,
surface, checkpoint/lap, fuel and RNG state.

Native read-only observations with four laps, fuel 10, damage 300, named AI
profiles at setting 100 and seed 0x1234:

- `NATURALT`, mixed 5/2/0/0 fleet, port 25250: prolonged (134,72) pit
  approach delay, then natural completion at update 5,586; four finishers,
  results and restoration 0x1f.
- `NATURALI`, identical vehicle-0 fleet, port 25251: three finishers and
  driver 3 still targeting (114,121) near (134,72) at update 7,200.
  This is a bounded observation, not a completion pass.
- Both independent DOS/native exhausted-fuel approach fixtures remain
  stranded through 7,200 matching updates. No AI escape heuristic or
  forced completion has been added. Broader mixed-role/fleet coverage is
  still required before closing the driving item.

## Post-race completion gate closed (2026-09-25)

The original post-race implementation is integrated and the first completion
item is removed from the actionable list. Evidence below covers original
record qualification/rendering/publication, final championship rendering and
statistics, waits/fades, actual read/save Retry/Skip, two-track Arcade return,
natural Custom results, and named-profile save/fresh-process reload.

Final source build `9bc71ce`, port 25184, reruns the longer `CONFIGDR`
upper-pit regression unchanged: update 3648, clock/deadline 6641/6640,
four finishers, repairs 1/42/23/25 and refuels 98/82/98/97, title return and
restoration `0x1f`. All 72 statistics bytes match the fresh-process reload.
Final host checks pass the original record, standings, full-screen pixels,
wait/fade, composed finish, actual race-step and storage-fault tests.

This closes the post-race flow, not general AI trajectory or all-track
fidelity work. The stranded native-setup pit case below remains explicitly
open under driving/AI, as do broader HUD/weapon option combinations.

## Record recovery and finish statistics (2026-09-25)

Both record and championship screens now use the independently verified
`2b73b` wait core through native key/game-port/100-ms-delay adapters. Record
reads have a no-allocation Retry/Skip warning; failed saves retain the one
in-memory insertion result for Retry. Explicit Skip preserves disk contents.
Real `.new`/`.bak` recovery files are never removed by this UI. A successful
old-format no-op is not treated as a failed save. Cleanup-pending publication
is reported separately and does not insert or publish again.

Original per-race finish/win statistics (`22b7c`, `22bdf`) are now connected
to the one-shot finish callback, in addition to championship match/win totals.
The dirty flag survives early return to the title. All 4,096 rank/profile/
wrapping-word oracle cases pass. Original PLR rules are retained: built-in
profiles 0..2 are intentionally not serialized.

- Port 25171, `OPTIONSBR`: missing record read then `.new` obstruction,
  ordinary Enter retries both, two record insertions for two tracks (no
  duplicate insertion on Retry), both natural finishes, final standings,
  statistics, setup save and restoration `0x1f` pass. BASIC's complete 1,694
  bytes match the original-format encoder after recovery.
- Port 25173, `OPTIONSBS`: read Retry followed by save Skip. All final-screen,
  statistic/save/title/restoration gates pass, and BASIC is byte-identical
  to its original file after Skip.
- Port 25175, `OPTIONSBL`: read Skip, no first-track insertion/write, second
  record panel and final standings/statistics/save/restoration pass. BASIC
  remains byte-identical to the original.
- Port 25170 reached both successful retries and the title; its final
  debugger expression referenced an unavailable local. The corrected
  assertion was rerun successfully on port 25171 above.

The new `NATURALD`/`NATURALF` diagnostic presets supply four named, persistable
computer profiles before native selection/new-game setup, then run normal
simulation. They never inject car positions, finish results or display data.
They use one lap and the established mixed fleet (vehicles 5/2/0/0), to test
the complete result/save/reload path separately from the longer service gates.
The initial four-
car-0 fixture on port 25176 reached three finishers by update 2400 but still
had three at 7200. The mixed fleet on port 25178 also had three finishers at
5400. Both initially inherited the automatic diagnostic's default **four**
laps, not the configured five. Port 25179 exposed that same fixture mistake
when requesting one lap; the corrected preset applies the selected count
before starting, and the debugger now bounds the completed lap counters.
The final preset starts through ordinary native GO input, not automatic
preparation (which also left the post-race filename uninitialized). Port
25180 rejected a lap assertion placed before initialization; port 25181
exposed the filename problem. These aborted fixtures are not passing runs.

`NATURAL` previously also matched the legacy `NOAUDIO` first-letter switch.
That collision is excluded. Completion notification/music now follows the
simulation's incomplete-to-complete edge instead of engine playback state,
so disabled audio cannot suppress the results checkpoint. Host debug output
remains muted while emulated audio stays enabled in the final fixture.

The port 25179 read-only snapshot identifies driver 0 (vehicle 5) near
(134,72), lap 2, exhausted fuel `0xfffffffc`, service flag 1, AI state 1 /
service state 2, pit target (114,121), waypoint 4 and no damage. No finish
deadline was manufactured. This native-setup trajectory case is tracked
separately in the driving/AI open item; it is not a passing service regression.

Port 25177, final-build `CONFIGDR`: the legacy upper-pit gate remains exact:
update 3648, clock/deadline 6641/6640, ranks 4/1/2/3, repairs 1/42/23/25,
refuels 98/82/98/97; all four finish, title returns and restoration is `0x1f`.

Port 25183, corrected `NATURALD`: ordinary native GO starts BASIC with four
named profiles, vehicles 5/2/0/0, one lap, fuel 10 and damage 300. All four
finish naturally at update 556; clock/deadline 1012/1011. Both original
record and championship owners pass all three phases, each profile receives
exactly one finished-race statistic, exactly one race win is awarded, and
positive-point match statistics agree with the final scores. The ordinary
exit saves CFG and the 235-byte PLR, returns to title and restores `0x1f`.
This short race does not exercise refuelling/repair; the separate CONFIGDR
gate continues requiring actual service activity for every entrant.

Port 25185 restarts a fresh `SETUPR` process from those actual files. All
four selected names/profiles and vehicles are recovered. All 72 bytes of
their nine-word statistics are byte-identical before save and after reload.
Reproduce in a dedicated sandbox (debug startup recopies reference tracks):

```sh
FSUAE_RUN=.run/native-results DEBUG_PORT=25183 SLICKS_NATURAL_RESULTS=damage amiga/debug.sh "" diag_native_results.gdb
FSUAE_RUN=.run/native-results DEBUG_PORT=25185 SLICKS_SETUP_RELOAD=1 amiga/debug.sh "" diag_results_reload.gdb
cmp amiga/.run/native-results/saved.stats amiga/.run/native-results/reloaded.stats
```

## Native championship screen (2026-09-25)

The final results owner now loads `sskuppi.@I`/`sskuppi.@p` and `kirj.@f`
from the archive, draws the original gradients, rankings, names and points,
updates original match/win statistics once, and returns to the native title.
The `(1,1)` text-shadow bridge is separate from records' `(1,0)` bridge.
Both planar pages are populated once; palette fades rebuild only the inactive
copper list before swapping in blanking. Temporary palettes use modal-owned
static storage to stay inside the original 4 KiB CLI stack.

`verify-track-records-pixels` includes 32 exact full-frame cup comparisons
against original x86 rendering, with real artwork/fonts and the production
68020 text bridge. All activity masks and tied ranks pass. Existing records,
track information, pause and Speed compositions remain passing.
`verify-palette-fade` passes 336 full original-routine cases / 657,408 RGB
writes including skipped ticks, final endpoints and unchanged base palettes.
`verify-result-wait` passes 192 original release/key/button/timeout/demo traces.

Muted 2 MiB A1200 `OPTIONSB`, port 25169 / `diag_standings.gdb`: both natural
Arcade races complete at updates 1408 and 1535. Final scores 7/6/2/0 order
drivers 3/2/1/0. All three screen/fade checkpoints, per-profile match/win
increments (including shared-profile accounting), CFG/PLR save on exit,
title return and restoration `0x1f` pass. `OPTIONSB` now explicitly saves
its isolated diagnostic setup to exercise statistics persistence.
The first transition run on port 25168 exposed excessive nested stack use;
it is superseded by the passing bounded-stack run above.

## Native record-results integration (2026-09-25)

The custom RESULTS overlay and its private text painter are removed.
`results_drawn` is retained as a legacy debugger-compatible final-frame
handoff flag, not evidence that a results screen has been drawn. Normal
interactive races now enter post-race processing without an extra Return.

The platform invokes qualification/insertion before refreshing player
selections, loads the existing original record-table renderer/icons/fonts,
then publishes changed tables through the transactional writer. Unchanged
tables are not written. A save failure displays an explicit disk warning;
recovery files are preserved. The original backdrop tint is 10/10/30 at 75%,
rectangle (35,75)..(270,180), with table origin (40,55). The 300-count wait is
mapped to 301 100-ms intervals and may be dismissed with a new key.

Muted A1200/no-Fast-RAM `OPTIONSB`, port 25167, completes both races, enters
both original record displays, returns to the title and restores hardware
status `0x1f`. BASIC inserts records (markers 0/6/6/7) and writes them;
BASICTRK has no improvement and remains byte-identical to its original.
`verify_target_records` compares the captured native table with the actual
AmigaDOS file: all 1,694 BASIC bytes match the independently verified encoder,
including untouched track payload. Reproduce with:

```sh
FSUAE_RUN=.run/post-race-records-v1 DEBUG_PORT=25167 SLICKS_OPTIONS_MENU=8 amiga/debug.sh "" diag_post_race_records.gdb
build/verify_target_records ref/TRACKS/BASIC.SS amiga/.run/post-race-records-v1/dh1/TRACKS/BASIC.SS amiga/.run/post-race-records-v1/first.records
```

After overlay removal, the 55,296 composed finish crossings and full native
race-step completion/freeze gates still pass. Existing record renderer tests
pass 216 command traces and 12 full-screen/font comparisons with original
assets and native 68020 text/icons. This checkpoint preceded the championship
screen and recovery integration documented above.

## Final standings translation

`verify-championship-standings` passes 65,536 original signed-score sorting,
inactive-slot, tie, shared-profile and statistic-wrap cases. It executes the
original ordering and match/win increments, not a reference reimplementation.
`verify-standings-draw` passes 20,736 complete original draw traces including
gradient colours/stripes, font colour slots, names, points and tied rank
suppression. This checkpoint covered typed routines before their platform owner.

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

## Earlier results audit: before native record integration

The earlier `race_runtime.c:draw_results` was a custom black rectangle labelled
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
