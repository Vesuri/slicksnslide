# Open work

Updated 2026-09-26. Actionable open and deferred work only.

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

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
