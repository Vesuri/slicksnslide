# Open work

Updated 2026-09-26. Actionable open and deferred work only.

## Integration and release

1. Run additional muted 2 MiB/no-Fast-RAM A1200 regressions: F1 and WHACKO,
    six-mode race/menu transitions, and failure recovery. Check dirty-region
    integrity, DMA blanking and clean system restoration. Rerun applicable
    original-code oracles after changes; refresh binary-specific debug fixtures.
2. Refresh graphics trace validation for the expanded primitive table;
    resolve the screen-transition source-wrap call from 185ff to 2b8de
    and remaining unsupported platform-boundary cases.
3. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.

Publish/push only when explicitly requested.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- Further performance optimization: deferred by the user. Preserve existing
  fast paths; do not assume all current frames meet the 20 ms target.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
