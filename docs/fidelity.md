# Fidelity and intentional differences

The original `SLICKS.EXE` (1.51) running under the PC reference loop is the
ground truth. Native behaviour is accepted only when it matches that
executable: routine-level oracles execute the original x86 instructions
against the native code, full-frame comparisons render the original routines
from the original assets, and caller-level audits trace how the original
reaches each routine. A difference is either corrected or listed here as a
deliberate adaptation with its reason. The findings F01–F19 come from a
source/original-code audit of the whole port. Detailed evidence stays in
the deleted `docs/fidelity-audit.md` in git history. Release blockers belong in
[open-work.md](open-work.md).

## Intentional adaptations

| Area | Difference from DOS | Reason | Code |
| --- | --- | --- | --- |
| Audio | Direct Paula playback on four channels, no software mixer. Each car has a home channel; an effect borrows an engine channel (round robin) before it interrupts another effect. Effects otherwise keep the original signed-priority, flag-1 protection and flag-2 duplicate rules. | User request: no software mixing. DOS protects every engine voice, which four channels cannot do. | `src/game/audio_channels.h` (`slicks_audio_channels_request`), `src/platform/amiga/amiga_audio.c` |
| Display ownership | The game takes over the hardware (own copper list, VBI, keyboard). Disk I/O hands the OS back its interrupts and devices but keeps the game's display, so AmigaOS is never shown. | User request; also removes the OS-visible menu transitions of F07. | `amiga_platform.cpp`: `slicks_amiga_platform_begin`, `_begin_io`, `_end_io` |
| Publication | Updates are published at the display-end edge (line $100). In races, an update that misses the edge is published at once instead of waiting a full frame (adaptive publication). | Single-buffered display; avoids 40 ms snaps. Late publications can tear only inside dirty regions. User accepted it after watching F1. | `slicks_amiga_platform_wait_display_end`, `slicks_amiga_platform_wait_publication` |
| Race clock (B1) | None at the logic level: each update runs as many 91 Hz physics ticks as real time allows, capped at 45, as in `1000:fe5e..fe98`. The clock is PAL raster lines, not the PIT. | Replaces an earlier fixed 1/50 s per update, which ran races in slow motion. | `src/game/race_runtime.c` (`next_physics_ticks`), `slicks_amiga_platform_raster_time` |
| Speed change (D4) | Changing speed in the pause menu restarts only the tick phase, so game time stays monotonic. The original zeroes its tick counter but not the race loop's saved count; the next batch is then negative and refunds Arcade countdown time. | User decision: do not reproduce the glitch. | `slicks_race_set_timer` (`race_runtime.c`) |
| Installation | A native installer extracts data from the publisher's `Slix151.zip`; WHDLoad is an optional launcher running the same executable. The CFG platform byte is a fixed Amiga tag (`0xa1`) instead of the DOS BIOS-derived byte; a CFG with another tag is refused at startup, with no files changed. | User request. No DOS BIOS identity exists on the Amiga. | `tools/install-data/`, `whdload/SlicksSlave.s`, `amiga_setup_storage.c` (`SLICKS_AMIGA_CONFIG_SIGNATURE`) |
| Load Game (D1) | Not an adaptation: the title hides Load Game, as the supplied original does (`2985c` skips entry 4 unconditionally; no reachable route was found). Saving stays available at intermission: F2 opens Change Cars, and after it closes Down/Enter reaches the hidden Save action, as in DOS. | User decision to match the original. | `src/ui/title_navigation.h`, `src/ui/intermission_menu.h` |
| Sparse shop (D2) | With non-contiguous active players (for example only driver 3), the original's shop caller passes an actual driver index where the painter expects a packed column and returns an uninitialised result. The port keeps actual driver IDs for transactions and converts them to packed columns for drawing. | User decision: safe mapping, never reproduce undefined indices. | `src/platform/amiga/amiga_shop.c` |
| Language (D3) | A negative saved selector (the shipped default, `field_05e1 = 255`) means English. The original runs DOS KEYB and picks language 2 only for code 358, which has no Amiga equivalent. Positive selectors load their own language. Only a saved selector of 0 opens the console chooser, so a fresh installation never reaches it. | No KEYB on the Amiga; the publisher's archive has no `SLICKS.CFG`. | `slicks_diag.c` (startup language resolution), `src/gen/setup_defaults.h` |

### Defensive parsing and I/O

The port does not reproduce memory corruption or crashes on malformed input.
Normal-game UI and semantics are still compared with DOS.

- **Registration names.** The reader keeps the original 60-byte name, byte
  order, checksum, sentinel and uppercasing, and accepts trailing bytes as DOS
  does. An unterminated name is rejected instead of the original's
  out-of-bounds string read. A malformed key is an error, never an unlock. A
  real I/O failure is reported, not treated as a missing key
  (`src/game/registration.h`).
- **Bounds checks** on loaded files: setup, profiles, tracks (8192-byte
  limit), saved championships and archive resources are size-checked. Reads
  probe one byte past capacity so a truncated or oversized file is not
  mistaken for a complete one (`amiga_setup_storage.c`, `read_file`).
- **I/O errors.** Failed reads or writes show a game warning with Retry
  instead of an OS requester; requesters are suppressed only around game
  writes (`store_files`). Post-race records keep their in-memory result, so
  Retry never inserts the same results twice.
- **Track catalogue (F18).** Discovery and selection storage are dynamic, up
  to a configured cap of 10,000 files (`SLICKS_TRACK_FILE_MAX`). The original
  fallback to `.\*.SS` and its stem ordering are kept. Catalogues of 300
  tracks pass startup, race and Help. A catalogue too large for memory
  produces a memory warning and a safe return, never a partial catalogue.

### Saves

Saves behave like DOS: each file is replaced in place with one complete
write. There are no `.new`/`.bak` files, existence checks, renames or
deletes. A failed write may leave that file incomplete; the game keeps its
in-memory state and offers Retry. CFG/PLR are rewritten only when their bytes
differ from those last loaded or saved, and track records only when the
record block changes. Under WHDLoad each write is one `resload_SaveFile`
([whdload.md](whdload.md#file-operations)). Code: `write_whole` and
`slicks_amiga_store_setup` in `amiga_setup_storage.c`.

Older descriptions of "atomic saves" and "recovery files" (including the
`.SSS.new` rejection case in `championship-save-resume.md`) are obsolete;
the transactional writer was removed.

A championship `.SSS` stores the next race in the original format, not cars
in motion. Options, bindings, colour ramps and RNG state are not in that
format; resume uses the persisted setup and keeps the running process's RNG.

### Held keys

`src/ui/key_repeat.h` is an exact port of the original menu repeat reader
`36ce0`: the latch holds every raw keyboard byte, a fresh hold returns at
once, and the key then repeats whenever BIOS ticks exceed `last + argument`.
The VBI advances an emulated BIOS tick count at 1193182/(65536*50) per frame.

| Repeat argument | Owners |
| --- | --- |
| 2 | Title, Players, Options, shop, Change Cars, race Speed, Controllers |
| 3 | Pause menu, intermission, profile editor, colour picker |
| 7 | Track info preview |
| column+1 | Tracks |
| focus_actions*4+2 | List dialogs |
| none | Messages, key capture, the race |
| AT typematic | Help (title, Options, Players, shop, pause), name dialogs, saved-game name entry |

The original never programs typematic, so Help and name entry use the AT
power-on default: 500 ms delay, then 10.9 repeats/s
(`slicks_amiga_platform_typematic_key`).

**Deviation.** Owners whose originals call `36ca5` on entry or exit (pause
menu, intermission, Change Cars, Controllers, colour picker, list dialogs)
clear the latch (`slicks_amiga_platform_clear_latch`). On a PC the keyboard's
typematic would re-send a still-held key after about 500 ms; the Amiga does
not, so the key must be pressed again.

### Rendering and rate

- **Partial redraws.** A link-only change on an unchanged Help page draws
  only the lines holding the old and new link; a cursor-only move in Tracks
  redraws only the old and new bevel and scroll-marker rectangles. The
  original redraws the whole page or list. Pixels are identical, proven by
  comparing partial and full renderers after every key
  (`src/ui/help_renderer.h`, `src/ui/track_menu_renderer.h`;
  `verify-help-partial`, `verify-track-partial`).
- **Font caches.** Glyph offsets and the character-to-glyph map are cached
  per font (`src/ui/sui_font_cache.s`); output is unchanged.
- **Work per drawn update.** Particle ageing, actor animation and the homing
  turn step advance once per drawn update, on DOS as on the Amiga. On DOS
  this rate depends on the CPU: the title runs at 18–55 ms per update between
  12,000 and 100,000 DOSBox cycles, and a fast PC averaged about 70 updates/s.
  The Amiga's up to 50 updates/s matches a slower PC. Physics and clocks are
  independent of it (B1).

## Native additions removed

These were port-only features with no original counterpart and are gone:

- **F01** CAR/TRACK/LAPS title footer and its black rectangle.
- **F03** visible LOAD GAME title row (READ THIS and QUIT are back at
  y=137/150).
- **F06** extra intermission rows and the altered Up navigation.
- **F11** left-click title activation. The original title reader `36ce0`
  reads the keyboard latch, not mouse buttons.
- **F16** global right-mouse program exit.

No game path consumes the mouse. The original's `INT 33h` calls only reset
it, read status once to choose a coordinate scale, set bounds and set the
cursor; nothing reads position or buttons for input.

## Absent or unverified

- **Order form.** The optional `webf_ord.bmp` shown from the shareware exit
  is absent from the supplied data. The game skips it when missing, as DOS
  does, and a registered installation never requests it. Its presentation
  has not been visually checked.
- **Joystick.** The manual joystick test is deferred by the user.
- **PC joystick adapters.** PC parallel-port joystick adapters are not
  available; Amiga joystick ports are read natively.

## Findings F01–F19

| ID | Finding | Resolution |
| --- | --- | --- |
| F01 | Added title footer and black rectangle | Removed; lower title matches `mainmenu.@I` pixel for pixel. |
| F02 | Missing title player icons, track counts and badges | Restored from the original role/badge dispatch; command oracle and transition captures pass. |
| F03 | Extra LOAD GAME title row | Hidden as in the original (D1). |
| F04 | Bevel fixed behind GO | Bevel follows the selected row; matches original draw commands. |
| F05 | Title arrows wrapped, no Left/Right edits | Up/Down clamp and skip entry 4; Left/Right edit track count and mode, as in `2a0cb..2a1b6`. |
| F06 | Extra intermission rows, altered Up | Rows hidden; first navigable row forced to 2 as in the original. |
| F07 | Menu transitions exposed AmigaOS | Resident menu resource cache; disk I/O keeps the game display. |
| F08 | Title animation coverage incomplete | Pulse runs through the translated original every update; captured title compositions match the original. Cadence is CPU-dependent on DOS (see rate above). |
| F09 | Title shadows forced to black | Original palette-selected shadow colour passed to the font renderer. |
| F10 | Arcade title integration | Mode 5 uses the Arcade renderer; full-screen comparisons with the original pass. |
| F11 | Added mouse-click title activation | Removed. |
| F12 | Missing title F9 and demo routes | F9, keyboard demo and idle demo connected, with original demo setup and restoration. |
| F13 | Missing computer-car display-direction delay | Reproduced (`car_display.h`); 278,528 original comparisons pass. |
| F14 | Missing prepared-title background tints | Original tint preparation applied once at startup. |
| F15 | Saved language ignored | All live table consumers load the saved language; D3 covers the negative selector. |
| F16 | Added right-mouse program exit | Removed. |
| F17 | Shop Help dropped printable input | Shop Help uses the shared keymapped Help adapter. |
| F18 | Track catalogue truncated at 256 | Dynamic storage up to 10,000 files; 300 tracks pass; memory exhaustion fails safely. |
| F19 | Registration Help retained released modifiers | Caller passes the full raw byte; Shift release clears modifiers. |
