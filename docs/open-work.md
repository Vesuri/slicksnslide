# Open work

Updated 2026-09-25. Actionable open and deferred work only.

## Remaining game completion

1. Finish original post-race results flow: replace the custom RESULTS overlay
   with lap-record qualification/insertion, record display/save, and final
   championship standings/profile statistics with original waits/fades.
   Verify that sequence through natural fuel/damage and timed/Arcade finishes,
   next-track intermission and final return. Keep the natural-completion and
   upper-pit regression gates passing.
2. Port original weapon firing, cycling/depletion, weapons-enabled AI and shop
   transactions; verify gameplay-driven inventory and HUD transitions.
3. Compare full driving/AI/contact updates and sustained trajectories across
   routes, recovery, pits, collisions and active-driver combinations.
4. Complete animated boundaries, track-actor simulation/rendering/shared-pool
   integration, other probing callers and collision-effect emission. Verify
   actor ordering/reuse/capacity, foreground interaction, startup gating and
   permanent-mark survival on representative tracks.
5. Verify complete HUD/results sequencing across inactive/finished drivers,
   option combinations and real weapon state transitions.
6. Perform audio listening comparisons, check extreme engine frequencies
    against Paula limits, and extend event/call-site and results/pause/restart
    coverage. Keep direct playback without software mixing.

## Integration and release

7. Run applicable original-code oracles and muted 2 MiB/no-Fast-RAM A1200
    regressions after integration changes: persistence, recovery, dirty-region
    integrity, DMA blanking and clean system restoration. Broaden tracks/modes.
    Refresh binary-specific debug fixtures before using them on new builds.
8. Resolve executed source-wrap and unsupported platform-boundary cases.
9. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.
10. Publish/push only when explicitly requested.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- Further performance optimization: deferred by the user. Preserve existing
  fast paths; do not assume all current frames meet the 20 ms target.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
