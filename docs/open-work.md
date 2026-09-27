# Open work

Updated 2026-09-27. Actionable open and deferred work only.

Publish/push only when explicitly requested.

## Performance

- Profile and optimize general gameplay to at most 20 ms per update on a
  stock PAL A1200 (68020, 2 MiB chip RAM, no Fast RAM). Preserve fidelity,
  include particle-heavy gameplay, and verify complete frames rather than
  isolated rendering timings.
  - Establish four-track work-time distributions, budget misses and cadence,
    correlating busy frames with particle/actor load.
  - Use selective profiling with same-build outer-only controls to isolate
    restoration/drawing, simulation and their dominant subpaths.
  - Explain F1 overhead and prioritize safe structural reductions in repeated
    work and Chip-RAM traffic; retain only full-workload improvements.
  - Verify retained changes against original-code and display-integrity tests,
    then remeasure all four tracks including particle-heavy frames.
  - Profile with the target-side CIA-B sampler (`amiga/pc_profile.sh`), not
    GDB interrupts; verify native replacements with `amiga/shadow_check.sh`
    plus the host DOS oracles. See `docs/performance-profiling.md`.
  - Particle-heavy frames cost about 1.1 lines per live point: tighten the
    native draw chain (drafted, fewer Chip accesses per point), then decide
    whether exact retention of unmoved, unoverlapped points is worth its
    conflict tracking. The C restore loop is already lean.
  - Measure dropping the sparse-pixel prune (its rectangle scan now costs
    more than the conversions it avoids) and reloading the 528-move copper
    palette only when colours change.
  - Replace remaining hot compiled C where it shrinks executed code volume:
    per-car tails in `slicks_race_step`, actor ordering, emission slot scan.
  - F1/CITY: remove repeated per-object work for stationary track objects.

## Verification debt

- `diag_fuel.gdb` and `diag_damage_race.gdb` fail identically on 5fa9458 and
  later builds (finished car reaches lap 6; fuel fixture sees no finishers).
  Decide whether the fixtures or the finish/lap behaviour are stale.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
