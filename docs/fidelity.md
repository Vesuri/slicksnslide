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

## Confirmed startup defects — 2026-10-12

These are **unintended differences**, not accepted adaptations. This focused
caller-level audit does not establish completeness elsewhere in the game.
The previous routine and startup smoke checks did not prove the combined
light/audio/driver-update sequence. Current status is tracked in
[open-work.md](open-work.md).

### F20 — invented stationary countdown (fixed)

The audited `race_runtime.c` set `countdown_ticks=120`, `countdown_stage=0`, `racing=0`
at race startup. Its `!racing` branch advanced clocks but returned before
input polling and car updates until six timer transitions had elapsed.
The 120/10 timing belongs to DOS's animated boundary palette, not a start
countdown: original `1fc41` initializes BP-60, and `1fec8..1ff97` updates
DS:4c6c and colours 199..203. The port already implements that mechanism
separately in `animated_boundary.h`.

The original-byte reproducer runs `1fe9b..2034f` with a one-tick update and
boundary timer 120: it reaches the per-driver update with the timer at 119.
The audited native public step returned with no driver poll.
The original audit reached `racing=1` only at frame 97, game clock 176 and
car-0 elapsed time 352 (about 1.9 seconds at normal speed). That stationary
time was charged to the first lap, after the new 900 ms audio prelude.

The F20 fix removes that branch and marks the race ready for driving at
construction. The caller still owns the pre-race presentation. The first
public step now polls input and runs physics; boundary animation remains
independent. The original-byte regression uses the public native constructor
and checks first-poll clock=1 and boundary timer=119. Clock-arithmetic and
track-animation fixtures no longer require an invented stationary phase.
On the normal 2 MiB target build, `diag_audit_startup.gdb` confirms frame=1,
clock=1, car-0 elapsed=2 after the four pre-race cues. The real pause-menu
regression passes all 13 dialogs with one cue sequence and engines restarted
only on resume. Host drive, lap-clock, track-actor setup and DOS AI checks
also pass. A 50-update target shadow run exercises all nine sites with zero
mismatches (200 calls each at sites 1, 2, 5, 7, 8 and 9; 50 at 3, 4 and 6).
The diagnostic build uses 2 MiB extra RAM for comparison storage; normal
startup and pause tests use the production 2 MiB configuration. Reproduce
with `SHADOW_FRAMES=50 SLICKS_DEBUG_FAST_KB=2048 amiga/shadow_check.sh f20 0`.
The default shadow checkpoint remains 700 updates. Full-frame display
audits pass F1 and WHACKO for 600 frames and CITY through natural completion
at frame 554. The display audit now accepts a clean completed race before
600, rather than requiring the removed wait to keep the fixture running.

### F21 — cues and start-light changes are disconnected (fixed)

Original `25358..253f4` draws the next light before cue 1 and another light
before cue 2. Commit `44c0fb8` restored only the sound requests and waits:
all four target cues ran at stage 0, then the invented countdown advanced
the lights. Its audio-only checks did not cover the surrounding drawing.

The F21 fix puts both light transitions in the shared cue sequence and
publishes their rectangles before issuing the corresponding sound request.
The constructor positions initial light 0 beside car 0, using the original
signed-coordinate calculation, right-side fallback and y=20 minimum.
Presentation and its waits also run with audio disabled. Pause/resume does
not enter the sequence.

The expanded original-byte audio oracle compares drawing times/stages as
well as all four sound requests. `verify-start-lights` compares 252 complete
VGA frames and saved-background restoration across original position cases.
The target records stages 1/2/2/2, clock=0 at every cue, then first driving
clock=1. Cue timing remains VBI 0/20/45/45 and the final period remains 272.
Both captured bitplane frames match every authoritative chunky pixel, and
the light rectangles match the original archive artwork (874 pixels each).
Moving the first light after its cue is rejected by the expanded oracle.
F1/WHACKO display audits pass 600 frames, CITY passes through completion at
554, and all 13 pause dialogs pass without repeating the presentation.

### F22 — startup sprite actors are invisible (fixed)

Original `251f3..252cf` allocates an intro sprite (`lahto4.@I`, 23×38)
with priority 10 and lifetime 30. It starts at the light's position with
zero velocity and horizontal acceleration −20 when x < 130, otherwise
+20 (actor coordinates use 1/64 pixel units). With weapons enabled, it
also allocates `aseviive.@I` (13×8) at (15,15), horizontal velocity −4,
priority 3 and lifetime 200. The port previously reserved their slots and
lifetimes but left both actors invisible (`kind=0`).

Both actors now use the original artwork and configuration, preserving
allocation order before the reserved shadow slots. Original `252cf` runs
one actor update before showing the first static light: the intro's life
is already 29 and the notice's 199 during the cue sequence. The port now
performs that setup update, including track-actor animation age. First
racing update still uses actor page zero. The intro has a separate
874-byte saved background; the native restoration shortcut declines this
actor rather than reading beyond the ordinary 128-byte inline buffer.

`make audit-startup-fidelity` executes the original constructors and motion
loop and compares 2,472 states through retirement, with both acceleration
directions and weapons off/on. `make verify-weapon-actors` now compares
both startup assets against the original sprite painter, including native
68020 drawing/restoration and deferred dirty bookkeeping: 2,688 complete
frame comparisons per renderer, plus the existing mixed-pool checks.
`verify-actor-slots` and `verify-actor-advance` also pass.

`diag_start_actors.gdb` checks the production 2 MiB Amiga build through
200 updates, using `SLICKS_OPTIONS_MENU=16` for weapons off and
`SLICKS_WEAPON_CASE=1` for weapons on. Constructor state, acceleration,
velocity and retirement match; captures at updates 1, 10, 30 and 200 match
all 64,000 displayed pixels. Original DOS captures use the normal PC
reference loop with BASIC.SS; weapons on requires Custom mode, since the
standard mode overrides the stored weapons option. Captures and decoded
artwork remain local under `tmp/`. The captured intro rectangles match all
874 original pixels and the notice matches its 57 nontransparent pixels.
The DOS weapons-on capture confirms the notice at the upper left and the
intro light departing as driving starts. Reproduce that capture with CFG
Custom mode (`options[0]=4`), Weapons on (`options[7]=1`) and the usual
reference menu sequence followed by Escape to leave the shop. The local
final videos are `tmp/f22-pc-0/capture/slicks_005.avi` (off) and
`tmp/f22-pc-1/capture/slicks_009.avi` (on).

All-site startup SHADOW checks pass 50 updates without a mismatch.
Production display audits pass F1/WHACKO for 600 updates and CITY through
its finish at 554. RETCHECK passes 700 updates on BASIC, F1, CITY and
WHACKO, with zero surface, particle, geometry, immutable-map or HUD-cache
mismatches. Diagnostic snapshots use 2 MiB additional Fast RAM; the
production captures and display audits use the normal 2 MiB machine.

### Reproduction and test blind spots

- `make audit-startup-fidelity`: executes original bytes for F20/F22 and
  light/cue ordering, and compares the relevant native behavior. It now fails on
  a regression in the repaired startup behavior.
- `SLICKS_AUTO_RACE=1 SLICKS_DEBUG_WARP=1 amiga/debug.sh '' diag_audit_startup.gdb`:
  reads target startup state through the cues and first enabled driving frame.
- The old track-actor and lap-clock fixtures assumed a stationary grid.
  F20 replaces those assumptions with isolated helper checks and the
  original first-driver boundary comparison.
- The initial audit changed no game behavior; F20–F22 above describe the
  subsequent fixes. Source archives and emulator outputs remain local-only.

## Intentional adaptations

| Area | Difference from DOS | Reason | Code |
| --- | --- | --- | --- |
| Audio | Direct Paula playback on four channels, no software mixer. Each car has a home channel; an effect borrows an engine channel (round robin) before it interrupts another effect. Effects otherwise keep the original signed-priority, flag-1 protection and flag-2 duplicate rules. | User request: no software mixing. DOS protects every engine voice, which four channels cannot do. | `src/game/audio_channels.h` (`slicks_audio_channels_request`), `src/platform/amiga/amiga_audio.c` |
| Sample amplitude | Ignore the sample-bank DOS header gain; convert unsigned PCM directly to signed PCM. Master and background channel controls remain active. | User request: remove software-mixer attenuation for separate Paula channels. Gain-100 samples become about 2.55 times louder; archives remain untouched. | `amiga_audio.c` (`copy_blocks`) |
| Display ownership | The game takes over the hardware (own copper list, VBI, keyboard). Disk I/O hands the OS back its interrupts and devices but keeps the game's display, so AmigaOS is never shown. | User request; also removes the OS-visible menu transitions of F07. | `amiga_platform.cpp`: `slicks_amiga_platform_begin`, `_begin_io`, `_end_io` |
| Publication | Updates are published at the display-end edge (line $100). In races, an update that misses the edge is published at once instead of waiting a full frame (adaptive publication). | Single-buffered display; avoids 40 ms snaps. Late publications can tear only inside dirty regions. User accepted it after watching F1. | `slicks_amiga_platform_wait_display_end`, `slicks_amiga_platform_wait_publication` |
| Race clock (B1) | None at the logic level: each update runs as many 91 Hz physics ticks as real time allows, capped at 45, as in `1000:fe5e..fe98`. The clock is PAL raster lines, not the PIT. | Replaces an earlier fixed 1/50 s per update, which ran races in slow motion. | `src/game/race_runtime.c` (`next_physics_ticks`), `slicks_amiga_platform_raster_time` |
| Speed change (D4) | Changing speed in the pause menu restarts only the tick phase, so game time stays monotonic. The original zeroes its tick counter but not the race loop's saved count; the next batch is then negative and refunds Arcade countdown time. | User decision: do not reproduce the glitch. | `slicks_race_set_timer` (`race_runtime.c`) |
| Installation | A native installer extracts data from the publisher's `Slix151.zip`; WHDLoad is an optional launcher running the same executable. The CFG platform byte is a fixed Amiga tag (`0xa1`) instead of the DOS BIOS-derived byte; a CFG with another tag is refused at startup, with no files changed. | User request. No DOS BIOS identity exists on the Amiga. | `tools/install-data/`, `whdload/SlicksSlave.s`, `amiga_setup_storage.c` (`SLICKS_AMIGA_CONFIG_SIGNATURE`) |
| Sparse shop (D2) | With non-contiguous active players (for example only driver 3), the original's shop caller passes an actual driver index where the painter expects a packed column and returns an uninitialised result. The port keeps actual driver IDs for transactions and converts them to packed columns for drawing. | User decision: safe mapping, never reproduce undefined indices. | `src/platform/amiga/amiga_shop.c` |
| Language (D3) | A negative saved selector (the shipped default, `field_05e1 = 255`) means English. The original runs DOS KEYB and picks language 2 only for code 358, which has no Amiga equivalent. Positive selectors load their own language. Only a saved selector of 0 opens the console chooser, so a fresh installation never reaches it. | No KEYB on the Amiga; the publisher's archive has no `SLICKS.CFG`. | `slicks_diag.c` (startup language resolution), `src/gen/setup_defaults.h` |

### Load Game (D1)

Not a difference: the title hides Load Game, as the supplied original does
(`2985c` skips entry 4 unconditionally; no reachable route was found). Saving
stays available at intermission: F2 opens Change Cars, and after it closes
Down/Enter reaches the hidden Save action, as in DOS. User decision to match
the original (`src/ui/title_navigation.h`, `src/ui/intermission_menu.h`).

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
