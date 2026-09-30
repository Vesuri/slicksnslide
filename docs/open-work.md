# Open work — ship list

Updated 2026-09-30. This is the complete, finite list of work between now and
release. Close each item exactly as written, commit it, and tick it off.

**Adding an item requires a demonstrated defect.** That means a reproduced
difference from the DOS original, a crash, or a visible fault. Open-ended
"verify more paths" items are not ship work. Earlier coverage items without a
known defect were removed on 2026-09-30. Their evidence stays in
[menu-cache-verification.md](menu-cache-verification.md),
[fidelity-audit.md](fidelity-audit.md) and
[release-verification.md](release-verification.md).

Working rules are in [development-verification.md](development-verification.md).

## A. Decisions (made 2026-09-30)

- **D1:** Load Game stays hidden, as in the original. Saving at intermission
  stays. Done: the Load fixtures and the Load draft were removed
  ([release-verification.md](release-verification.md)).
- **D2:** Sparse shop uses safe active-player mapping (already implemented;
  see B5).
- **D3:** A negative saved language selector means English. Done: recorded in
  `slicks_diag.c`.
- **D4:** A speed change during a race keeps the game clock monotonic, an
  explicit adaptation of the original's negative-batch glitch. Done: recorded
  in `slicks_race_set_timer` and [fidelity-audit.md](fidelity-audit.md).

## B. Fixes

B1–B9 are closed; evidence is in [frame-pacing.md](frame-pacing.md),
[fidelity-audit.md](fidelity-audit.md) and
[release-verification.md](release-verification.md).

- [ ] **B10. Preserve the game display during disk access:** The manual race
  completion briefly exposed Workbench before displaying the results screen.
  Keep the current game image visible while loading/saving, while allowing the
  OS services required for disk I/O. Remaining boundaries:
  track-info/record-clear operations, screenshot writes, registration
  images, setup-save failure recovery and nonfatal race-preparation failures.
  Audit remaining teardown calls and verify safe ownership/restoration on exit.

- [ ] **B11. Intermission fails in stock-A1200 manual session:** After the
  completed race, `ENTER: RETRY / ESC: END MATCH` appeared instead of the
  intermission menu. This is `run_intermission`'s unavailable/error path, not
  the ordinary match-end prompt. Track selection is 195/195 and mode is
  Classic. Isolated Workbench-loaded 2 MiB/4 KB-stack reproduction identifies
  the 65,536-byte preview arena allocation in `slicks_amiga_intermission_open`
  as the failure, after data and menu allocations succeed. Preview now borrows
  the dead VGA race image's startup allocation; stock-2-MiB intermission/edit/
  next-race and pixel-publication checks pass. Finish championship save and
  end-to-end release validation with this build.

- [ ] **B12. Startup-owned runtime memory:** User requested reserving all
  game-owned runtime storage before entering the game, with a clear launch
  failure when it cannot fit. Replace late allocations with bounded, reusable
  workspaces whose simultaneous lifetimes are proved. Cover menus and nested
  dialogs, preparation, persistence, catalogue growth and ending screens;
  preserve disk-error recovery. Audit remaining OS allocation calls after
  startup and verify cleanup. Inventory/design: [memory-lifetimes.md](memory-lifetimes.md).

## C. Packaging

Done 2026-09-30 in the Vette layout ([install-original-data.md](install-original-data.md)):
- `$VER` strings;
- the stricter `check_release.py`;
- the `release`, `dist` and `release-check` targets;
- the ReadMe in WHDLoad-install format;
- the installer's Kickstart/RTB warning;
- the `dist/` cleanup.

Vette's "remove existing drawer" prompt is deliberately not copied: the Slicks
drawer holds the user's key, profiles and championships, and the existing
Reinstall/Use existing data prompt already covers updates. The installer now
creates only the standard Slicks WHDLoad icon; standalone runs `data/Slicks`
directly, without a Play script or stack increase.

Final release version: **0.90 (30.09.2026)**, requested by the user. The current
validation candidate still says 0.1; apply the final version after D-3 passes.

## D. Release gate (run once, after B and C)

- [x] **D-1.** Rerun `make release-check` from a clean tree after the shutdown
  allocation fix and confirm it passes.
- [x] **D-2.** Run F1, CITY and WHACKO full-frame display audits on the release
  candidate, plus the `make -C amiga RETCHECK=1` retention check, and confirm
  both pass.
- [ ] **D-3.** Do one manual FS-UAE session on a stock PAL A1200 configuration
  (68020, 2 MiB Chip, no Fast RAM), in this order:
  1. install from `Slix151.zip` with the Installer;
  2. start standalone from its data drawer with `Slicks`, default 4 KB stack;
  3. let the idle demo run, then return;
  4. open Players, then start one race and finish it;
  5. save a championship at intermission;
  6. quit to Workbench;
  7. repeat steps 2–4 and 6 from the WHDLoad icon, with and without PRELOAD, at
     the memory configuration `docs/whdload.md` documents.
  **Resume point:** retry the corrected build after a natural demo, open
  Players, and continue the race/save/quit checklist. Automated cleanup,
  allocation-failure, repeated-launch and default-4-KB-stack checks are now
  covered; they do not replace the user's manual session.
- [ ] **D-4.** Change VERSION and all game/slave/Installer/ReadMe version strings
  to **0.90 (30.09.2026)**, rebuild and audit **Slicks-0.90.lha**, record the hashes
  in [release-verification.md](release-verification.md), and tag the version in git.
  Follow the sibling WHDLoad packages: installer drawer/icon, native executable,
  production slave, game icon template, Install/ReadMe and their icons, data
  extraction helper, credits and required licences. Exclude Play scripts,
  original game data, keys, saves, ROMs and diagnostic binaries.

## Not in ship scope

- Worst-case 20 ms updates. With B1 and B2, game speed is correct at any frame
  rate. See [frame-pacing.md](frame-pacing.md) for the profile and why further
  micro-optimization was stopped.
- The rate of work done once per drawn update: particle ageing, actor
  animation and the homing turn step. On DOS this rate is CPU-dependent: the
  title runs at 18–55 ms per update between 100k and 12k DOSBox cycles, and a
  fast reference PC averaged about 70 updates/s. The Amiga's up to 50 updates/s
  falls inside that range, so it matches a slower PC rather than being a
  defect. B1 makes the physics and clocks independent of it.
- The registration order-form image. The original `webf_ord.bmp` is absent.
- The manual joystick test. It is deferred by the user.
- General translator expansion. See [phases.md](phases.md).
