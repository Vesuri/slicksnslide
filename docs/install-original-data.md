# Installer and release packaging

The current distribution is `dist/Slicks-0.1.lha`, containing a `Slicks Install`
drawer and its icon. It replaces the earlier four-file developer ZIP. The
end-user instructions are [release/ReadMe](../release/ReadMe).

Download the installer LHA and the publisher's unchanged
[Slix151.zip](https://www.slicksnslide.com/webapi/download.php?p=dos-slix&v=Slix151.zip).
Unpack the LHA on an Amiga and run Install with Installer 43+. No external
UnZip utility or host-side conversion is required. Standalone installation
does not require WHDLoad. Installer/IconX/SetPatch are user-supplied OS tools.

Installed layout:

```text
Slicks/
  Play, Play.info               IconX script (standalone)
  ReadMe, ReadMe.info
  Slicks.slave                  optional WHDLoad launch
  SlicksWHDLoad.info            optional WHDLoad project icon
  data/
    Slicks                     native game executable
    SLICKS.000, SLICKS.DAT      untouched publisher originals
    TRACKS/*.SS                195 original tracks
    SLICKS.REK                 optional user's key, never distributed
    ...                        settings/profiles/championships created in play
```

The helper validates SHA-256 of the complete publisher ZIP before creating any
output. Supported ZIP SHA-256:
`d12114fccdd86b10e9d77d652acec35ea0a45230fcbe3fb32970431c18b55c2c`.
A repacked or changed archive is intentionally rejected. File selection is
allowlisted to the two runtime data files and plain root-level TRACKS/*.SS names;
all selected DEFLATE streams must finish at their declared lengths and match
their ZIP CRC32. The helper uses bounded buffers (about 958 KiB static storage),
never allocates the whole ZIP, rejects an existing destination, and removes its
own partial outputs on failure/cancellation. Existing user directories are never
recursively deleted by the helper.

Installer extracts to a unique disk staging drawer. Only after success does it
copy data to the selected installation. Keep is the default for existing data;
it preserves modified tracks/records. Reinstall replaces the supplied tracks,
not keys, profiles, settings or championships. A copy-stage disk error is not an
atomic whole-directory transaction: retain the verified staging drawer and rerun.
Use disk scratch, not RAM:/T:, on a 2 MiB machine. No private key is read by
installation, packaging or extraction tests.

## Manual installation without Installer 43

From the unpacked installer drawer, create a fresh destination parent and run
`SlicksInstallData Slix151.zip <destination>/data`. Copy the supplied native
`Slicks` executable to that data drawer and `Play` to its parent. Change to the
parent and `Execute Play`. This uses ordinary AmigaDOS tools; no WHDLoad or
Installer program is required. For Workbench icons use the normal installer.
If installing manually under WHDLoad, put `Slicks.slave` beside `data`, then run
`WHDLoad Slicks.slave PRELOAD`. Requirements are in [whdload.md](whdload.md).

## Building

Source `amiga/env.sh`. `make dist` builds the game, helper and production slave,
strips the game HUNK symbols, and creates/audits `dist/Slicks-$(cat VERSION).lha`.
Set `RELEASE_DIR` for a separate candidate; an existing archive is never replaced.
For a release after diagnostic builds, clean/rebuild `amiga` with default options
while no emulator is using its ELF. Do not package SHADOW/RETCHECK/profile builds.

The helper includes Mark Adler's puff 2.3 from zlib v1.3.1, with its original
notice and a marked adaptation using GCC non-local-goto builtins on Amiga.
Its Amiga object disables unwind/debug-frame emission to avoid the installed
cross-compiler's dwarf2 CFI crash around those builtins. The host uses standard
setjmp/longjmp. I/O, startup and SHA-256 scaffolding follow the Vette installer.

LH5 encoding uses LHa for UNIX (`LHA` override; default shared local installation),
not extraction-only Homebrew Lhasa. The auditor separately decompresses with
Lhasa, verifies every header/payload CRC, exact membership and source identity.
The archive has 12 allowlisted members including the drawer icon. No recursive
asset collection, original files, keys, saves, ROMs, RTBs or WHDLoad binary.
Icons follow Vette/Rescue on Fractalus; see [icon provenance](../release/icons/README.md).

Release: `make release-check` runs the host oracles, the helper test, a
two-build byte comparison of the stripped executable and a scratch package
audit in `build/release-check/`. `make dist` (alias `make release`) does a clean
Amiga rebuild and writes `dist/Slicks-$(cat VERSION).lha`. It refuses to
overwrite an existing archive. `tools/check_release.py` also requires the
game, slave and Installer `$VER` strings to match `VERSION`, the ReadMe
section headings and history entry, and the project URL.

Tests:

```sh
make -C tools/install-data test
python3 tools/install-data/test_amiga.py
python3 tools/install-data/test_installer_script.py /local/Installer --package dist/Slicks-0.1.lha
python3 tools/install-data/test_installer_script.py /local/Installer --whd --keep  # needs KICKSTART (amiga/env.sh)
python3 tools/test_standalone_release.py /local/test-install/Slicks
python3 tools/check_release.py dist/Slicks-0.1.lha
```

Native tests use locally owned Kickstart/Workbench/SetPatch and the downloaded
ZIP under `tmp/Slix151-release.zip`. They are muted and close their own emulator.
The Workbench test disk currently defaults to Vette's shared local test fixture;
it is neither a game dependency nor included in the archive.
