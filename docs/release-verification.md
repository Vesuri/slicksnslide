# Development release audit

## 2026-09-28 — installer release 0.1

The distribution is now `dist/Slicks-0.1.lha`, not the historical developer ZIP
below. Instructions/build details are in [install-original-data.md](install-original-data.md)
and [whdload.md](whdload.md). This is packaging/validation, not release publication.

- Downloaded the exact publisher Slix151.zip specified by the user. Native
  helper extraction matches an independent ZIP-library reader for both data
  files and all 195 tracks. Both core files also match development originals.
- Host rejection tests cover missing, empty, truncated, extended and altered
  archives and an existing destination containing settings/key placeholders.
  No settings/key are replaced, selected or shipped.
- Native helper: PAL A1200, 2 MiB Chip, zero Fast RAM, 4 KiB stack. All 197
  outputs match independently; 1,246 bytes of stack remain unused. Partial
  staging is cleaned. Actual user-key bytes are not test inputs.
- Real Amiga Installer 43.3: fresh standalone installation directly from the
  LHA candidate plus original ZIP passes on the same 2 MiB/no-Fast machine.
  Only requester answers are supplied deterministically; helper execution,
  filesystem operations, copying and native icon updates run unchanged.
  Existing-install Keep with optional WHDLoad preserves settings/key and
  produces both launch icons. Native project-icon default tools/tooltypes
  are checked (IconX and WHDLoad/SLAVE/PRELOAD).
  Explicit Reinstall replaces a deliberately modified original track while
  retaining settings and key placeholders; staging is removed after success.
- The installed, unchanged Play script runs SetPatch, sets the stack, enters
  data/ and reaches the native active title display on a 2 MiB/no-Fast A1200.
  Its executable is the stripped release executable, not a test replacement.
- WHDLoad 19.2.6941/A600 Kickstart 40.063: production-slave normal startup;
  real racing with PRELOAD on 4 MiB Fast RAM; live reads with PRELOAD disabled;
  normal REGCHECK exit returns OK. Diagnostic-only slave arguments select
  racing/exit without altering the production game. Actual AGA bitplanes and
  palette decoded from dumps show the native title and race/HUD/effects.
  WHDLoad is not claimed to work without Fast RAM.
- A clean default-options Amiga rebuild has a byte-identical stripped HUNK
  payload to the tested release candidate. Only discarded debug-symbol
  padding differed in the unstripped executable.
- Independent Lhasa extraction verifies every member against source, both
  header and payload CRCs, exact 12-member allowlist, LH5 format, HUNK binaries
  and reference icons. No private key, original archive/data, DOS program,
  ROM, RTB, WHDLoad binary, host configuration, dump or screenshot is included.
- All owned debug runs are muted and terminate their own emulator. Existing
  run.sh remains audible. The source tree contains no original or private files.

Limits: PAL stock-A1200 standalone and the stated emulator WHDLoad configuration
were tested, not all accelerators/ROMs/filesystems. Hardware joystick checks
remain deferred by the user. Gameplay still does not meet the 50 FPS target.
Registration's interactive exit-help/optional missing-image checks remain on
the open list; packaging does not reclassify them as completed.

## 2026-09-26

- Runtime dependencies: AmigaOS DOS/utility/keymap libraries version 37 and
  graphics.library version 39, AGA, the supplied original archive/DAT/tracks,
  and SetPatch from the user's AmigaOS installation. No PC executable, host
  emulator, SDL or external audio mixer is needed by the native binary.
- Build dependencies remain the documented shared Amiga GCC/vasm/elf2hunk
  toolchain and generated inputs derived locally from the user's executable.
  FS-UAE and the shared process helper are development-launcher dependencies,
  not dependencies distributed inside the native release.
- Runtime memory: all six mode-transition tests passed with exactly 2 MiB chip
  and no Fast RAM. A fresh current-build race entry also passed; the target
  compiler reports a 202,702-byte race structure. The native shop saved two
  screenshots with an additional temporary 65,078-byte allocation on the same
  configuration. These checks establish tested configurations fitting, not a
  claim that arbitrary Workbench memory pressure or every asset combination fits.
- Fresh BASIC GO preparation measured 142 OS ticks (2.84 seconds), excluding
  the starting-light countdown. Evidence: `tmp/release-startup.log`, fixture
  `amiga/diag_release_startup.gdb`; no per-frame debugger stops were used.
- Launcher defaults retain PAL A1200, 2 MiB chip and no Fast RAM. `run.sh`
  keeps audio enabled; scripted debugging uses dummy host audio and closes its
  owned emulator. `run.sh` now consistently honors `FSUAE_RUN` for both the
  mounted directories and process ownership instead of splitting those paths.
- Packaging uses an exact four-file allowlist: native HUNK executable, README,
  credits and a SHA-256/source-revision manifest. It checks HUNK headers and
  archive membership/content, refuses replacement of an existing archive,
  and never traverses the original data or generated reference directories.
  The source tree tracks no files under `ref`, `tmp`, `src/gen`, `amiga/out`
  or emulator run directories. Runtime data and OS/ROM files must be supplied
  separately; no license for those assets is implied.

At that revision a local archive was built with `make release-package`. A fresh output path
used `RELEASE_ARCHIVE=build/release/<name>.zip`. Those instructions are historical;
use the current installer build instructions above. No upload/push is implicit.

The verified local package is `build/release/slicks-749bb11.zip`, built from a
clean working tree. Its executable is 408,344 bytes; declared HUNK allocations
total 389,316 bytes (excluding runtime allocations and OS memory). All four
archive entries were reread and compared to their source content. No release
was uploaded. Integration/release open items 1–3 are now complete within the
native-port scope; general translator expansion remains explicitly deferred.
