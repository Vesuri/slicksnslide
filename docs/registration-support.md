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

The optional order-form image and interactive exit-help key sequence have not
had a separate end-to-end visual run. This is not a claim of exhaustive testing
of every malformed filesystem condition or every original registration key.
Performance optimization remains stopped; this feature does not establish
the outstanding 50 FPS target.
