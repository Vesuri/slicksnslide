# Open work

Updated 2026-09-25. Actionable open and deferred work only.

## Remaining game completion

1. Integrate original weapon firing, cycling/depletion, weapons-enabled AI,
   projectile collisions/effects and actor allocation. Complete native shop
   screen comparison, end-game/retry and championship continuation gates.
   Verify gameplay-driven selected-icon/ammunition-bar updates,
   depletion/empty selection, inactive/finished-driver gating and preservation
   through pause/results/next-track. Use actual game events, not injected
   inventory values. Retain the original-font, composed HUD and native results
   regression gates documented in hud-verification.md.
2. Compare full driving/AI/contact updates and sustained trajectories across
   routes, recovery, pits, collisions and active-driver combinations.
   Investigate exhausted-fuel cars stranded on the BASIC upper route while
   targeting the pit in native Custom setups, including mixed and identical
   fleets. Compare the unfinished driver's trajectory with DOS before changing
   AI. Reproduction evidence belongs in race-completion-verification.md.
3. Complete animated boundaries, track-actor simulation/rendering/shared-pool
   integration, other probing callers and collision-effect emission. Verify
   actor ordering/reuse/capacity, foreground interaction, startup gating and
   permanent-mark survival on representative tracks.
4. Perform audio listening comparisons, check extreme engine frequencies
    against Paula limits, and extend event/call-site and results/pause/restart
    coverage. Keep direct playback without software mixing.

## Integration and release

5. Run applicable original-code oracles and muted 2 MiB/no-Fast-RAM A1200
    regressions after integration changes: persistence, recovery, dirty-region
    integrity, DMA blanking and clean system restoration. Broaden tracks/modes.
    Refresh binary-specific debug fixtures before using them on new builds.
6. Resolve executed source-wrap and unsupported platform-boundary cases.
7. Audit dependencies, memory/startup behavior, launcher defaults and release
    documentation. Package without original game data, captured frames,
    generated reference bytes or emulator artifacts.
8. Publish/push only when explicitly requested.

## Deferred

- Manual joystick press/steer/release verification: deferred by the user.
  Do not restart it without agreement.
- Further performance optimization: deferred by the user. Preserve existing
  fast paths; do not assume all current frames meet the 20 ms target.
- General-purpose translator expansion: exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths.
