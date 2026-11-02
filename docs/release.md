# Release

How a release is built and checked, and the record of each release. The
dated development audit that led to 0.90 is in git history
(`docs/release-verification.md` before the 0.90 clean-up).

## Building and auditing the package

```sh
make release-check                      # full gate from a clean tree
make dist                               # dist/SlicksNSlide-$(cat VERSION).lha
python3 tools/check_release.py dist/SlicksNSlide-$(cat VERSION).lha
```

`make release-check` runs the release host checks (`RELEASE_HOST_CHECKS`), the
installer helper's extraction test against the 197 original files, builds the
game twice from clean and requires a byte-identical stripped executable, then
builds and audits the package. `tools/check_release.py` decodes every LH5
member independently with Lhasa and checks the allowlisted contents, CRCs,
HUNK headers, icons, Installer tooltypes, `$VER` strings against `VERSION`,
and that no original data, key, save, ROM or diagnostic binary is included.

The package follows the sibling WHDLoad packages: a `SlicksNSlide` drawer with its
icon, the native `SlicksNSlide` executable, `SlicksNSlide.slave`, the game icon template
`SlicksNSlide.inf`, `SlicksNSlideInstallData`, `Install`/`ReadMe` and their icons,
and `puff-license.txt`. Credits are included in ReadMe, not a separate file.

Version strings live in `VERSION`, `src/platform/amiga/version.s`,
`whdload/SlicksSlave.s` (`slv_info` and `$VER`),
`tools/install-data/io_amiga.c`, `release/Install` and the ReadMe history.

## Release gate

Before tagging, on the release candidate:

1. `make release-check` from a clean tree.
2. F1, CITY and WHACKO full-frame display audits on the stripped candidate
   (`SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3
   ./debug.sh "" diag_dirty_sprites.gdb`), and the `make -C amiga RETCHECK=1`
   retention check with `diag_retention_check.gdb` and
   `SLICKS_DEBUG_FAST_KB=2048` on BASIC, F1, CITY and WHACKO; every mismatch counter must be zero.
3. WHDLoad regressions (`tools/test_whdload.py --mode records`,
   `championship`, `championship-edit`, `race`, `quit`) at 8 MiB Fast with
   PRELOAD and 4 MiB without, and `timed`, `race` and `quit` on `--setup a1200-040` and
   `--setup a1200-030`, the 030 with and without `--whdload-mmu`, and `race`
   and `quit` with `--ntsc`; see [whdload.md](whdload.md).
4. One manual FS-UAE session on a stock PAL A1200 (68020, 2 MiB Chip, no Fast
   RAM): install from `Slix151.zip` with the Installer; start standalone from
   the data drawer with the default 4 KB stack; idle demo; Players; one race;
   save a championship at intermission; quit to Workbench. Repeat the launch,
   race and quit from the WHDLoad icon at 8 MiB Fast with PRELOAD and 4 MiB
   without.

## 1.0 (02.11.2026)

- Full version: all shop items, no registration reminders or order form,
  authorized by the original author Timo Kauppinen.
- Correct player helmets and option badges on the title screen.
- WHDLoad runs the game in Fast RAM with CPU caches enabled; it needs 8 MiB
  Fast RAM, or 4 MiB without PRELOAD.
- PAL and NTSC: the beam and crystal are measured at startup; game speed and
  sound pitch match PAL on 60 Hz NTSC, with the picture on lines `$2c..$f3`
  ([rendering.md](rendering.md#video-standards)).

The end-user changelog is included in `release/ReadMe`. Version strings and
archive timestamps use 02.11.2026.

Package `SlicksNSlide-1.0.lha`, 281,804 bytes, ten archive members.

| File | SHA-256 |
|---|---|
| `SlicksNSlide-1.0.lha` | `71e3f658170ff4c824900511b8a3d367627efd3178b9df80c2608b14921e29e7` |
| `SlicksNSlide` (stripped game) | `9a37268a752bc565e905b8532595bf387f51fadcee825d0fbf2981adbfd6b39d` |
| `SlicksNSlide.slave` | `2ac40754fb8e8667faae1231948eb1c49aa18a96849769c7d9905670ec89fab1` |
| `SlicksNSlideInstallData` | `b34ff6ee203f6cd295eb00716a3314cede5fa3c727e5493e0e7f7976cad5cb5d` |

Gate results on the 1.0 code (2026-11-02):

- `make release-check` passes: deterministic stripped executable, installer
  helper extraction of the 197 originals, and the package audit. A second
  `make dist` reproduces the archive byte for byte.
- F1/CITY/WHACKO display audits pass (600 updates with 32 actors, a
  completed 554-update race, 600 updates with 5 actors). RETCHECK passes on
  BASIC, F1, CITY and WHACKO: 700 race and 700 HUD comparisons each, 650 and
  699 geometry comparisons on F1 and CITY, all counters zero.
- WHDLoad on the packaged executable and slave: records, championship,
  championship-edit, race and quit pass on a 68020 at 8 MiB Fast with PRELOAD
  and at 4 MiB without (at most two OS switches per save without PRELOAD,
  none with it); timed, race and quit pass on `a1200-040` (CACR `80008000`),
  `a1200-030` (CACR `2011`) and `a1200-030` with `MMU` (CACR `2111`, TC
  translating); race and quit pass on an emulated NTSC A1200 with the
  `$2c..$f3` window in every copper list.
- The real-Installer package test passes on 2 MiB Chip: expected data,
  executable, one WHDLoad icon, preservation and cleanup.
- Gate step 4, the manual session, was not run for 1.0. WHDLoad's `NTSC`
  tooltype itself is untested (no NTSC monitor driver in the fixture), and the
  4 MiB PRELOAD hang remains open ([open-work.md](open-work.md)).

## 0.91 (13.10.2026)

- Louder sound effects and engines.
- Restored race-start sounds and synchronized traffic lights.
- Removed the extra wait before driving and its effect on first-lap times.
- Restored the moving start light and weapons notice.
- Small gameplay performance improvements.

The end-user changelog is included in `release/ReadMe`. Version strings and
archive timestamps use 13.10.2026. The 0.90 release evidence below remains
historical; it does not claim a new full release-gate run for 0.91.
A clean 0.91 package build and archive audit pass, including all executable
version/date markers, exact contents and independent decompression/CRC checks.

## 0.90 (01.10.2026)

Package `SlicksNSlide-0.90.lha`, 281,944 bytes. Rebuilt on 2026-10-01 with
release date 01.10.2026 throughout the game, slave, installer, helper and
ReadMe, including archive timestamps. `CREDITS.txt` is omitted; credits remain
in ReadMe. The exact-content audit passes with ten archive members.

| File | SHA-256 |
|---|---|
| `SlicksNSlide-0.90.lha` | `79b927470811e95c217f9e4c1c3922d6f4492e595df2305ea23d81467af646f8` |
| `SlicksNSlide` (stripped game) | `bb6b00614bc383ee1f7b2cf029565a4665e03fbe526e8033736f2c5dbb802214` |
| `SlicksNSlide.slave` | `90594fcd36767b9ee2268b28b0a0ac51dbd764ba77295d9471f92cc2b47be990` |
| `SlicksNSlideInstallData` | `a2d1c648811d360cf4e5b54356156281c8cc0d950036128fe56f8be232fdc843` |

The game build is deterministic (`make release-check`); the archive is
reproducible because the packager owns every LH5 header field.

Gate results on the 0.90 code:

- `make release-check` passes; the stripped executable is deterministic.
- F1/CITY/WHACKO display audits pass (600 updates each; 32/18/5 actors).
  RETCHECK passes on BASIC, F1, CITY and WHACKO (603 race and 700 HUD
  comparisons each; geometry comparisons on F1 and CITY), all counters zero.
- WHDLoad 19.2 with Kickstart 40.063, 68020, production slave with no
  expansion memory, 2 MiB Fast: records, championship create/overwrite/delete,
  race and quit pass with and without PRELOAD. A whole records session takes
  14 OS switches; each save takes at most one; unchanged files are not
  rewritten. 1 MiB Fast works but cannot cache files (152 switches).
- Standalone save fixtures on 2 MiB Chip / no Fast RAM: record retry, skip
  and read failure; setup save failure, cancel and retry; championship save
  and read-only failure; track-list save, reload and read-only failure;
  clear records.
- The manual stock-A1200 session passed standalone and from the WHDLoad icon.
- After the final rename to `SlicksNSlide` file names and the "Slicks 'N'
  Slide" display name, `make release-check`, the real-Installer package test
  (`test_installer_script.py --package`) and the WHDLoad race, quit, records
  and championship regressions at 2 MiB Fast were rerun on the packaged
  binaries and pass.
