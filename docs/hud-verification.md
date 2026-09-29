# Original HUD verification

## Reported lap-digit/border overlap (2026-09-29)

The user's `FS-UAE_Full_260929-1553_00.png` shows all four lap-1 digits
overlapping the upper recessed-panel borders. This specific placement matches
the original; it is not evidence that the complete live HUD is certified.

The original background caller `25044..25064` draws the HUD image at (0,184).
The racing-number caller `1df58..1dfcc` places the right-aligned lap digit at
(101+60*driver,186), using DS:0680 (`kirj`). `verify-dos-hud` executed the
original painter with the decoded original font and HUD background. Its new
optional `SLICKS_HUD_REFERENCE=tmp/hud-border-original.bin` output stores the
original VGA pixels for the first all-active lap-1 case, not the port's output.
The complete 768-transition composition suite still passes.

The supplied screenshot is a 2x display with native origin (74,60). Comparing
the foreground masks in each driver's lap cell gives exact equality for all
four digits (15 pixels each, covering native rows 186..191). The palette-backed
upper-border comparison also matches; the only three differences when the
comparison extends to row 191 are the fixture's different timer digits, not
the panel geometry. Thus the raised placement relative to the recessed black
interior is present in the original drawing, and no coordinate adjustment was
made. Screenshot and extracted reference pixels remain local-only.

## Font selection and composed display (2026-09-25)

The race used the wrong original font: `pieni.@f` (103 glyphs, height 5),
rather than `kirj.@f` (147 glyphs, height 6). The earlier composition test
installed the same wrong font into the x86 reference and native renderer,
so its pixel agreement did not prove the resource selection.

Original startup `19dd8..19e16` loads `/KIRJ.@F` into DS:0680, then loads
the next archive font into DS:0684. HUD callers `1ddc0` and `2adbe` use
DS:0680. `verify-font-resource` now executes that startup block with only
the archive-loader boundary substituted and asserts that the selected name
matches `SLICKS_RACE_FONT_NAME`. Both font slots must remain distinct.

Production now loads kirj from the archive into the existing 64 KiB scratch
allocation. Its 3,600 packed glyph pixels and 5,726-byte padded runtime fit
the enlarged 4,096/6,144-byte race font buffers. No captured pixels or new
font artwork are used. Coordinates, alignment and colours still come from
the translated original HUD. The original 68020 string/glyph renderer is
unchanged; the six-pixel font fits the original HUD rectangles.

Verification:

- Original font selection and full loader/padding comparisons pass, including
  every truncated input and insufficient decoder capacity.
- 512 composed whole-screen comparisons pass with the correct font: all 16
  participation masks, human/computer roles, staggered racing-to-finished
  transitions, lap digit-width changes, last/best times, all eight combinations
  of weapon/fuel/damage display options and both fuel-warning blink phases.
  The original's page-zero-only track name is explicitly mirrored for the
  single native surface; no differing HUD pixels are masked out.
- Production 68020 kirj rendering passes 3,528 full-frame glyph comparisons,
  4,240 measurement cases and 640 full-frame string comparisons against x86.
- Dirty-region tests pass with six-pixel glyphs, including last/best-time and
  Arcade countdown/LAST/LAP/grace transitions, unchanged-frame caching,
  chunky/VGA agreement and sparse-list saturation.
- Muted A1200/68020, 2 MiB chip/no fast, port 25186:
  `diag_hud_results.gdb` observes the real 147-glyph/six-pixel font, then four
  natural finishers at update 556 (clock/deadline 1012/1011), original records,
  championship standings/statistics, native setup save and restoration `0x1f`.

Reproduce after building with `amiga/env.sh`:

```sh
make verify-font-resource verify-font-glyph verify-dos-hud verify-dirty-tracking
FSUAE_RUN=.run/native-results DEBUG_PORT=25186 SLICKS_NATURAL_RESULTS=damage amiga/debug.sh "" diag_hud_results.gdb
```

## Boundary of this evidence

The live purchase/fire test exposed an empty-selection discrepancy: native
timer caching treated selection changes as full 1ddc0 redraws. Original
20c09..20c6f changes selection without invoking that redraw; its subsequent
1d9b6 status call leaves the previous icon visible when selection becomes -1.
Production now preserves that distinction. The composed oracle has been
extended to 768 full-screen transitions, including weapon cycling/depletion
while all lap/finish text stays fixed, using original status-only calls.
All pass, as do producer dirty-region checks. This fixes the actual gameplay
case rather than changing the test to accept a different icon policy.

The composed weapon tests supply inventory/selection values to compare the
original painter. They do **not**, by themselves, prove gameplay-driven firing,
cycling or depletion. The separate native gates now do: all eight weapons are
bought/fired/depleted through real shop and control events, and a two-weapon
case checks manual cycling plus automatic selection after depletion. Actual
icon/bar pixels are checked at each transition. Pause, results, next-track,
retry and fresh-process championship load preserve the resulting state. See
the native gameplay/transition evidence in `weapon-verification.md`.

The final integrated build repeated `diag_hud_results.gdb` on port 25247:
147-glyph, six-pixel kirj font; four finishers at update 556; original records,
standings/statistics, setup saving and clean restoration `0x1f` all pass.
