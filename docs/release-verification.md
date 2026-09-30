# Development release audit

## 2026-09-30 — manual release retry, in progress

In the prepared stock PAL A1200 session (68020, 2 MiB Chip, no Fast RAM,
standalone release executable, default 4 KB stack), the user confirmed that
Players opens and returning to the main menu succeeds. This clears the
previously reported exit on that transition in this manual retry.

The user also observed that Escape from Players applies the main-menu palette
immediately, leaving the outgoing image garbled for over half a second before
the main-menu image appears. Recorded as B9; fix deferred until after this
manual session at the user's request. Race/save/exit and WHDLoad checks remain
pending.

The user subsequently reached the post-race results screen (local screenshot
`FS-UAE_Full_260930-2039_00.png`). Workbench was briefly visible before the
results appeared; the exact disk operation has not yet been identified.
Race completion is observed; championship save, normal exit and WHDLoad
checks are still pending. The requested preservation of the current game
display during disk access is tracked as B10.

After dismissing the results, the user reached `ENTER: RETRY / ESC: END MATCH`
(`FS-UAE_Full_260930-2040_00.png`). Escape then reached the trophy/final
standings screen (`FS-UAE_Full_260930-2041_00.png`), again exposing Workbench
briefly first. This is a second observed B10 transition. This match ended
without reaching a between-races save menu; manual championship saving remains
untested.

Correction after inspecting the prompt's source: `ENTER: RETRY / ESC: END MATCH`
is exclusively the intermission failure/retry notice, not a normal match-end
prompt. The earlier guidance to end the match misidentified it. Subsequent
screenshots show 195/195 selected tracks and Classic mode with five laps.
Intermission entry therefore failed; its exact failed operation remains to be
diagnosed (B11). Race completion passed, but intermission did not.

The user then confirmed a clean normal exit to Workbench. Standalone manual
exit passes; championship saving remains blocked by B11, and both WHDLoad
manual passes are still outstanding.

Isolated `OPTIONSTI` reproduction with Workbench loaded and the default 4 KB
stack reaches the same intermission failure (`tmp/standalone-release-j8z09nkk`).
An optimized source-line breakpoint run was inconclusive and stopped.
Exec AllocMem/return breakpoints in `tmp/standalone-release-vcndob3s` identify
the exact failure: the 65,536-byte preview arena in
`slicks_amiga_intermission_open` returns null. The 41,500-byte SLICKS.DAT,
2,011-byte BASICTRK.SS, menu surface and dialog were already allocated.
This is evidence for peak-memory pressure, not another demonstrated leak.
Tests used a separate copy of the installation and did not modify the manual
session's data. The completed manual emulator and diagnostic sessions were closed.

## 2026-09-30 — allocation ownership and default-stack lifecycle

The framework allocated its 24,577-word blitter queue unconditionally in a
startup constructor, even though Slicks builds without `USE_BLITTER_QUEUE`.
Its raw static pointer had no destructor or explicit shutdown free. The old
release audit `tmp/standalone-release-4781m6ij/debug.log` confirms exactly
49,158 requested bytes outstanding after returning to DOS. In the same
Workbench-loaded 2 MiB test, a menu-surface request of 86,422 bytes failed.
That run's attempted debugger-written stack watermark is **invalid**: this
FS-UAE build does not reliably apply target-memory writes. It is not evidence
of stack overflow.

The queue allocation is now conditional on actually using queued blitting.
There is also an explicit final shutdown release for builds that do enable it.
The platform's existing takeover, copper and bitmap implementation is unchanged.
The fix saves roughly 48 KiB during play and avoids leaking it at every exit.

`tools/memory_audit.py` observes real Exec AllocMem/FreeMem calls from before
constructors until return to DOS. It checks ownership and exact free sizes,
including allocations made by C++ new. It uses a host-side debugger driver,
not a tracking allocation on the Amiga, and does not write game memory.
The native DEMOPLR fixture completes two demos naturally, opens/closes Players
through ordinary input events, and exits. The previous debugger-injected input
attempts were discarded, not counted as passing evidence.

Passed local fixtures (`tmp/standalone-release-*`):

- `jsskk5u2`: normal build, Workbench loaded, 2 MiB Chip/no Fast, 4096-byte
  Shell stack, two natural demos then Players; 161 allocations, zero failures,
  zero outstanding at successful exit.
- `ke0l7jnf`: deliberately restricted 1 MiB Chip, genuine allocation failure;
  return code 20, 19 attempts/one failure, zero outstanding allocations.
- `x4vdm7sd`: native STACKCHECK watermark, same demo/Players sequence; 692
  bottom-of-stack bytes remain untouched, zero outstanding allocations.
  One optional 64 KiB request fails and its normal fallback succeeds.
- `ek3ac_fx`: STACKCHECK, DISPMEM's ten display-construction failure boundaries
  followed by demo/exit; 692 untouched stack bytes. Three direct launches in
  the **same** Workbench session all return successfully. After each, Avail
  FLUSH reports exactly 1,891,720 free Chip bytes and a 1,890,792-byte largest
  block. No accumulating leak or fragmentation is observed.
- `vjqrkoq0`: restored normal build, two complete DEMOPLR launches in the same
  Workbench session. The first audit confirms Players stage 4, two natural demo
  returns, zero demo-state errors and zero outstanding allocations. Both
  launches return successfully; free Chip bytes/largest block are the same
  1,891,720/1,890,792 after each. No stack instrumentation is in this binary.

STACKCHECK=1 is diagnostic-only and writes its watermark natively before main;
it neither extends the stack nor allocates a replacement. Mode stamps force
recompilation when toggling it. These are measured exercised paths, not a
mathematical maximum for every possible input. Debuggers and emulators used
for these completed tests were closed. All runs were muted.

Repeat after building/copying the matching executable into a private install:

```
. amiga/env.sh
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMOPLR --allocation-audit --audit-players --marker ALLOCATION_CLEANUP_OK
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DISPMEM --allocation-audit --repeat 3 --marker ALLOCATION_CLEANUP_OK
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMORET --allocation-audit --chip-memory 1024 --expect-failure --marker ALLOCATION_CLEANUP_OK
```

For the watermark, build `make -C amiga STACKCHECK=1`, run with the matching
binary, then restore `make -C amiga STACKCHECK=0` before release packaging.

The clean-tree `make release-check` gate passes at `fe94b50`, including all
host reference comparisons, the 197-file extraction/preservation/rejection
tests, two identical clean stripped builds and the 11-member LH5 audit.
Log: `tmp/release-gate-memory-fix.log`. The normal stripped executable is
455,104 bytes, SHA256
`be8071c8df2f9449759db8e14b7a7dbf6b2e51c93d780b6e7ae7e6264e7dcec7`.
`build/release-check/Slicks-0.1.lha` is 281,331 bytes, SHA256
`8c4b0861ccd678a64e2f6706d7932c68f95b04babe67de03570fdd8b992d3ee8`.
This is a validation candidate, not the final 0.90 release. The prepared
manual-retry executable compares byte-for-byte with the packaged executable.
The exact archive also passes real Intermediate-level Installer tests:
`tmp/installer-script-i4r6c_5n` (Use existing, stock 2 MiB) and
`tmp/installer-script-m6rzumtt` (fresh extraction through RAM-backed T:,
2 MiB Chip plus 4 MiB Fast). Both verify original data, expected binaries,
absence of Play, preserved user files where applicable, and staging cleanup.

## 2026-09-30 — direct executable launch and RAM temporary directory

Removed the Play script from the package and installer. Upgrades also remove
the obsolete installed script. Standalone instructions now run `Slicks` from
the data drawer, without increasing the Shell stack. The existing template icon
already sets `MINUSER=AVERAGE` (Intermediate); a new archive assertion protects
it. The earlier manual fixture incorrectly overrode that setting with NOVICE
on its command line. Installer test launches now use AVERAGE explicitly.

Real Installer tests passed using the updated sources:

- `tmp/installer-script-zoncd5dx`: stock 2 MiB/no Fast, Use existing preserves
  modified tracks and user-file placeholders, removes legacy Play/icons, and
  never asks for ZIP or scratch storage.
- `tmp/installer-script-ms9rlngg`: 2 MiB Chip plus 4 MiB Fast, fresh extraction
  through `T:` assigned to `RAM:T`. All 197 original files match and temporary
  staging is removed. RAM-backed temporary storage is supported, not prohibited;
  on a stock 2 MiB system disk scratch remains the memory-saving recommendation.
- `tmp/installer-direct-candidate/Slicks-0.1.lha`: independent audit passes with
  exactly 11 allowlisted members and no Play script (281,069 bytes).

These packaging checks do **not** close the manual release gate. The user saw
the menu, completed an idle demo, then selecting Players exited and relaunch
failed. Memory cleanup and direct 4 KiB-stack lifecycle verification are still
under investigation; neither success nor a memory-leak diagnosis is claimed.

## 2026-09-30 — WHDLoad-only icon and existing-data installer workflow

At the user's request, the installer now follows the WHDLoad Install Template
and Vette conventions: mandatory WHDLoad path check, standard `Slicks.info`
project icon with `Slave=Slicks.slave` and `PreLoad`, and no optional-launcher
question or standalone icon. The uniconed `Play` script remains usable through
`Execute Play`. Upgrades remove only the old `Play.info` and
`SlicksWHDLoad.info`; data, keys and saves are not deleted.

The default `Use existing` choice skips both ZIP and scratch questions when
the two original files and TRACKS drawer are present. `Reinstall` refreshes
the publisher's files without removing user settings, profiles, championships,
keys or custom tracks. The whole-drawer deletion prompt remains deliberately
absent to protect those files.

The exact candidate `tmp/installer-whd-candidate/Slicks-0.1.lha` passes the
independent 12-member LH5 package/CRC/source/version audit. It is 281,199 bytes,
SHA256 `88bfe7925c6bb1fab2a57755e37055dbfeb2ed535149c413273e0d36f4b7f875`.
The native executable remains byte-identical to the D-1/D-2 candidate; the
change is confined to the installer, documentation and installer tests.

Real Installer checks using this archive pass on 2 MiB Chip/no Fast:

| Case | Local fixture under tmp/ | Result |
| --- | --- | --- |
| Use existing | installer-script-nba46zx_ | ZIP/scratch branches would abort if reached; neither is reached. Modified records and all user-file placeholders survive; old icons disappear. |
| Fresh install | installer-script-c5bs1bdi | All 197 original files, native executable, slave, single WHDLoad icon and staging cleanup match. |
| Reinstall | installer-script-n8yr4d5j | Original track restored; configuration, profile, championship, key placeholder and custom track survive. |
| Missing TRACKS drawer | installer-script-r1krdmnq | Incomplete installation is repaired by extraction, not falsely reused. |
| Missing WHDLoad | installer-script-v8ri8nfn | Prerequisite failure branch reached before installation writes; fatal requester replaced with a marker and quiet exit for unattended testing. |

The user also supplied a screenshot of the unmodified fatal requester from
the deliberate missing-WHDLoad fixture `installer-script-w5jfmvu8`, confirming
the expected visible error. That fixture was closed rather than left waiting
for dismissal. All other test emulators closed automatically. The earlier
manual installer session was stopped because its workflow was superseded;
D-3 still requires a new manual session, now using Execute Play for standalone.
No private key was used, no release tag was made, and `dist/` was not replaced.

## 2026-09-30 — ship gate D-2 passed

The exact stripped D-1 candidate passes all three full-frame display audits
with `SLICKS_LIVE_STATS=0`: F1, CITY and WHACKO each cover 600 updates, with
32/18/5 actors and 2068/1480/1854 marks respectively. The debugger uses the
preserved matching release ELF, not a subsequently rebuilt diagnostic ELF.

After the diagnostic-only allocation repair in 68efb9c, all four RETCHECK
fixtures pass on PAL A1200 with 2 MiB Chip and no Fast RAM:

| Track | Race comparisons | HUD comparisons | Geometry comparisons |
| --- | ---: | ---: | ---: |
| BASIC | 603 | 700 | 0 |
| F1 | 603 | 700 | 575 |
| CITY | 603 | 700 | 603 |
| WHACKO | 603 | 700 | 0 |

Every surface, particle, immutable-map, HUD and geometry mismatch counter is
zero. Zero geometry counts on BASIC/WHACKO are not claimed as geometry coverage;
F1 and CITY exercise that path. Logs: `tmp/release-render-73f9c03/display-*.log`,
`retention-*.log`, `snapshot-host.log` and `retention-remaining.log`.

The normal build was then rebuilt cleanly and stripped. It is byte-identical
to the preserved D-1 candidate, SHA256
`a25d8cc46aa4f64a2fa807ae7a02d2bfb0529731440cd7f44411161aea6888e8`.
Thus the diagnostic repair does not change the already audited release payload.
All automated emulators were muted and closed. D-2 is complete; the actual
manual Installer/Play/gameplay/exit and WHDLoad session is still D-3, not covered
by these automated results. No archive was published and no version tag made.

## 2026-09-30 — retention diagnostic allocation repair

The release-gate RETCHECK run initially reached 700 updates with zero actual
comparisons. Its 129,748-byte contiguous state allocation failed: after the
64,000-byte surface allocation, 134,472 bytes remained free but the largest
block was only 128,840 bytes. Reserving the state earlier instead prevented
startup's menu cache from fitting, so that attempt was rejected.

The optional diagnostic now stores the same mutable prefix in independently
allocated 1 KiB blocks, with an exact-sized final block. It still keeps all
production assets resident and hashes the three excluded immutable maps.
Allocation failure exits the diagnostic rather than silently bypassing its
comparisons; partial allocations are released at cleanup. The gate reports
free and largest memory measurements to make future failures identifiable.

The host snapshot test passes full mutable-state restoration, guards around
each block and all twelve immutable-map mutation cases, both normally and
under address/undefined-behavior sanitizers. On PAL A1200, 2 MiB Chip and no
Fast RAM, BASIC passes 603 race and 700 HUD comparisons with zero surface,
particle, immutable-map or HUD mismatches. Final state matches the original
zero-comparison run exactly. Logs are under `tmp/release-render-73f9c03/`.
Remaining track audits and the restored release binary comparison are recorded
separately when complete; this repair alone does not close D-2.

## 2026-09-30 — ship gate D-1 at 73f9c03

Started `make release-check` with an empty `git status --porcelain` at
`73f9c0367914fb501c4f321f3469466f5234693c`. The command completed with exit 0.
Log: `tmp/release-gate-73f9c03.log`.

All prerequisites passed, including timing/physics/lap-limit/collision,
title/Help/key-repeat/loading/font checks, 56,000 Help partial-redraw keys,
30,000 Tracks partial-redraw keys, 378 original Tracks preparation comparisons
and 168 sequential original redraw comparisons. The installer helper passed
197 exact-original outputs, preservation and corrupt/wrong/truncated ZIP tests.

Two clean default Amiga builds produced byte-identical stripped executables.
The subsequent clean packaging build produced the same stripped game hash:
`a25d8cc46aa4f64a2fa807ae7a02d2bfb0529731440cd7f44411161aea6888e8`.
The scratch archive `build/release-check/Slicks-0.1.lha` is 280,990 bytes,
SHA256 `fb004f4bf78c1d3fff16b8df55a678bc3b7eff68b2e5f7499cad9af6ac1796d1`.
Its independent audit passes all 12 allowlisted LH5 members, decompression,
header/payload CRCs and executable/script/icon identity, including version checks.

This closes D-1 only. D-2 rendering/retention checks and D-3 manual installation
and gameplay remain separate gates. This is a scratch candidate, not publication
or a version tag; `dist/` was not replaced. Debug validation remains muted.

## 2026-09-30 — clean-build installer candidate refresh

A clean default-options Amiga rebuild produces the exact same stripped HUNK
and ELF hashes as the WHDLoad regression below. A separate candidate was
created at `tmp/installer-candidate-O0ydKu/Slicks-0.1.lha` (267,646 bytes),
SHA256 `13b5bda2fa2a046b09186ceac8f922a79f0910a1746eda3b68d755f1fe981270`.
The existing `dist/Slicks-0.1.lha` was neither overwritten nor published.

The independent package audit passes all twelve allowlisted LH5 members,
header/payload CRCs, decompression, HUNK checks and exact script/icon/source
identity. No original assets, private keys or emulator/OS material are included.
The package uses the current production slave, not a diagnostic slave.

Real Amiga Installer tests consume this exact LHA and the unchanged publisher
ZIP on PAL 68020 with 2 MiB Chip and no Fast RAM:

| Workflow | Run under tmp/ | Verified result |
| --- | --- | --- |
| Fresh standalone install | `installer-script-8ozngfgu` | Original data, current executable, icons and staging cleanup match. |
| Existing install, Keep, optional WHDLoad | `installer-script-d4tsohzy` | Modified track and settings/key placeholders survive; both launch paths and icon metadata are installed. |
| Explicit Reinstall | `installer-script-9_ib_5o0` | Original track data replaces the modified track; settings/key placeholders survive; staging is removed. |

Only requester answers are supplied by the test harness; extraction, copying,
filesystem operations and native icon changes execute through the real script.
Placeholder files are deliberately not a real registration key.

The fresh installation is then launched unchanged through Execute Play:
`tmp/standalone-release-e7npf7kc` reaches the active native title. A direct
Options/edit/close/reopen/race run on that same installed executable confirms
the default 4096-byte stack and passes at race entry:
`tmp/standalone-release-lpf7judw`. The latter bypasses Play's explicit larger
stack, rather than claiming the launcher itself uses 4 KiB. These are
checkpoint checks, not normal-exit proof; the separate WHDLoad quit check is
recorded below. All test emulators were muted and closed.

Build/package logs: `tmp/release-refresh-build.log`,
`tmp/release-refresh-package.log`. This candidate is verified packaging for
the present build, not a declaration of complete fidelity or resolution of the
outstanding Load Game entry and other open-work policies. Repeat affected
release gates after further production changes before replacing the published
candidate.

## 2026-09-30 — current WHDLoad regression

The stripped executable built from 1d293e2 passes fresh isolated WHDLoad
checks after the menu/cache/title changes. The production, race-test and
exit-test slaves were rebuilt; the latter two supply diagnostic arguments
only and are not release payloads. The original publisher ZIP supplies data
inside each private installation. No registration key is used.

| Workflow | Local run directory under tmp/ | Result |
| --- | --- | --- |
| Production startup, PRELOAD | `whdload-test-fqrar8aj` | Reads original data and reaches the automatic demo before the timed stop. |
| Race, PRELOAD | `whdload-test-ivto1r7s` | Reads original archive/DAT and reaches racing before the timed stop. |
| Normal REGCHECK exit | `whdload-test-qo7ialx2` | WHDLoad reports Return OK; host completion marker passes. |
| Race, no PRELOAD | `whdload-test-yfidkue9` | Live reads of original archive/DAT pass and execution reaches the intentional timed stop. |

Actual AGA copper/bitmap decoding from the startup dump shows the title and
a DEMO-labelled track/HUD; the race dump shows BASIC with updated timers and
effects. This is inspection of native output, not a DOS-frame substitute or
pixel-fidelity oracle. Timed runs intentionally end in WHDLoad's DEBUG dump;
only the separate quit case proves normal return.

Configuration: PAL A1200, 68020, 2 MiB Chip plus 4 MiB Fast, A600 Kickstart
40.063 with matching RTB, locally installed WHDLoad. This retains the existing
WHDLoad memory requirement; it does not claim no-Fast standalone memory limits
or a 4 KiB WHDLoad execution stack. Each run used muted host audio and closed
its emulator. No source/game/ROM/key/dump material was added to Git.

Executable: `tmp/whdload-current-JKq1eh/Slicks`, SHA256
`5f4be6edb92f9ac488c6ae130c8d8983d7a90d3903fe9d3a51057b52a3f79c97`.
Companion ELF SHA256:
`19503a7fd07e3790aed93d768fcab61b03c017de29b5136bc67a3a4ee2ec8f7d`.
Slave build log: `tmp/whdload-current-build.log`. Reproduction uses the
documented `tools/test_whdload.py` modes, `--exe` pointing at that stripped
binary, `--ticks 5000 --seconds 180`, and `--no-preload` for the final case.

This renews the listed WHDLoad workflows for this build. Installer/archive
refresh, unresolved saved-game entry validation and remaining fidelity work
are still open; the existing distribution archive was not replaced or uploaded.

## 2026-09-29 — startup display allocation cleanup

The explicit `DISPMEM` diagnostic tests all ten allocation sites in
`slicks_amiga_platform_create`: bitmap data, bitmap descriptor, copper storage,
Bitmap wrapper and CopperList wrapper for each of the two startup views.
Each case supplies a null allocation at exactly one site; other allocations
and the ordinary failure cleanup execute normally. Wrapper failures skip
construction, matching a null `new` result under the build's `-fcheck-new`.

For each case the target requires failure, consumption of the selected fault,
no active display, and null owning pointers after the internal cleanup.
It calls destroy again, as the outer cleanup may do, and requires total
`AvailMem(MEMF_ANY)` to equal the pre-case value. Scheduling is forbidden
only around each allocation/free check to avoid competing task allocations;
interrupts remain enabled. The checks occur before the real display is
created. Normal launches never enable these faults.

The current stripped build passes all ten cases with a confirmed 4096-byte
entry stack on a stock-speed PAL 68020, 2 MiB Chip and no Fast RAM. It then
creates the real display, enters a demo, pixel-checks both data views,
restores configuration/playlist and exits with system-restoration mask 31.
Evidence: `tmp/standalone-release-lfe6w95v/debug.log` contains
`DISPLAY_ALLOCATION_CHECKS 10` and `DEMO_LIFECYCLE_EXIT_OK`.
Build log: `tmp/display-allocation-build.log`. The run was muted and its
emulator closed. No cleanup defect was found.

This is controlled allocation-failure coverage, not deliberate system-wide
memory exhaustion, a stack high-water measurement, or validation of unrelated
startup resource failures. It does not renew every release workflow for this
binary.

## 2026-09-29 — current title/menu default-stack regression

Two further workflows pass on the same stripped binary and hardware settings
below, both with a confirmed 4096-byte entry stack and normal system restoration:

- Options-owned Help navigation, history return, close and reopen:
  `tmp/standalone-release-cdyjoeo9`. Five ready checkpoints and two closes pass;
  the complete 64,000-byte saved background and final restored chunky image
  compare identically.
- Championship save from a genuine first intermission, including picker
  cancellation/re-entry, name acceptance, catalogue refresh and exit:
  `tmp/standalone-release-z_axl9an`. The shared resident-display assertions
  are expanded by the standalone harness and execute in this run.

The subsequent `CHAMPLOAD` run (`tmp/standalone-release-8y29uof8`) is **not a
pass**. Inspection showed its startup input still sends four Down keys and
Enter to the obsolete title Load Game row; it never reached the saved-game
picker or resume checkpoint. The debugger was stopped and the harness closed
its emulator. Do not rerun this fixture as a release gate until its original
entry route is resolved. No before/after championship-state comparison or
fresh-process resume is established by this batch, and no invented menu row
was reinstated. The private fixture contains the test save for later checks.

2026-09-30, decision D1 (keep Load hidden, as the original does): the
`CHAMPLOAD`, `CHAMPLOADW` and `CHAMPFAIL` fixtures and their debugger scripts
were removed, together with the uncommitted dynamic Load-storage draft
(archived locally as `tmp/load-storage-draft-20260930.patch`). The
unreachable Load handler itself stays, mirroring the original's. After the
removal, `CHAMPSAVE` then `CHAMPEDIT` in one shared run directory passed
(`NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`,
`NATIVE_CHAMPIONSHIP_RESAVE_OVERWRITE_DELETE_CANCEL_OK`; `tmp/b4-{save,edit}.log`).

Current-build full-frame racing audits also pass 600 updates each on F1
(`tmp/standalone-release-cl_xj10f`, 32 actors, 2,068 marks), CITY
(`tmp/standalone-release-o6qr7np5`, 18 actors, 1,480 marks), and WHACKO
(`tmp/standalone-release-whrmplu7`, 5 actors, 1,854 marks). All confirm
the 4 KiB entry stack and live statistics disabled, using the stripped binary
and ELF hashes below. These compare incremental bitplanes against the full
rendering reference; they are not performance measurements or normal-exit
tests. The harness closes each emulator after the successful checkpoint.

The current ELF was converted directly to a stripped HUNK and installed in
a fresh private original-data directory, without overwriting `dist/` or an
existing release candidate. This includes the prepared-title background fix
and tight registered-owner bounds. Executable SHA256:
`9e78a66e4dbedaa31f9f57795075071b8fad0d6d606df25a46d37bfe63784f8f`.
Companion ELF SHA256:
`f7f021b4fcc69ce11eb8d49cabc997eaae231d77c2a4612037f4872b4e48f817`.

All three runs confirm a 4096-byte task stack at entry, with no Stack command,
on PAL A1200/68020 real speed, 2 MiB Chip and zero Fast RAM:

- Normal active title: `tmp/standalone-release-xzvybrv3`.
- Options edit, close, reopen and race handoff:
  `tmp/standalone-release-pjvfrrlc`.
- Unmodified demo entry, both track-data views, restoration of configuration
  and playlist, zero setup saves, and normal system-restoring exit:
  `tmp/standalone-release-33y84fv3`.

The installation is `tmp/release-current-PH0gjX`; it contains copied original
data, not private registration material. Runs were muted and the harness
closed each owned emulator. Title and Options fixtures stop at their stated
checkpoints; only the demo fixture proves normal exit here. These are bounded
stack/memory regressions, not exhaustive high-water measurements, renewed
WHDLoad/installer validation, or completion of the remaining release gates.

## 2026-09-28 — display-end publication pacing

Supersedes the VBlank-at-loop-entry limiter below. Simulation and chunky
rendering now run immediately after the preceding publication. Once ready,
the main race path waits for a **fresh** row `$100` display-end edge, then
updates audio/palette and performs C2P without another synchronization wait.
If preparation finishes during the lower border, it waits for the next edge
rather than publishing late or twice in one refresh. Missed opportunities
do not accumulate catch-up updates. Existing menu and modal timing is unchanged.
The original level-sensitive `wait_display_blank` remains for those callers;
`wait_display_end` is a separate edge-sensitive publication wait.

`diag_display_end_limit.gdb` replaces `diag_vblank_limit.gdb`: it checks
publication timing, not simulation-start VBlank counters. On 2 MiB Chip,
no-Fast, confirmed 4 KiB-stack native runs:

- Accelerated 68040: 120 publications spanning 119 VBlank intervals, all at
  row 256 (`tmp/standalone-release-ed26g3lg`).
- Stock-speed 68020: 120 publications spanning 121 VBlank intervals, at
  rows 256–257 (`tmp/standalone-release-11xv4vz_`).

Repeat using `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack
--args NATURALQB --checks amiga/diag_display_end_limit.gdb
--marker DISPLAY_END_LIMIT_OK`, adding `--cpu 68040 --cpu-speed max` for the
accelerated case. Use a disposable installation for the test's game saves.

The required stock-speed, live-statistics-off full-frame display audits pass
600 updates each: F1 (`tmp/standalone-release-otwrfnx1`), CITY
(`tmp/standalone-release-5sxrw5qw`) and WHACKO
(`tmp/standalone-release-kjjbcn4o`). All sessions are muted and closed on exit.

These breakpoint-observed timing checks establish phase and rate, not an
uninterrupted performance benchmark or proof that C2P always fits inside the
blanking window. The single-buffered design and outstanding worst-frame
performance target are unchanged. The publication wait remains excluded from
measured CPU work; end-to-end cadence includes it.

## 2026-09-28 — gameplay refresh-rate limiter

The main loop now permits at most one iteration per new vertical-blank count
while racing. The existing display-blank wait only protects visible DMA writes:
it returns immediately throughout the lower border and therefore could permit
multiple updates in one refresh on a fast CPU. Menus retain their unconditional
VBlank wait. Updates that already cross a VBlank incur no additional wait;
missed refreshes do not accumulate catch-up updates. The limiter is outside
the measured work region. Simulation and rendering code are unchanged.

`diag_vblank_limit.gdb` observes 120 consecutive real simulation updates and
rejects any repeated VBlank counter. Muted A1200 tests with 2 MiB Chip, no Fast
RAM and the confirmed default 4 KiB task stack passed:

- 68040, maximum CPU speed: 120 updates spanning 120 VBlanks,
  `tmp/standalone-release-77armh10`.
- 68020, real CPU speed: 120 updates spanning 122 VBlanks,
  `tmp/standalone-release-fo5cp1dn`.
- Options edit/reopen/race entry (`tmp/standalone-release-9rqn9w4c`) and
  championship save/normal system-restoring exit
  (`tmp/standalone-release-nz430lqg`) also pass on the stock configuration.

These short pacing checks include countdown/startup; they do not establish
that all gameplay now meets the outstanding 20 ms performance target.
Repeat with `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack
--args NATURALQB --checks amiga/diag_vblank_limit.gdb --marker VBLANK_LIMIT_OK`
and optionally `--cpu 68040 --cpu-speed max`.

## 2026-09-28 — default 4 KiB stack verification

The unchanged stripped release executable was launched directly, bypassing
Play's conservative `Stack 16384`. No Stack command was used. Read-only GDB
checks at main entry confirmed `tc_SPUpper - tc_SPLower == 4096` in every run.
Configuration: PAL A1200, 2 MiB Chip, no Fast RAM; debug audio muted.

Passed paths and local-only evidence directories (`tmp/standalone-release-*`):

- Normal title/startup: `f8z8wiak`.
- Options edit, close, reopen and entry into racing: `yn4541t_`.
- Championship menu save and normal system-restoring exit: `49dsjbiw`.
- Fresh-process championship reload/resume, advancing the race and normal
  exit: `l90qfbku`. Saved/restored points, cash, inventory and vehicle dumps
  also compare byte-for-byte.
- Help-menu navigation, history, close/reopen and system-restoring exit:
  `xzeu7a54`.
- F1 racing through 600 updates with the full-frame dirty-sprite audit:
  `er198zfr` (`NATURALO1Q`, `diag_dirty_sprites.gdb`).

The executable SHA256 is
`fc6b9d7db5ee6253ce716778a6d9d04815b7ed107c51dc7cdcdd1a4637e60b24`,
identical to `build/release/Slicks`. No game rebuild or code change was needed.
This verifies the listed workflows, not an exhaustive maximum-stack bound.
An attempted debugger-written watermark failed its immediate write/readback
check and was discarded; it is not evidence of a game stack overflow or a
valid high-water measurement. A repeated save fixture with an already existing
test save and an incorrect help launch mode were also discarded and rerun
with the correct clean fixture/mode.

Repeat using `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack`;
add `--args OPTIONS --checks amiga/diag_options.gdb
--marker OPTIONS_ENTRY_EDIT_RETURN_REOPEN_RACE_OK` for the options path.
Use a disposable installation: scripted checks really write saves/records.
The harness isolates debugger dumps and closes its own emulator on success
and failure. The existing package/launchers are unchanged by this verification.

## 2026-09-28 — installer release 0.1

The distribution is now `dist/Slicks-0.1.lha`, not the historical developer ZIP
below. Instructions/build details are in [install-original-data.md](install-original-data.md)
and [whdload.md](whdload.md). This is packaging/validation, not release publication.

- Downloaded the exact publisher Slix151.zip specified by the user. Native
  helper extraction matches an independent ZIP-library reader for both data
  files and all 195 tracks. Both core files also match development originals.
- Host rejection tests cover missing, empty, truncated, extended and altered
  archives and an existing destination containing settings/key placeholders.
  No settings/key are replaced, selected or shipped.
- Native helper: PAL A1200, 2 MiB Chip, zero Fast RAM, 4 KiB stack. All 197
  outputs match independently; 1,246 bytes of stack remain unused. Partial
  staging is cleaned. Actual user-key bytes are not test inputs.
- Real Amiga Installer 43.3: fresh standalone installation directly from the
  LHA candidate plus original ZIP passes on the same 2 MiB/no-Fast machine.
  Only requester answers are supplied deterministically; helper execution,
  filesystem operations, copying and native icon updates run unchanged.
  Existing-install Keep with optional WHDLoad preserves settings/key and
  produces both launch icons. Native project-icon default tools/tooltypes
  are checked (IconX and WHDLoad/SLAVE/PRELOAD).
  Explicit Reinstall replaces a deliberately modified original track while
  retaining settings and key placeholders; staging is removed after success.
- The installed, unchanged Play script runs SetPatch, sets the stack, enters
  data/ and reaches the native active title display on a 2 MiB/no-Fast A1200.
  Its executable is the stripped release executable, not a test replacement.
- WHDLoad 19.2.6941/A600 Kickstart 40.063: production-slave normal startup;
  real racing with PRELOAD on 4 MiB Fast RAM; live reads with PRELOAD disabled;
  normal REGCHECK exit returns OK. Diagnostic-only slave arguments select
  racing/exit without altering the production game. Actual AGA bitplanes and
  palette decoded from dumps show the native title and race/HUD/effects.
  WHDLoad is not claimed to work without Fast RAM.
- A clean default-options Amiga rebuild has a byte-identical stripped HUNK
  payload to the tested release candidate. Only discarded debug-symbol
  padding differed in the unstripped executable.
- Independent Lhasa extraction verifies every member against source, both
  header and payload CRCs, exact 12-member allowlist, LH5 format, HUNK binaries
  and reference icons. No private key, original archive/data, DOS program,
  ROM, RTB, WHDLoad binary, host configuration, dump or screenshot is included.
- All owned debug runs are muted and terminate their own emulator. Existing
  run.sh remains audible. The source tree contains no original or private files.

Limits: PAL stock-A1200 standalone and the stated emulator WHDLoad configuration
were tested, not all accelerators/ROMs/filesystems. Hardware joystick checks
remain deferred by the user. Gameplay still does not meet the 50 FPS target.
Registration's interactive exit-help/optional missing-image checks remain on
the open list; packaging does not reclassify them as completed.

## 2026-09-26

- Runtime dependencies: AmigaOS DOS/utility/keymap libraries version 37 and
  graphics.library version 39, AGA, the supplied original archive/DAT/tracks,
  and SetPatch from the user's AmigaOS installation. No PC executable, host
  emulator, SDL or external audio mixer is needed by the native binary.
- Build dependencies remain the documented shared Amiga GCC/vasm/elf2hunk
  toolchain and generated inputs derived locally from the user's executable.
  FS-UAE and the shared process helper are development-launcher dependencies,
  not dependencies distributed inside the native release.
- Runtime memory: all six mode-transition tests passed with exactly 2 MiB chip
  and no Fast RAM. A fresh current-build race entry also passed; the target
  compiler reports a 202,702-byte race structure. The native shop saved two
  screenshots with an additional temporary 65,078-byte allocation on the same
  configuration. These checks establish tested configurations fitting, not a
  claim that arbitrary Workbench memory pressure or every asset combination fits.
- Fresh BASIC GO preparation measured 142 OS ticks (2.84 seconds), excluding
  the starting-light countdown. Evidence: `tmp/release-startup.log`, fixture
  `amiga/diag_release_startup.gdb`; no per-frame debugger stops were used.
- Launcher defaults retain PAL A1200, 2 MiB chip and no Fast RAM. `run.sh`
  keeps audio enabled; scripted debugging uses dummy host audio and closes its
  owned emulator. `run.sh` now consistently honors `FSUAE_RUN` for both the
  mounted directories and process ownership instead of splitting those paths.
- Packaging uses an exact four-file allowlist: native HUNK executable, README,
  credits and a SHA-256/source-revision manifest. It checks HUNK headers and
  archive membership/content, refuses replacement of an existing archive,
  and never traverses the original data or generated reference directories.
  The source tree tracks no files under `ref`, `tmp`, `src/gen`, `amiga/out`
  or emulator run directories. Runtime data and OS/ROM files must be supplied
  separately; no license for those assets is implied.

At that revision a local archive was built with `make release-package`. A fresh output path
used `RELEASE_ARCHIVE=build/release/<name>.zip`. Those instructions are historical;
use the current installer build instructions above. No upload/push is implicit.

The verified local package is `build/release/slicks-749bb11.zip`, built from a
clean working tree. Its executable is 408,344 bytes; declared HUNK allocations
total 389,316 bytes (excluding runtime allocations and OS memory). All four
archive entries were reread and compared to their source content. No release
was uploaded. Integration/release open items 1–3 are now complete within the
native-port scope; general translator expansion remains explicitly deferred.
