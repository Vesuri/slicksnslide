# Open work — ship list

Updated 2026-10-01. This is the complete, finite list of work between now and
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

- [ ] **B14. Batch WHDLoad saves:** the manual production-icon test now exits
  successfully, but standings continuation and Quit each trigger about 16
  separate disk-access periods of roughly five seconds. Eliminate the repeated
  switches without reintroducing the cached-new-file hang or silently raising
  the documented memory requirement. Verify both record and setup saves.

B1–B13 are closed; evidence is in [frame-pacing.md](frame-pacing.md),
[fidelity-audit.md](fidelity-audit.md) and
[release-verification.md](release-verification.md).

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
  **Resume point:** standalone race/intermission/save/Workbench-exit retry
  passed on 2026-10-01. Complete the production-icon WHDLoad passes in step 7,
  with and without PRELOAD. Automated WHDLoad launch/exit checks already pass
  but do not replace those manual race checks.
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
