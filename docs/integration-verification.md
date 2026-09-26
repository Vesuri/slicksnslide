# Integrated-build verification

## 2026-09-26: after audio listening acceptance

Baseline: `9adf4e0`, using the native executable built from `0e8399d`.
The following tests exercise the integrated implementation, not replacement
frames or injected race state. Original game files and generated evidence
remain local-only.

### Host and original-code checks

The audio volume/pitch/adapter, setup storage/load, saved-game storage/resume,
track storage/list storage, dirty tracking, planar writers and race-completion
targets passed together. Local log: `tmp/integration-20260926-host.log`.

- Setup publication: 2,600 fault cases; setup loading: 254 checks.
- Championship storage: 1,485 save faults and 2,310 load faults/truncations.
  Original resume matching: 3,392 cases and 2,560 vehicle-fallback cases.
- Track storage and record publication: 1,225 fault cases each; saved track
  lists: 361 fault cases plus malformed/recovery/allocation guards.
- Dirty coverage: original HUD/font pixels, clock transitions, overlaps,
  shadows, sparse-list saturation and unchanged-frame caches.
- Real 68020 planar writers: 1,394 cases, including final-value stores and
  sparse pixels surviving overlapping conversions. Particle retirement:
  48 cases; motion: 70; consecutive original/68020 lifecycle: 1,920.
- Composed race completion: 6,480 sequences and 55,296 line crossings across
  six modes, all active masks and finish orders; exact sound requests match.
- Audio: all 27 sample one-shots/engine returns, all 44 allocation failures,
  48 lifecycle cases, 65,536 original volume pairs, 655,360 original pitch
  cases, 280 driver-step cases, 440 whole engine-update cases and 655,360
  safe Paula-period cases pass.

`verify-drive-trajectory` passed all eleven scenarios: nine BASIC scenarios,
BRIDGES and BUMPS, each with 7,200 consecutive original/native updates
(79,200 total). `verify-track-actors` passed 14 decoded frames, 20,000
motion/contact updates, 200 finish-flag style indices, 1,792 full-screen
render/restore comparisons, 64 saturated-pool comparisons, 1,920 off-road
pool cases and 808 startup cases.

`verify-native-tracks` successfully built all 195 supplied tracks with service
mode both disabled and enabled (390 scene/material/mask builds). This is
asset/navigation coverage, not proof of completing a race on every track.

### Fresh-process native championship persistence

Muted PAL A1200/68020, 2 MiB chip/no Fast, isolated directory
`amiga/.run/integration-20260926-championship`, port 2394:

1. `SLICKS_CHAMPIONSHIP=save`, `diag_championship_save.gdb`: selected three
   tracks through native menus, raced 40 updates, advanced through F9,
   cancelled/reopened Save, named E2E and saved. Cash was 250; the actual
   196-byte file contains three tracks and next index 1. Passed
   `NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`, restoration 31.
2. Separate process, `SLICKS_CHAMPIONSHIP=load`,
   `diag_championship_load.gdb`: loaded through the native picker, prepared
   `TRACKS/66.SS` with `new_game=0`, raced 40 updates and exited. Passed
   `NATIVE_CHAMPIONSHIP_FRESH_PROCESS_RESUME_RACE_OK`, restoration 31.
3. Points, cash, inventory and vehicles match byte-for-byte before save and
   after loading. Independently decoding the new E2E.SSS also matches all
   four restored snapshots. Runtime vehicles, saved position scales and
   all weapon inventory slots passed the target-side assertions.

The existing fixtures write their snapshots under `.run/championship-v1`;
the tested save itself is in the new isolated directory above. No user saves
were read or overwritten, and no debugger target-memory writes were used.

### Native CITY renderer/actor/audio integration

Same muted PAL A1200/68020, 2 MiB chip/no Fast settings, isolated
`.run/integration-20260926-city`, `SLICKS_TRACK_ACTOR_TEST=1`,
`SLICKS_TRACK_ACTOR_CASE=2`, `diag_track_actors.gdb`:

- Starting-grid checks passed for all 18 track actors and shared handles.
- Reached the native 600-update bound with 18 retained actors, shared-slot
  high-water 165, 1,480 permanent marks and four flag activations.
- Per-frame bitmap auditing remained enabled; no bitmap or collision failure
  checkpoint fired, and the race error remained zero.
- Passed `TRACK_ACTOR_RESTORE_OK audio_spills=0`, restoration 31. The fixture
  now explicitly rejects any audio-VBI display-blanking violation.

This is a bounded integrated race check, not a claim of natural CITY race
completion or of meeting the deferred 20 ms performance target.

### Compatibility audit follow-up

The old graphics summary tool rejects newer mixed traces at target 0e6b1,
before performing its bounds audit. This is a tooling coverage gap, not a
demonstrated native-game failure. The historical screen-transition call from
185ff to 2b8de is still present in older local traces: its source Y plus
height exceeds the declared sprite height. It remains outside the bounded
subrectangle helper's proved ABI. No compatibility completion is claimed.

### Additional native regressions and test cleanup

Baseline `6fc78e4`, executable from `0e8399d`, muted PAL A1200/68020,
2 MiB chip/no Fast RAM:

- F1 and WHACKO each passed 600 updates with per-frame bitmap auditing,
  clean restoration and zero audio blanking spills. F1 retained 32 actors,
  reached 200 shared slots and recorded 2,076 marks; WHACKO retained five
  actors, reached 155 slots and recorded 1,854 marks with four flag events.
  Logs: `tmp/integration-f1.log`, `tmp/integration-whacko.log`.
- Five native pause allocation/archive failures recovered and a subsequent
  retry succeeded. All fifteen before/after race, chunky and configuration
  snapshots matched. Log: `tmp/integration-pause-failure.log`. This fixture
  does not assert the audio-spill counter.
- Arcade skip/next/end passed two starts, two pause menus and an intermission
  with clean audio ownership, restoration 31 and zero spills. This is not
  native transition coverage for all six modes. Log: `tmp/integration-restart.log`.
- Original-code options, race-menu, intermission-menu/preparation, setup-session
  and weapon-shop gates passed together (`tmp/integration-mode-oracles.log`).

These bounded checks do not establish natural race completion or 20 ms frames.
Scripted debugging now closes its owned emulator when GDB exits. A real
restoration test passed `SLICKS_RESTORE_OK STATUS=1f` and removed its PID file;
a deliberately failing debugger preserved exit status 1 and also removed its
PID file. The subsequent process inventory showed no FS-UAE processes. The
normal launcher and interactive debugger behavior are unchanged.

### Expanded primitive-trace audit

The validator explicitly recognizes surface-event target `0e6b1` and the
legacy `unknown` label for that target only. Unknown addresses and malformed
argument lists still fail. Subrectangle height uses the low byte, matching
`3b8d:007b`, avoiding false overruns from unrelated upper argument bits.
Six synthetic regression checks pass. The historical `tmp/pc-fixed` trace
passes the full BASIC coverage gate, retaining the genuine source-overrun
notice. `tmp/audio-pcm-identity` passes the explicitly partial bounds/schema
audit; it lacks the complete BASIC title sequence and is not claimed as full
sequence coverage. The source-wrap compatibility implementation remains open.
