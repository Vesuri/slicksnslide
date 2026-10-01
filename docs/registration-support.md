# Registration support

The original 1.51 executable is the compatibility
oracle; registration is not a new port-specific licence or an override switch.

## Installation and privacy

Put your legitimately obtained `SLICKS.REK` in the installed `data/` drawer,
beside `SlicksNSlide` (the game's working directory). Without it, the game remains shareware. A malformed key is
an error, not an unlock or a silent fallback. Registration is read at startup;
it is never stored in configuration, profiles or saved championships.

`amiga/run.sh` deploys an optional local `tmp/slicks.rek`, or the file selected
by `SLICKS_REGISTRATION_KEY`. It preserves an already installed key when no
local key is present. Debug runs only copy a key when that environment variable
is explicitly supplied; use a fresh `FSUAE_RUN` directory for keyless tests.
Debug audio remains muted and normal launch audio remains enabled.

All case variants of the `.rek` extension are ignored. The personal test key,
its owner name, byte contents and checksums are not included in source, generated
headers, test output or release packages. Tests read the private file at runtime.
The release packager uses an explicit file allowlist, not a directory sweep.
No key generator or registration bypass is provided.

## Recovered behaviour

The original loader is relocated `1987:c3e0..c4fb` (physical
`25c50..25d6b`). The native reader preserves its bounded name, byte order,
signed-byte checksum arithmetic, sentinel and original uppercasing, including
the three extended-character mappings. Trailing bytes are accepted as in DOS.
Unterminated malformed names are safely rejected instead of reproducing the
original out-of-bounds string access. An actual I/O failure is reported rather
than being mistaken for a missing key.

Consumers of the original registration-name field at DS:062f are connected:

- Shop price/availability and REGISTER! labels use the validated registration
  state. Restricted weapons stay restricted without a valid key; ordinary
  prices, buying, selling and computer purchases retain the original rules.
- The registered-owner label uses the original small font and right-aligned
  (310,190) anchor. Its colour uses the translated title pulse. It replaces the
  port's extra LAPS footer to avoid overlap; no owner text is baked into assets.
- The startup trial reminder uses the original unsigned, wrapping date
  comparison against the saved installation date plus 40. Valid registration
  suppresses it. Its background, tint, text and delayed prompt are rendered from
  the original archive/font, not a captured framebuffer.
- Exit selects the original `end1.bmp` or registered `end2.bmp`, preserving
  the shareware-only wait and optional order-form branch. Y/F1 opens ordering
  help. The optional `webf_ord.bmp` is absent from the supplied data and is
  skipped when missing; a registered installation never requests it.

Presentation waits use real PAL frame time, rounded to whole frames, and
keyboard release/new-press handling. The existing title/menu implementation
does not reproduce every DOS animation cadence; registration does not claim
cycle-exact presentation. Explicit diagnostic game modes skip modal startup
and exit presentations, but still load and validate the actual key. `REGCHECK`
uses the normal presentation path and only supplies an Escape make/release
after several title updates.

## Original quirks kept

The expired-trial heading is anchored at y=81 while its blue tint rectangle
starts at y=83, so the heading extends above the rectangle. The original x86
routine and the native renderer match every pixel; do not "correct" it.

Exactly DOS scan codes 21 (Y) and 59 (F1) open exit help, with the original
help topic `reg` (DS:1436), which the supplied HELP.TXT resolves to chapter
353, page 0. Exit help closes its viewer before the final fade, restoring the
exit screen underneath. The external `webf_ord.bmp` is read through the
checked plain-file reader; a failed read or close skips it like an absent file.

## Tests

- `make verify-registration REGISTRATION_KEY=tmp/slicks.rek`: the original
  loader against valid, absent, truncated, mutated, empty-name and
  trailing-data keys; every trial/exit gate input; all 256 name-byte case
  conversions; all 65,536 exit-help key values. Only file services and the
  fatal-error boundary are intercepted.
- `make verify-registration-pixels`: full-frame equality with the original x86
  renderer for the trial tint/text, its delayed prompt and the right-aligned
  owner text (synthetic owner names only).
  `build/verify_registration_pixels DIR` optionally writes local PPM evidence.
- Native fixtures: `tools/test_standalone_release.py PRIVATE_INSTALL
  --default-stack --args REGCHECKY --checks amiga/diag_registration_help.gdb
  --marker REGISTRATION_EXIT_HELP_RESTORE_OK` (also `REGCHECKF`, with and
  without a key) and `diag_registration_external_failure.gdb` with `REGCHECK`,
  `REGCHECKK` (read failure) and `REGCHECKL` (close failure).

The original order-form image is absent from the supplied data, so it has no
visual check.
