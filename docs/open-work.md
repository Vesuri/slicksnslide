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

### B2. Adaptive publication

In the race loop (`slicks_diag.c`, around line 6915):
- if no vblank has passed since the previous publication, wait for the
  display-end edge exactly as now;
- otherwise publish immediately.

Record late publications in a counter that `bench_tracks.sh` prints.

Accept when all of these hold:
- the real-time four-track benchmark (`SLICKS_REALTIME_CLOCK=1
  amiga/bench_tracks.sh`) shows cadence at or above the
  `tools/sync_policy_model.py` prediction from `tmp/b1-realtime-*` (46.8 fps
  on F1, 49.5 on WHACKO, 49.9 on BASIC, 50.0 on CITY);
- `diag_display_end_limit.gdb` is updated to accept late publications and
  still forbids more than one publication per update;
- the late-audio check below passes;
- the user watches one F1 race and accepts the tearing.

If they do not accept it, the only follow-up is to convert dirty regions below
the beam first. Do not add double buffering.

**Late-audio check.** Run an F1 benchmark with audio enabled and the existing
`diag_audio_restart.gdb` checks. Mid-frame effect starts must play and retire
through the VBI staging. If they do not, keep effect dispatch at the next VBI
and publish only the picture late.

### B3. Original loading screen during track loads

In `prepare_race` (`slicks_diag.c:2463`), replace `slicks_amiga_platform_end`
with this sequence:
1. keep the display;
2. paint the original panel with `src/ui/loading_presentation.h` and
   `slicks_loading_caption`, using the track stem and `kirj.@f`;
3. publish it once;
4. call `slicks_amiga_platform_begin_io`, load, then call `end_io`.

Constraints:
- Do not publish chunky between `begin_io` and `end_io`: chunky storage is
  borrowed as the decoding arena.
- The same path serves demos, GO/F9, retry and next-track.

Accept when all of these hold:
- one normal race entry and one demo pass their existing race-start checks;
- the SETUPG late-failure/retry fixture still recovers;
- the user looks once at a load (`diag_loading_io_visual.gdb`) and sees the
  panel, not Workbench.

Out of scope: the other disk boundaries (intermission preview, records, cup
image, setup save). They keep today's OS hand-off.

### B5. Sparse-shop fixture

Run one native fixture with only driver 3 human: buy one item, sell one item,
press one ignored key. Accept when there is no crash, the transactions land on
driver 3, and the ignored key triggers no redraw.

### B6. Held-key auto-repeat in menus and Help

The original repeats held keys:
- the title/menu reader through its repeat timer (DS:1714 plus BIOS ticks,
  argument 2);
- Help through `getch` with BIOS typematic repeat.

The Amiga keyboard interrupt queues raw make/break events and has no repeat
([fidelity-audit.md](fidelity-audit.md) around line 939, "keyboard repeat
cadence not established"; `amiga_platform.cpp` `keyboard_handler`).

Steps:
1. Recover the exact delay and rate from the original title reader and its
   callers.
2. Use the BIOS default typematic for Help (500 ms delay, 10.9 repeats/s).
3. Generate the repeats in the platform key queue only for the owners that
   repeat in the original. The race does not, because it polls key state.

Accept when all of these hold:
- a host test of the repeat schedule passes;
- one native fixture holds Down on the title and in Help and shows the same
  number of steps as the recovered schedule.

### B7. First-run language chooser under WHDLoad and Play

The startup chooser (saved language 0) is a console text menu. With no
interactive console, the game refuses to start.

Check the language byte of the `SLICKS.CFG` that the installer produces from
`Slix151.zip`:
- if it is not 0, record that and close this item;
- if it is 0, start once from the WHDLoad icon and once from `Play`. Any start
  that refuses to run is a defect to fix here.

### B8. Mouse audit

The original calls INT 33h (reset, status, position, bounds, cursor;
[external-surface.md](external-surface.md)). The title and Help are proven
keyboard-only, but the other consumers were never audited.

List every INT 33h call site in `disasm/live-listing.txt` and its consumer.
Close this item if none drives menu or game input; otherwise add one fix item
per consumer.

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
