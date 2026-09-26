# Open work

Updated 2026-09-26. Actionable open and deferred work only.

## Integration and release

3. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.

Publish/push only when explicitly requested.

## Performance (after integration and release)

- Profile and optimize general gameplay to at most 20 ms per update on a
  stock PAL A1200 (68020, 2 MiB chip RAM, no Fast RAM). Preserve fidelity,
  include particle-heavy gameplay, and verify complete frames rather than
  isolated rendering timings.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
