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
Reinstall/Keep data prompt already covers updates.

Version: 0.1 (30.09.2026), chosen by the user, in all `$VER` strings.

## D. Release gate (run once, after B and C)

- [ ] **D-1.** Run `make release-check` from a clean tree and confirm it passes.
- [ ] **D-2.** Run F1, CITY and WHACKO full-frame display audits on the release
  candidate, plus the `make -C amiga RETCHECK=1` retention check, and confirm
  both pass.
- [ ] **D-3.** Do one manual FS-UAE session on a stock PAL A1200 configuration
  (68020, 2 MiB Chip, no Fast RAM), in this order:
  1. install from `Slix151.zip` with the Installer;
  2. start with `Play`;
  3. let the idle demo run, then return;
  4. start one race and finish it;
  5. save a championship at intermission;
  6. quit to Workbench;
  7. repeat steps 2–4 and 6 from the WHDLoad icon, with and without PRELOAD, at
     the memory configuration `docs/whdload.md` documents.
- [ ] **D-4.** Build the archive, record the hashes in
  [release-verification.md](release-verification.md), and tag the version in git.

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
