# Fuel and damage fixture correction

Verified 2026-09-27. Historical evidence only; current work is in
`open-work.md`.

Both fixtures incorrectly required every finished driver to have visible
lap 5. Original completion assigns ranks to lapped entrants and stops the
remaining drivers after the winner according to the race's finish rules;
a common visible lap is not the completion predicate.

Reproduced the failures without changing simulation state. The fuel race
already completed at update 2921 with four refuelling/departure histories,
positions 1..4, and visible laps 3/6/4/4. The damage race completed at update
3426 with every driver marked finished and visible laps 5/6/5/5. Only the
fixed-lap assertion failed.

Original-executable checks rerun before correcting the fixtures:

- `verify-race-completion`: 6480 composed DOS finish sequences / 55296
  crossings, including lapped and repeatedly crossing finished drivers,
  all finish orders, active masks, six modes, awards, sounds and deadlines.
- `verify-dos-ai`: 256 DOS finish-line gates, 2352 checkpoint cases and 512
  lap-clock cases, plus the fuel/refuelling/pit-repair/route AI oracles.
- `verify-race-lap-limit`: runtime countdown, completion, ranking and
  post-completion state checks.

The fixtures now require all four finished flags and a permutation of
positions 1..4. Fuel already checked the permutation; damage now does too.
All fuel-service, damage/repair, results and status-pixel checks remain.
No game code or lap limits were changed.

Fresh reruns pass: `SLICKS_FUEL_RACE_OK CARS=4` at update 2921 (three status
checks), and `DAMAGE_RACE_OK` at update 3426 (four status checks, two damage
checks). Both have zero pixel failures. Debug audio was muted and sessions
closed on exit.

Local-only evidence: `tmp/handoff-{fuel,damage}-20260927.log`,
`tmp/handoff-{fuel,damage}-fixed-20260927.log`,
`tmp/handoff-finish-oracles-20260927.log` and
`tmp/handoff-ai-oracle-20260927.log`.
