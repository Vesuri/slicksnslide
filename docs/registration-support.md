# Registration support

Implemented 2026-09-28. The original 1.51 executable is the compatibility
oracle; registration is not a new port-specific licence or an override switch.

## Installation and privacy

Put your legitimately obtained `SLICKS.REK` beside `SlicksDiag` in the game's
working directory. Without it, the game remains shareware. A malformed key is
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

## Verification

The expired-trial heading intentionally retains the original layout quirk:
its anchor is y=81 while the blue tint rectangle starts at y=83. The heading
therefore extends above the rectangle, as reported during native testing.
The original x86 routine and the native 68020 renderer match every pixel here;
do not move the text or enlarge the rectangle as an assumed port correction.
`build/verify_registration_pixels EXISTING_LOCAL_DIRECTORY` optionally writes
local-only PPM evidence from both renderers before the delayed prompt. It does
not dump a real registration owner or key. Visual check on 2026-09-30:
`tmp/trial-layout-LginwP/trial-original.png` matches the reported overlap.

- `make verify-registration REGISTRATION_KEY=tmp/slicks.rek`: 531 original
  loader comparisons (valid, absent, truncations, mutations, sentinel, empty
  name and trailing data); 8,192 original trial/exit gate comparisons; all 256
  name-byte case conversions. Only file services and the fatal-error boundary
  are intercepted; original checksum/string code executes independently.
- `make verify-registration-pixels`: full-frame equality with original x86
  rendering for trial tint/text, its delayed prompt and right-aligned owner
  text. Owner text in pixel tests is synthetic, not taken from a key.
- Existing weapon-shop oracle suite: 32,768 prices, 65,536 buy/sell cases,
  4,096 computer-shopping/RNG cases, 1,215 navigation and 2,720 row cases.
- Existing menu-bitmap suite: original loading, players and both exit BMPs,
  including pixel/palette equality and truncated-input checks.
- Muted PAL A1200 native shop runs reach the real shop with `extra=1` for the
  valid key and `extra=0` without a key, without diagnostic weapon overrides.
- Native startup/exit fixtures cover registered, keyless and expired-trial
  paths and require full system restoration (`0x1f`). Invalid-file fixture
  checks rejection before display takeover.

The optional order-form image remains unavailable for a visual check. The
interactive exit-help key sequence is verified below. This is not a claim of
exhaustive testing of every malformed filesystem condition or every original key.
Performance optimization remains stopped; this feature does not establish
the outstanding 50 FPS target.

## Exit-help verification and corrections — 2026-09-28

Executing the original key gate at physical `2658b` proves that exactly DOS
scan codes 21 (Y) and 59 (F1) open exit help. All 65,536 values now have an
independent original-code comparison in `verify-registration`. Executing the
original wrapper at `2a096` captures the actual help-call argument at `327dc`:
DS:1436 contains `reg`, not the empty topic previously used by the port.
The port now passes that exact topic. The supplied HELP.TXT resolves it to
chapter 353/page 0 (contents, including the full-version link); the empty
topic instead resolves to 9589. `verify-help-index` confirms these original
lookup results. No replacement order text or invented registration flow is used.

The exit-help cleanup also now calls the viewer's close operation before
freeing it. This restores the underlying exit-screen pixels; previously the
help pixels remained for the final fade. Four native A1200 cases pass with
2 MiB Chip, no Fast RAM, and the confirmed default 4 KiB task stack:

- Keyless Y: `tmp/standalone-release-dkr1v7n9`.
- Keyless F1: `tmp/standalone-release-kw5bzu60`.
- Valid private key, Y: `tmp/standalone-release-5shx3je8`.
- Valid private key, F1: `tmp/standalone-release-ztaa50fh`.

All four reach chapter 353/page 0 through ordinary Amiga raw-key input,
navigate down/up, close with Escape, and restore system state (`0x1f`).
The before/after 64,000-byte exit-screen dumps compare identically in every
case. The keyless help image was rendered from its native dump and visually
inspected. No title/owner/key dumps were captured. The tests are muted and
close their owned emulator sessions. `REGCHECKY` and `REGCHECKF` are explicit
diagnostic input fixtures only; normal launches never populate their key queue.

Existing independent help suites also pass: 98 page comparisons, 16 viewer
entry/close comparisons, 1,166 pixel/font comparisons, 816 navigation cases,
and 3,024 refresh/call-order cases. Registration loader/date/pixel suites pass.

Reproduce with a disposable installed data directory and
`tools/test_standalone_release.py PRIVATE_INSTALL --default-stack --args REGCHECKY
--checks amiga/diag_registration_help.gdb --marker REGISTRATION_EXIT_HELP_RESTORE_OK`
(repeat with `REGCHECKF`, with and without a legitimate local key). Compare
the resulting `.run/registration-help/before.chunky` and `after.chunky` dumps.
The debugger fixture also asserts that keyless exit attempts the optional
image and safely skips its absence, while registered exit never requests it.
It intentionally expects the original supplied data, without `webf_ord.bmp`.
The final fixture including these optional-image assertions passed all four
cases again in `tmp/standalone-release-{lzx01xz8,4mccf3zq,hirhxwgi,4b0rtf3g}`
(same order as above); all four exit-screen comparisons also pass.

## External-image read and close failure rejection (2026-09-30)

`registration_screen` now uses the shared checked plain-file reader for its
external BMP fallback. Previously it accepted a successful `Read` regardless
of `Close`'s result. A failed read or close now returns -1 before decoding;
the existing optional-image skip policy remains unchanged. The real file handle
is closed even when the explicit read-error diagnostic is enabled.

Native checks use the ordinary REGCHECK exit sequence and the read-only
`diag_registration_external_failure.gdb`:

| Case | Argument | Run under `tmp/standalone-release-` |
| --- | --- | --- |
| Genuinely absent external file | REGCHECK | `1asexidi` |
| Injected read failure after positive real read | REGCHECKK | `6hx5vzi5` |
| Injected close failure after real handle closure | REGCHECKL | `rrlwbm3d` |

Each run observes exactly one external result of -1 while takeover is inactive,
one ordinary unregistered exit screen, no optional order-image presentation,
and normal restoration mask 31. Fault cases require the intended fault to be
consumed. Tests use fresh copied data, no key, a stripped executable, stock
PAL 68020/2 MiB Chip/no Fast and confirmed 4096-byte stack. All muted emulators
are closed. Build log: `tmp/registration-external-build.log`.

The two fault fixtures deliberately copy non-image SLICKS.DAT bytes to the
isolated `webf_ord.bmp` path to supply a positive real read; those bytes are
rejected before decoding and are not replacement artwork. These are controlled
API-failure checks, not a reproduced failing AmigaDOS handler and not visual
verification of the original order form. The genuine external order image is
still absent and its conditional visual check remains open. No original data,
test payload or private key is committed.
