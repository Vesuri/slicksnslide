# Development and verification rules

Working procedures, not an open-work list. See [open-work.md](open-work.md)
for scope, pending decisions and paused/deferred work, and [../CLAUDE.md](../CLAUDE.md)
for project rules. Accepted and rejected optimizations are summarised in
[performance.md](performance.md) and intentional deviations in
[fidelity.md](fidelity.md); the detailed historical logs are in git history.
Consult them before revisiting a rejected design or repeating a test.
An isolated routine oracle, a native smoke test and a production-screen comparison
prove different things; none alone proves whole-game fidelity.

## Execution and acceptance

- Fail-fast order: build/quick correctness smoke tests, parent performance
  comparison, then expensive correctness/rendering validation only for
  promising candidates, followed by final timing confirmation. Do not pay
  for full validation of a candidate already rejected by performance.
- Profile with the CIA-B PC sampler (`amiga/pc_profile.sh`, see
  [performance.md](performance.md)); GDB sampling is vsync-locked.
- Compare `amiga/bench_tracks.sh` against an exact parent-build control;
  FINAL_STATE must match. Use uninterrupted timing, not debugger-stopped runs.
  Include HUD phases when timing variation could obscure a regression.
- Native routines require independent Unicorn/host oracles. Simulation
  changes also require `amiga/shadow_check.sh` at the relevant sites.
  All-site mask is 0x3fe; surface tail is site 9 / SHADOW_SITES=512.
- Rendering changes require F1/CITY/WHACKO display audits:
  `SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 ./debug.sh "" diag_dirty_sprites.gdb`.
  Drawing-order/retention changes additionally require RETCHECK and
  `diag_retention_check.gdb`, with the independent geometry/legacy path.
- Expensive diagnostics remain opt-in; time all normal-game work.
  Force relevant object rebuilds when changing SHADOW/RETCHECK flags.
- Never rebuild an ELF in use by an emulator. Debug runs are muted; keep
  run.sh audible. Close every emulator session started for this work.
- Commit each verified piece with the repository's Vesuri identity, with
  hooks/signing disabled and no co-author trailer. Push only when asked.
  Never commit original executables, dumps, traces, screenshots or other
  byte-derived game material.
