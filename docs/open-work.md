# Open work

Updated 2026-09-25. Actionable open and deferred work only.

## Remaining game completion

1. Perform audio listening comparisons, check extreme engine frequencies
    against Paula limits, and extend event/call-site and results/pause/restart
    coverage. Keep direct playback without software mixing.

## Integration and release

2. Run applicable original-code oracles and muted 2 MiB/no-Fast-RAM A1200
    regressions after integration changes: persistence, recovery, dirty-region
    integrity, DMA blanking and clean system restoration. Broaden tracks/modes.
    Refresh binary-specific debug fixtures before using them on new builds.
3. Resolve executed source-wrap and unsupported platform-boundary cases.
4. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.
5. Publish/push only when explicitly requested.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- Further performance optimization: deferred by the user. Preserve existing
  fast paths; do not assume all current frames meet the 20 ms target.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
