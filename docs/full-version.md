# Full version

The port is always the full, registered game. Removing the shareware
limitations is authorized by the original author, Timo Kauppinen. Players can
still support his work by registering at <https://www.slicksnslide.com>.

The original 1.51 executable remains the compatibility oracle; the port
behaves as the original does with a valid registration:

- The shop offers every item. The registration-only items (rules flag 16) are
  priced and bought as the original prices them when registered, for humans
  and computers alike, with no REGISTER! labels or tints
  (`src/game/weapon_shop.h`, `src/ui/shop_draw.h`).
- There is no trial reminder at startup, so the installation date is only
  kept in the configuration as the original stores it.
- Exit shows the registered `end2.bmp` (original `1977:ccb0..cd56`): no
  shareware wait before the key, and no optional order form afterwards.
  Y and F1 (DOS scan codes 21 and 59) still open Help at topic `reg`, as in the
  registered original; Help closes its viewer before the final fade.

## Owner name

`SLICKS.REK` is optional. If a player puts their own key in the installed
`data/` drawer, beside `SlicksNSlide`, the title shows its owner name with the
original small font at the right-aligned (310,190) anchor, coloured by the
title pulse. It replaces the port's extra LAPS footer to avoid overlap. A
missing, unreadable or invalid key only omits the name; it never stops the
game and never changes anything else. The key is read at startup and never
stored in configuration, profiles or saved championships.

The reader is the original loader (relocated `1987:c3e0..c4fb`, physical
`25c50..25d6b`): bounded name, byte order, signed-byte checksum arithmetic,
sentinel and original uppercasing, including the three extended-character
mappings. Trailing bytes are accepted as in DOS. Unterminated malformed names
are rejected instead of reproducing the original out-of-bounds string access.

`amiga/run.sh` deploys an optional local `ref/slicks.rek`, or the file selected
by `SLICKS_REGISTRATION_KEY`; debug runs copy a key only when that variable is
supplied. Use a fresh `FSUAE_RUN` directory for keyless tests. All case
variants of the `.rek` extension are ignored. No key, owner name, key bytes or
checksums are in source, generated headers, test output or release packages;
the release packager uses an explicit file allowlist.

## Tests

- `make verify-registration REGISTRATION_KEY=ref/slicks.rek`: the original
  loader against valid, absent, truncated, mutated, empty-name and
  trailing-data keys; all 256 name-byte case conversions; all 65,536 exit-help
  key values and the actual Help topic argument.
- `make verify-registration-pixels`: full-frame equality of the right-aligned
  owner text with the original x86 renderer (a synthetic owner name).
- `make verify-weapon-shop`, `verify-shop-draw`, `verify-shop-pixels`: the
  original shop routines run with a nonzero registration name byte (DS:062f).
- Native fixtures (`SLICKS_REGISTRATION_TEST=N amiga/debug.sh "" FIXTURE`, a
  fresh `FSUAE_RUN`): `diag_exit_screen.gdb` with and without a key (N=1),
  `diag_exit_display_io.gdb` (N=1), `diag_exit_help.gdb` (N=2 for Y, N=3 for
  F1), `diag_exit_help_failure.gdb` (N=9), `diag_owner_invalid_key.gdb` with an
  invalid `SLICKS_REGISTRATION_KEY` (N=1), and `diag_title_owner_bounds.gdb`
  with a key (N=5). `SLICKS_WEAPON_CASE=7|8` with `diag_weapon_case.gdb` fires
  the two registration-only weapons from a normal shop purchase.
