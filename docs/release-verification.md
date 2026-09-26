# Development release audit

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

Build a local archive with `make release-package`. Choose a fresh output path
with `RELEASE_ARCHIVE=build/release/<name>.zip` when packaging the same revision
again. Packaging is not publication, and no upload/push is implicit.

The verified local package is `build/release/slicks-749bb11.zip`, built from a
clean working tree. Its executable is 408,344 bytes; declared HUNK allocations
total 389,316 bytes (excluding runtime allocations and OS memory). All four
archive entries were reread and compared to their source content. No release
was uploaded. Integration/release open items 1–3 are now complete within the
native-port scope; general translator expansion remains explicitly deferred.
