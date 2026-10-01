# Release

How a release is built and checked, and the record of each release. The
dated development audit that led to 0.90 is in git history
(`docs/release-verification.md` before the 0.90 clean-up).

## Building and auditing the package

```sh
make release-check                      # full gate from a clean tree
make dist                               # dist/Slicks-$(cat VERSION).lha
python3 tools/check_release.py dist/Slicks-$(cat VERSION).lha
```

`make release-check` runs the release host checks (`RELEASE_HOST_CHECKS`), the
installer helper's extraction test against the 197 original files, builds the
game twice from clean and requires a byte-identical stripped executable, then
builds and audits the package. `tools/check_release.py` decodes every LH5
member independently with Lhasa and checks the allowlisted contents, CRCs,
HUNK headers, icons, Installer tooltypes, `$VER` strings against `VERSION`,
and that no original data, key, save, ROM or diagnostic binary is included.

The package follows the sibling WHDLoad packages: a `Slicks` drawer with its
icon, the native `Slicks` executable, `Slicks.slave`, the game icon template
`Slicks.inf`, `SlicksInstallData`, `Install`/`ReadMe` and their icons,
`CREDITS.txt` and `puff-license.txt`.

Version strings live in `VERSION`, `src/platform/amiga/version.s`,
`whdload/SlicksSlave.s` (`slv_info` and `$VER`),
`tools/install-data/io_amiga.c`, `release/Install` and the ReadMe history.

## Release gate

Before tagging, on the release candidate:

1. `make release-check` from a clean tree.
2. F1, CITY and WHACKO full-frame display audits on the stripped candidate
   (`SLICKS_LIVE_STATS=0 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3
   ./debug.sh "" diag_dirty_sprites.gdb`), and the `make -C amiga RETCHECK=1`
   retention check with `diag_retention_check.gdb` on BASIC, F1, CITY and
   WHACKO; every mismatch counter must be zero.
3. WHDLoad regressions (`tools/test_whdload.py --mode records`,
   `championship`, `championship-edit`, `race`, `quit`) at 2 MiB Fast, with and
   without PRELOAD; see [whdload.md](whdload.md).
4. One manual FS-UAE session on a stock PAL A1200 (68020, 2 MiB Chip, no Fast
   RAM): install from `Slix151.zip` with the Installer; start standalone from
   the data drawer with the default 4 KB stack; idle demo; Players; one race;
   save a championship at intermission; quit to Workbench. Repeat the launch,
   race and quit from the WHDLoad icon at 2 MiB Fast, with and without
   PRELOAD.

## 0.90 (30.09.2026)

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
