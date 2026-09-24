# Open work

Updated 2026-09-25. Actionable open and deferred work only.

## Current focus: championship save/load/resume

1. Connect intermission Save Game to the original file picker and name-entry
   flow, file enumeration, overwrite/delete confirmation and failure/retry UI.
   Verify saving a real in-progress championship through native menus.
2. Connect load selection to the file reader, decoder and track/profile/vehicle
   resolver. Apply points, cash, inventory, participation, position scale and
   next-track index without running new-game resets.
3. Handle missing tracks/profiles, malformed saves, I/O errors and recovery
   leftovers without partially changing the live session.
4. Verify save, process exit, fresh restart and native-menu resume across a
   multi-race championship, including repeated saves and cancellation.

## Remaining game completion

5. Verify natural fuel/damage-enabled race completion, finish deadlines/delays,
   results and return/next-track flow against DOS; cover timed/Arcade modes.
   Recheck the upper-pit AI stall on the current build.
6. Port original weapon firing, cycling/depletion, weapons-enabled AI and shop
   transactions; verify gameplay-driven inventory and HUD transitions.
7. Compare full driving/AI/contact updates and sustained trajectories across
   routes, recovery, pits, collisions and active-driver combinations.
8. Complete animated boundaries, track-actor simulation/rendering/shared-pool
   integration, other probing callers and collision-effect emission. Verify
   actor ordering/reuse/capacity, foreground interaction, startup gating and
   permanent-mark survival on representative tracks.
9. Verify complete HUD/results sequencing across inactive/finished drivers,
   option combinations and real weapon state transitions.
10. Perform audio listening comparisons, check extreme engine frequencies
    against Paula limits, and extend event/call-site and results/pause/restart
    coverage. Keep direct playback without software mixing.

## Integration and release

11. Run applicable original-code oracles and muted 2 MiB/no-Fast-RAM A1200
    regressions after integration changes: persistence, recovery, dirty-region
    integrity, DMA blanking and clean system restoration. Broaden tracks/modes.
    Refresh binary-specific debug fixtures before using them on new builds.
12. Resolve executed source-wrap and unsupported platform-boundary cases.
13. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.
14. Publish/push only when explicitly requested.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- Further performance optimization: deferred by the user. Preserve existing
  fast paths; do not assume all current frames meet the 20 ms target.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
