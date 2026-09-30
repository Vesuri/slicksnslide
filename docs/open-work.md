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
- [ ] **D4 — Speed change during a race.**
  - The original's pause-menu Speed child reprograms the PIT through `7bc2`,
    which zeroes the tick counter at `74bc`. The race loop's saved counter
    (BP-58) is not reset.
  - As a result, the next update's batch is negative (`fe73..fe8b` compares
    signed values, and the cap only limits the positive side). It runs no
    physics substeps, but it subtracts time from the game clock and adds it
    back to the countdown.
  - The port restarts the phase and carries on with a positive batch.
  - Choose one: reproduce the glitch, or keep the clock monotonic as an
    explicit adaptation.

## B. Fixes

### B2. Adaptive publication: user acceptance

This is implemented and measured ([frame-pacing.md](frame-pacing.md)). The one
remaining step is for the user to watch one F1 race on the normal build
(`amiga/run.sh`) and accept the tearing on late frames.

If they do not accept it, the only follow-up is to convert dirty regions below
the beam first. Do not add double buffering.

### B3. Loading screen: one visual confirmation

This is implemented and verified ([title-and-loading-verification.md](title-and-loading-verification.md)).
The one remaining step is for the user to look once during a track load and
confirm the tinted panel and caption on screen, not Workbench.

### B5. Sparse-shop fixture

Run one native fixture with only driver 3 human: buy one item, sell one item,
press one ignored key. Accept when there is no crash, the transactions land on
driver 3, and the ignored key triggers no redraw.

### B6. Typematic repeat in Help and name entry

Menu repeat through the original `36ce0` reader is done
([fidelity-audit.md](fidelity-audit.md), "Held-key repeat").

Help (3295d) and name entry (2f7a9) instead chain to BIOS INT 9 and repeat
through the PC keyboard's typematic.

Steps:
1. Measure the delay and rate in the DOSBox reference by holding a key in
   Help and counting the characters produced.
2. Generate matching repeat makes in the platform queue, only while those two
   owners are active.

Accept when a native fixture that holds a key in Help, and one that holds a
key in name entry, match the measured counts.

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

- [ ] **C1. Version number.** The user chooses the release version (currently
  `0.1`). Set it in `VERSION`, `src/platform/amiga/version.s`,
  `whdload/SlicksSlave.s`, `release/Install` and the ReadMe history. Then
  run `make release-check`.

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
