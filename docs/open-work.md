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

### B6. Held-key auto-repeat in menus, Help and name entry

**The original reader.** The latched-scan reader `36ce0` works like this:
- DS:1714 is the latch. The keyboard interrupt `36e29` stores every raw
  byte there, make and break alike.
- While the latch is 0x80 or above, the last-event time follows the BIOS
  tick count.
- On a fresh hold it returns the scan once.
- After that it returns the scan whenever `ticks > last + arg`, then sets
  `last = now`. So a held key repeats every `arg+1` BIOS ticks (18.2065 Hz).
- The state is global: a key held while one dialog opens another stays
  armed.
- Any other make or break, including Shift, replaces the latch.
- `36ca5` (at 1e18e, 1e53f, 1e72f, 247c9, 2497a, 24b50, 2da5e, 2df59, 2f37e,
  2f6aa, 31157 and 3175d) clears it to 0x80.

The Amiga keyboard interrupt only queues make and break events. No owner
repeats a held key.

| Call | Arg | Original owner | Amiga owner (`slicks_diag.c` unless noted) |
| --- | --- | --- | --- |
| 1e11e | 2 | Race Speed dialog | `slicks_amiga_race_speed_key` (`amiga_player_menu.c`) |
| 1e65e | 3 | Pause menu | `slicks_race_menu_key` in `run_race_pause` |
| 24704 | 3 | Intermission menu | `run_intermission` |
| 24a5f | 2 | Change Cars | `slicks_amiga_change_cars_key` |
| 26b6d | 7 | Track info preview (closes on any key) | Tracks preview |
| 274db | column+1 | Tracks selector | `slicks_track_menu_key` |
| 27f55 | 3 | Profile editor | `slicks_profile_editor_key` |
| 28993 | 2 | Players menu | `slicks_player_menu_key` |
| 293f1 | 2 | Options | `slicks_options_menu_key` |
| 2a378 | 2 | Title | `slicks_dispatch_title_key` |
| 2cfce | 2 | Shop | `run_shop` |
| 2dd8b | 2 | Controllers (menu and pause) | controllers key handlers |
| 2f5ba | 3 | Colour picker RGB edit | `slicks_colour_picker_key` |
| 315b2 | focus_actions*4+2 | Shared list dialog (player picker, saves, track lists) | list dialog handlers |

**Help and name entry** repeat through the PC keyboard's own typematic
instead:
- Help sets DS:1713=0xff at 3295d. It then chains to BIOS INT 9 and reads
  keys with `getch`.
- Name entry sets DS:1713=1 at 2f7a9.
- The race, the `36d8b` users (key capture and message waits) and `36d65`
  do not repeat.

**Implementation:**
1. `src/ui/key_repeat.h`, an exact port of `36ce0`: latch, last, armed.
2. Platform additions:
   - a raw latch written by `keyboard_handler` and initialised to 0x80;
   - a BIOS tick clock advanced per vblank (1193182 per 50 Hz frame against
     65536);
   - `slicks_amiga_platform_repeat_key(platform, arg, &raw)`;
   - `slicks_amiga_platform_clear_latch()` at the `36ca5` sites.
3. Each of the 14 owners calls `repeat_key` with the argument above once its
   queue is empty.
4. Help and name entry get a typematic model: first measure the delay and
   rate in the DOSBox reference, expected to be about 500 ms then 10.9/s.

**Accept when all of these hold:**
- a Unicorn test runs the original `36ce0` over scripted latch and tick
  sequences (arguments 0..7, hold, release, second key, Shift, word carry) and
  matches `key_repeat.h` on every return and every state;
- each owner's argument is confirmed by its existing dispatch oracle, where
  one exists;
- one native fixture holds Down on the title, in Tracks column 0 and in a list
  dialog's actions, and matches the host model's step counts.

## C. Packaging

Follow Vette's layout (`~/Documents/Vette`: `Makefile` `dist`/`release-check`,
`tools/check_release.py`, `release/{Install,ReadMe}`,
`src/platform/amiga/version.s`). Keep the Slicks-specific extras: the
standalone `Play` IconX launcher next to the WHDLoad icon, `CREDITS.txt`,
`puff-license.txt`, and data installed from the user's `Slix151.zip`.

- [ ] **C1. `$VER` string.** Add `src/platform/amiga/version.s` holding
  `$VER: Slicks <VERSION> (dd.mm.yyyy)` in a retained section, and link it into
  the release executable. Add `; $VER: Install <VERSION> (dd.mm.yyyy)` as line 2
  of `release/Install`. Choose the version number: `VERSION` says 0.1.
- [ ] **C2. `tools/check_release.py`.** Assert:
  - the three `$VER` strings (game, slave, Install) match `VERSION`;
  - `APPNAME=Slicks`, with no other project names in `Install.info`;
  - `MultiView` in `ReadMe.info`;
  - the slave tooltype and the `(tackon #parent "Slicks")` line in `Install`;
  - the ReadMe section headings and the GitHub URL.
- [ ] **C3. Makefile.**
  - `release: dist`.
  - `dist` does a clean rebuild of `amiga/` rather than depending on the
    incremental build.
  - `release-check`: host regression suite, `tools/install-data` test,
    two-build SHA-256 determinism check, package, `check_release.py`.
  - A top-level `install-data-test` target.
- [ ] **C4. `release/ReadMe`.** Use the Vette/RoF WHDLoad-install layout, with
  the current content reformatted:
  1. disclaimer, then "This install applies to…";
  2. Requirements, Installation, Display, Quitting, Features, Controls,
     Registration;
  3. History (`version <VERSION> (<date>): initial Amiga release`);
  4. Contact (github.com/Vesuri/slicks, whdload.de);
  5. the helper licence.
- [ ] **C5. Installer parity.**
  - Add the "drawer exists: Remove / Keep" prompt, keeping keys and saves by
    default.
  - The slave uses kickemu, so add Vette's Kickstart 3.1 image/`.RTB` check and
    warning.
  - Make the WHDLoad icon's stack consistent with the slave.
- [ ] **C6. Housekeeping.** Delete the stray `dist/{display-end,registration-verified,release,vblank-limit}-*`
  directories. Add a Release section (`make dist`, `make release-check`, tools
  needed) to the development docs.

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
