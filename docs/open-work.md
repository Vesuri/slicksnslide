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

## B. Fixes

### B1. Real-time race clock

In `slicks_physics_clock_advance` (`src/game/race_timing.h`) and
`next_physics_ticks` (`race_runtime.c`):
- add 1193182/50 PIT input cycles per vblank elapsed since the previous update,
  instead of a fixed 1193182/50 per update;
- keep the carried phase;
- cap each update at 45 ticks (original `fe6d..fea3`);
- reset the reference vblank wherever the original resets its clock (`fe3c`,
  before the lights) and after every non-race interruption (pause/Help, results).

The fixed per-update clock stays behind the existing benchmark and verification
switches, so `FINAL_STATE` fixtures remain deterministic.

Accept when all of these hold:
- host timing tests pass for 1, 2, 3 and 50 elapsed vblanks, including the cap;
- on F1, a lap timed by the vblank counter and by the HUD lap clock agree
  within 1%;
- deterministic benchmarks reproduce their current `FINAL_STATE` values.

See [frame-pacing.md](frame-pacing.md). Today F1 cars and lap clocks run at 74%
of real time.

### B2. Adaptive publication

In the race loop (`slicks_diag.c`, around line 6915):
- if no vblank has passed since the previous publication, wait for the
  display-end edge exactly as now;
- otherwise publish immediately.

Record late publications in a counter that `bench_tracks.sh` prints.

Accept when all of these hold:
- the four-track benchmark shows cadence within 1 fps of the
  `tools/sync_policy_model.py` prediction (≥ 48.9 / 49.8 fps on F1 / WHACKO;
  50.0 on BASIC and CITY);
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
- The registration order-form image. The original `webf_ord.bmp` is absent.
- The manual joystick test. It is deferred by the user.
- General translator expansion. See [phases.md](phases.md).
