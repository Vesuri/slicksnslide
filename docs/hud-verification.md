# Original HUD verification

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

The composed weapon tests supply inventory/selection values to compare the
original painter. They do **not** prove gameplay-driven firing, cycling or
depletion. The production game still lacks those weapon producers, which
are covered by the first remaining-game-completion item. The former fourth
item's remaining real-weapon transition check has been merged into that item;
these painter tests alone do not complete it.
