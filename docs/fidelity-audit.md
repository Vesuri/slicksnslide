# Fidelity audit evidence

Started 2026-09-29 against `6dc6b6f`. This is an evidence ledger, not an open
task list; actionable follow-ups are in `open-work.md`. The port is **not yet
verified as a faithful whole-game port**. A routine oracle, a native smoke test
and a production-screen comparison establish different things.

## Confirmed differences found in the first source/original-code pass

| ID | Difference | Evidence |
| --- | --- | --- |
| F01 | Added CAR/TRACK/LAPS title footer and black rectangle | `redraw_title_configuration` clears (105,172)..(235,200); original title composition `29753..29af9` and owner-label tail `29f2c..29ffe` have no such footer. User's DOS/Amiga images show it. |
| F02 | Missing title player icons, selected/total track counts and option badges | Original `29928..29af9`; production never invokes the old bounded `sui_title_status`, whose four roles and counters are hardcoded and cannot be used as a general replacement. |
| F03 | Extra visible LOAD GAME row moves READ THIS and QUIT down | Original `2985c` unconditionally skips entry 4; this is not a registration conditional in the supplied executable. Production `sui_title_menu` draws it. |
| F04 | Title bevel remains behind GO instead of following selection | Original `29865..298ab` bevels the selected row; production unconditionally bevels y=82. |
| F05 | Title arrows wrap and lack original left/right edits | Original `2a0cb..2a1b6` clamps Up/Down, skips entry 4, changes selected-track count on TRACKS and mode on OPTIONS with Left/Right. Production wraps and only changes values in the legacy diagnostic setup path. |
| F06 | Extra visible intermission rows and altered Up navigation | Original `245bb..245c8` forces the first navigable row to 2, and `245da..24702` draws rows 2/3. Production enabled `expose_actions` and intercepted Up before the original dispatcher. |
| F07 | Menu transitions briefly expose AmigaOS | Menu owners end/begin hardware takeover around disk reads and even RAM-only close/redraw transitions. The separate resident-assets inventory covers the dependencies. |
| F08 | Title-loop animation coverage is incomplete | Normal selected-label pulse now advances through the translated original routine every visible title update. Arcade and wall-clock cadence/reference comparison remain unverified; see the new native cycle evidence below. |
| F09 | Main-title label and counter shadows forced to black | Original `297c4..297e8` sets DS:1600 to nearest (10,10,20); native title font wrappers passed zero instead of the translated third-colour result. Corrected below. |
| F10 | Arcade title integration validation | Original `29f31..29f47` selects `29afa` instead of `29753` for mode 5. Native integration and original full-screen renderer comparisons pass. Caller/transition font-alias lifetime, shortcut and mouse coverage remain to be completed. |
| F11 | Added mouse-click title activation | Native code dispatched Enter on a left-button edge through a second, incomplete owner path. Removed: original `36ce0..36d64`, called by the title at `2a376`, reads the keyboard scan latch and repeat timer, not mouse buttons. |
| F12 | Missing title F9 and demo routes | The native owner originally ignored dispatcher actions 4 and 5. F9 is now connected to race preparation; demo remains missing. Original F9 jumps to the result-99 case; F12 and the title idle timeout enter `2a3db` demo setup, backing up configuration and selecting a random track with four computer profiles. These are not ordinary GO. |

The first pass also finds hardwired `lang1.txt` in live pause/intermission.
This is a **candidate**, not yet a confirmed bug: audit the original language
selection/startup consumer before changing it. Track catalogue sorting, limits,
shortcut routing and platform error screens need the same caller-level check.

## Title input caller audit

Removed the separate left-mouse activation branch and its otherwise-unused
`race_prepared` latch. This branch bypassed the keyboard GO playlist/error
handling and did not handle Tracks at all. Keyboard activation and the common
race preparation remain unchanged. The original reader uses DS:1714 and the
BIOS tick counter; the scan latch's instruction-listing writers are its reset,
explicit scan setter and keyboard interrupt/init routines. The title calls
the repeat reader with argument 2 and contains no mouse polling route.

`make verify-title-help` now executes that original reader for all 256 byte
scan values, in addition to comparing actual Help topic arguments. Make scans
below 128 are returned; break/idle values return zero. This does not establish
the port's keyboard repeat cadence or implement the newly identified F12
demo lifecycle. The original dispatch table at CS:3f5e maps scan 43 to
`2a4c5` and scan 58 to `2a3db`; title elapsed time above 20000 also supplies
scan 58. Demo setup saves selected profiles and configuration, chooses a
random track and substitutes four computer profiles; the next title entry
restores saved state when DS:1148 is set. Demo caller implementation is still open.

F9 now takes the native GO preparation tail without activating the highlighted
row. An additional 84 original-instruction cases execute the real dispatch
table at `2a3ac` through `2a566`: all six modes, seven rows and both
registration-flag values return 99 with selection unchanged. No submenu or
registration-notification callback is stubbed on this path. Subsequent
empty-playlist randomization and player preparation use the existing shared
native race-start path, rather than a second shortcut-only implementation.

Native F9 verification uses `REGCHECKC`, repeating the Arcade count edits and
Options round-trip from `REGCHECKB` but replacing the final Up/Enter with
F9 while Settings remains selected. `SLICKS_REGISTRATION_TEST=8
SLICKS_DEBUG_WARP=1 FSUAE_RUN=.run/title-f9 ./debug.sh ''
diag_arcade_title.gdb` passes counts=15, draws=28, checks=43, errors=0,
options=1, starts=1, restore=31. The race handoff has two human and two
computer participants and override=2. The muted emulator exits normally.
Local log: `tmp/title-f9-native.log`. The initial diagnostic launch failed
because its outer argument parser omitted the new C suffix; this was fixed
and the complete run repeated, not counted as a gameplay failure or pass.

After removing mouse activation, the native `REGCHECKT` title-transition run
passes modes=31, roles=7, counts=3, checks=33, errors=0, restore=31. The
normal keyboard paths remain functional. Reproduction from `amiga/`:
`SLICKS_REGISTRATION_TEST=5 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/title-input-audit ./debug.sh '' diag_title_transitions.gdb`.
The run was muted and its emulator exited; local evidence is
`tmp/title-input-native.log`.

## F06 correction and checks

Removed the production exposed-row policy and the owner-side Up override.
The renderer now has only the original policy, eliminating the test mode that
could pass original pixels while production deliberately displayed other pixels.
F2 still opens Change Cars; after returning with selected row 0, Down can reach
the original hidden Save Game action. Save/load serialization is not removed.

Fresh checks: 65,536 Change Cars option words and 3,072 input-state cases;
30 composition/failure cases; driver/action/car/header command comparisons;
75 full-screen/font comparisons; 8 Change Cars open/close and 20 row redraw
comparisons. The 68020 target builds successfully.

The earlier native UIMENU2 lifecycle run **did not pass**: it exited after phase 3,
before the normal successful open, with restoration status 3. Read-only
allocation tracing found 119,064 bytes free but a largest block of only 60,272
bytes: the 65,536-byte preview arena cannot be allocated. This fixture retains
an extra 64,000-byte verification snapshot, in addition to the actual menu's
saved screen. Its fault-injection phases can therefore return early for an
unintended allocation failure. Fix its memory arrangement and require each
injected failure to reach its intended boundary before claiming the old
17-phase failure/reopen test passes.

The repaired UIMENU2 gate now passes all 17 phases, with restoration status
31. It borrows the retained startup title staging buffer rather than allocating
another 64,000-byte snapshot, uses the resident archive, and loads DAT/track
data at their actual sizes. Cleanup now frees those exact allocation sizes;
the former fixture incorrectly freed fixed-capacity blocks using file lengths.
Separate reached-boundary markers ensure each simulated allocation/resource/
render failure occurs at its intended site, not an unrelated earlier failure.
Both edit/reopen rounds pass. Reproduction: `SLICKS_INTERMISSION_SURFACE=1
SLICKS_DEBUG_WARP=1 FSUAE_RUN=.run/intermission-owner-v1 ./debug.sh ''
diag_intermission_surface.gdb`; local log `tmp/intermission-owner-native.log`.

The actual production transition passes the focused native regression:
`SLICKS_INTERMISSION_LIVE=1 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/fidelity-intermission-edits ./debug.sh ''
diag_intermission_fidelity.gdb` (from `amiga/`, after sourcing `env.sh`).
It observes initial row 2, all nine F2/edit/close inputs, next-race progression,
zero race error and restoration 31 on 2 MiB/no-Fast A1200. Host audio is muted
and the runner closes its emulator. The retry variant (`...LIVE=2`) is not the
nine-input fixture; it reached one intermission and restoration 31 but correctly
failed the edit-count assertion. An older debugger fixture also failed on a
local-variable type expression; neither attempt is counted as an edit pass.

## F01 footer correction

Removed CAR/TRACK/LAPS text and its black rectangle. The original registered
owner label remains. No replacement ornament or captured image is added.
The resource-derived regression in `tools/verify_title_background.py` compares
all 8,000 pixels at y=175..199 with `mainmenu.@I` on a fresh keyless title.
The pre-fix native binary fails with 1,677 mismatches, proving this is not a
test that merely blesses the current output. This narrow check does not prove
the missing title status, menu layout or animation correct.

The corrected native binary passes all 8,000 pixels and restoration 31 in
`.run/fidelity-title-after`; the failing control is `.run/fidelity-title-before`.
Both are fresh keyless REGCHECK runs, muted, 2 MiB Chip/no Fast RAM. The title
still has the separately listed F02–F05/F08 discrepancies. The first link
attempt exposed an unsupported cross-section PC32 sibling jump after shrinking
the redraw function; an empty compiler memory barrier retains a normal call
without adding rendering work, and the rebuilt HUNK passes the native check.

## F03–F05 title layout and arrows

The user's subsequent instruction to fix differences is applied as strict
original presentation: the normal title hides entry 4, without removing the
serializer or internal action handler. READ THIS and QUIT return to y=137/150.
The selected bevel follows the selected row. Before redraw, the existing
translated title crop restores the original artwork, erasing the old bevel.
Up/Down clamp and skip entry 4; Left/Right edit track count or game mode on
their respective rows. The stale mouse Read This index is corrected to 5;
this does not establish all mouse/shortcut behaviour as verified.

`make verify-title-menu` compares the production 68020 renderer's ordered
label/bevel commands against original instructions `29852..29928`: 224 cases
(seven selection values, sixteen colour pairs, two pages) pass. The pre-fix
renderer fails this oracle with an extra draw. This checks layout and drawing
commands, not the still-missing status area or complete title pixels.

`make verify-title-navigation` executes original `2a0cb..2a1b6` at CS=266c
and compares selection, count, mode and refresh state: 17,920 cases pass.
`make verify-title-bridge` passes 65,536 dispatch, 512 text/selection and
320 font-argument cases. Native build and keyless title/exit smoke pass in
`.run/fidelity-title-controls`, with all 8,000 lower-title artwork pixels
unchanged and restoration 31. This muted 2 MiB/no-Fast run does not exercise
an interactive arrow sequence; it is not whole-title or whole-port acceptance.

## Coverage review required for whole-port acceptance

### F09 title shadow correction (2026-09-29)

Main labels and status counters now pass the original palette-selected shadow
colour (`slicks_title_third_color`) into the real planar font renderer. Other
generic small-title text retains its prior policy. Shared title text page state
is owned by `title_state.s` alongside the other title state, allowing the bridge
test to link the actual label wrapper rather than aliasing both font entries
to the same stub.

Fresh bridge checks include all 256 foreground colours with a nonzero shadow,
640 status-counter ABI cases, and the existing 65,536 dispatch, 512 legacy
text/selection and 320 original-font cases. The native target builds and the
muted `.run/fidelity-title-shadow` REGCHECK exits with restoration 31. Original
font rendering itself remains covered by the separate font pixel oracles.

### F02 title status restoration (2026-09-29)

Replaced the unused hardcoded BASIC status slice with the original signed-role
dispatch, compact icon positions, selected/total track counts and conditional
inventory/weapons/custom badges. Production reads the current setup session,
playlist, discovered catalogue and resolved configuration. The five original
indexed assets are decoded at startup (320 bytes of pixel capacity total), not
read on menu transitions. Counts use `kirj.@f`, original flags 6/4 (right/left
alignment), and the original (70,70,15) palette query. Restore underlying title
artwork before repaint so removed icons and shorter counts cannot remain.

Fresh `verify-title-status` executes original 2995f..29af6: 6,480 ordered
command comparisons across every ternary role combination, all badge
combinations and ten count patterns pass. The five title assets now also run
through the original indexed loader and transparent VGA renderer oracle.
The extended GCC bridge test passes 640 status-font argument cases. Existing
224 title-layout, 17,920 navigation and 65,536 dispatch cases still pass.

The muted 2 MiB/no-Fast/default-stack REGCHECK run in
`.run/fidelity-title-status` captures the actual title and exits with restoration
31. The capture displays the four icons and 195/195 counts, with all 8,000
lower artwork pixels unchanged. This is initial-screen integration evidence,
not yet an exhaustive interactive badge/role/count transition audit. The
original title animation (F08) and wider coverage below remain open.

F02's native transition gate now also passes: `REGCHECKT` uses ordinary menu
inputs to change Human -> Computer -> None -> Human, returns from Players
after each change, edits 195 -> 194 -> 195 selected tracks, and cycles modes
0..4 and back. `.run/title-transitions` reports modes=31, roles=7, counts=3,
17 publications, zero full-screen logical/chunky/bitplane mismatches and
restoration=31 on muted 2 MiB/no-Fast A1200. The fixture terminates without
saving its edits. Combined with the original-instruction command oracle,
this closes the normal-title status transition obligation. Arcade remains
separately open as F10; animation and shortcut coverage are not implied.

### Normal-title animation integration

Original `2a36c` calls `29f2c` on each title-loop iteration; normal mode calls
`29753`, whose `29779..297e8` advances the byte counter by four and selects
the triangular palette ramp. The port previously ran its translated
`sui_title_step` only during initial title construction. Visible normal-title
updates now invoke that routine and redraw through the existing dirty
publisher, including keyless titles. The shared registration tail advances
even when no owner name is present. Child dialogs and races do not tick it.
Arcade is deliberately not passed through this ordinary-menu tick.

`SLICKS_REGISTRATION_TEST=6 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/title-animation ./debug.sh '' diag_title_animation.gdb`
passes 72 updates, 19 palette-index changes, 73 complete logical/chunky/planar
pixel audits with zero mismatches, exactly one full-screen publication and
restoration 31. It crosses a complete byte-counter wrap. The native graphics
oracle passes 256 original-x86/68020 title-step cases and 256 title-tail cases;
layout (224), status (6480), navigation (17920) and dirty-bounds tests also pass.
Local-only logs: `tmp/title-animation-native.log`,
`tmp/title-animation-oracles.log`, `tmp/title-animation-regression.log`.
The animated REGCHECKT regression also passes: modes=31, roles=7, counts=3,
33 publications, zero pixel mismatches, restoration=31
(`tmp/title-animation-transitions.log`). Both owned emulator sessions exited.
This proves the restored colour sequence and publication, not equivalence
of DOS wall-clock cadence or completion of the separate Arcade title.

### Arcade input translation (not yet connected to presentation)

The original mode dispatch at `2a25b` selects `2a1c9` for mode 5, rather
than normal navigation `2a0cb`. Arcade has rows 0/1; row-zero Left/Right
changes DS:0f1a, the human-driver override count, within 1..4. Up invalidates
status only on movement; Down does not. `2a28f` maps visual row 1 to action
3 (Options), without changing the visual selection. These semantics are now
available in `title_navigation.h`. The original-instruction oracle passes
51,200 mode-dispatched input/edge-state cases and all 65,536 selection words
for each of ten modes (655,360 action mappings), alongside 17,920 existing
normal-title cases. Production integration must accompany the two-row Arcade
renderer, not silently apply these controls to the ordinary six-row display.

The complete Arcade drawing orchestration at `29afa..29f2b` is now
translated in `arcade_title_draw.h`. `make verify-arcade-title` compares
12,288 complete command streams against the original instructions: every
byte-counter value, three selection bytes, eight player-count edges and
both clean/invalidated status regions. It checks palette queries, font-colour
writes, shadow colour, both background crops, all four player boxes, all
seven text commands, and counter/refresh outputs. Signed colour endpoint
averages include negative and wrapping byte inputs. Font loading, language
lookup, formatting and primitive pixel painters remain explicit test
boundaries; this is not a full-screen pixel comparison or production proof.

Integration details recovered from callers: DS:0684 is the resident
`pieni.@f` heading font, DS:0688 is `iso.@f` for player numbers and the
two-line settings summary. The summary consumes DS:00fa/0102 (options
13/14). Preserve the distinct DS:6bd4 last-loaded-font alias used immediately
before the settings heading; do not silently treat that write as targeting
the following text call's font. The original 1..4 override must be mutable
in both profile selection and race-palette preparation, which previously used
the initial exported constant in production.

The production title now selects the separate Arcade draw sequence and
advances its pulse while visible. It retains decoded `pieni.@f` and the
language table at startup; ordinary redraws do no file I/O. A generic planar
font bridge uses the original font, flags and shadow, and has 2,048 native
argument cases. Signed settings formatting has 65,536 value-pair comparisons
and explicit capacity/unsupported-format checks. Status restoration retains
the original two-update invalidation byte rather than restoring on every tick.
The 1..4 count is now mutable in setup resources and race-palette preparation;
the original profile selector maps its human slots to profile 2 and the other
slots to computer profile 1. Consequently 2P means two humans and two computer
drivers, not two active cars. Keyboard and click action dispatch map the
second visual row to Options. Full original-pixel and mouse/shortcut route
audits are still required; command/bridge tests do not prove those boundaries.

Native `REGCHECKB` gate: `SLICKS_REGISTRATION_TEST=7 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/arcade-title ./debug.sh '' diag_arcade_title.gdb` passes all
four override counts (mask 15), 30 Arcade draws, 45 logical/chunky/bitplane
checks with zero errors, one Options round-trip and one race start with
roles -1/-1/1/1, override 2, four participating cars, and restoration 31.
The first diagnostic expectation incorrectly required the nonhuman slots
inactive; original `2bb70` profile-selection evidence corrected the test,
not the game. Full profile-selection and setup-session oracles pass again.
Local logs: `tmp/arcade-title-native.log`, `tmp/arcade-title-host.log`,
`tmp/arcade-title-profiles.log`.
Normal-title regression still passes modes=31, roles=7, counts=3, 33 display
audits, zero pixel errors and restoration 31
(`tmp/arcade-title-normal-regression.log`). Arcade initialization now uses a
background-only bridge: it must not advance the ordinary title counter before
executing its own renderer. The shared owner-name tail still advances once.

`make verify-arcade-title-pixels` now passes 144 complete 320x200 and
complete-font-state comparisons. This runs original `29afa..29f2b`, including
real DOS palette selection, crop, rectangles and font painters, against the
same `arcade_title_painter.h` used in production and the actual 68020 planar
font-string bridge/glyph implementation. Both sides use the supplied title
artwork, palette and all three fonts, not captured pixels. Cases cover both
rows, all four player counts, pulse edges/wrap and clean/dirty status crops;
all three last-loaded-font aliases are exercised. Only language lookup and
libc formatting are oracle boundaries. The separate original-command gate
and formatting/bridge tests remain applicable. Local log:
`tmp/arcade-title-pixels.log`.
The refactored production binding passes the stock-A1200 REGCHECKB gate
again: counts=15, draws=30, checks=45, errors=0, one Options return, one
correct race start and restoration=31 (`tmp/arcade-painter-native.log`).
The owned muted emulator exited normally.

Font startup verification now executes through the third font slot and
confirms kirj/pieni/iso load order. Each complete original font-loader run
also checks DS:6bd4/6bd6 against the returned allocation. The direct-call
inventory in the supplied normalized listing contains the three startup
loader calls; the known direct alias writes occur in that loader. This
supports the initial iso alias but does not replace caller/transition tracing
for the remaining lifetime audit. Pixel comparisons deliberately cover all
three aliases instead of assuming this lifetime requirement away.

| Area | Existing evidence to inspect | Caller/integration obligation |
| --- | --- | --- |
| Startup/title/registration/exit | registration and title verification | Whole title composition, real state changes, both registration states, input and animation; missing optional order image remains unverified. |
| Player setup/name/colour/controllers | player-setup completion, original dialog pixel tests | Production parameters, accepted/cancelled/repeated actions, all role/device choices; manual joystick test remains user-deferred. |
| Options/languages/help | option/input/help oracles | Every displayed option must reach its original consumer; topics, locale choice and shortcuts must use actual state. |
| Track selection/lists/preview/records | track menu and 195-track asset tests | Counts/order/duplicates/limits, keyboard shortcuts, actual selected-track handoff and records. Building an asset is not race completion. |
| GO/countdown/race restart | setup and actor startup tests | Fresh-game versus resume state and exact setup/render/audio order. |
| Driving/AI/collisions/pits | 79,200-update historical trajectory checks | Recheck current production boundaries, all-car ordering, options and untimed/timed finish interactions. |
| Weapons/shop | weapon verification | Registration gates, transactions, effects and production inventory/cash consumers. |
| Particles/layers/skid marks | actor/retention/rendering audits | Original effect calls and draw order as well as chunky/planar agreement; agreement between two native buffers alone is insufficient. |
| HUD/clocks | HUD font/composition and timing tests | All modes, role masks, phase transitions and real runtime values. |
| Pause/intermission/results | menu/finish/result oracles | Exact available actions, routes, composition, waits and resume state; no policy override bypassing the tested renderer. |
| Save/config/profiles/track records | fault and original-format tests | Real chooser entry paths, successful round trips and preservation on errors; classify safer malformed-input handling separately. |
| Audio | original request/sample/pitch tests and listening acceptance | Keep the explicitly requested four-channel/no-mixing adaptation; verify original sample/event selection, not PC voice-count equality. |
| Platform/timing/memory | native release and restoration tests | Hardware ownership, no transition flashes, supported memory/stack, actual update pacing; outstanding stock-A1200 50 FPS is not silently waived. |

Historical test counts above identify available evidence, not fresh passes of
this audit. An area is not cleared by a TODO search, an old completion claim,
or tests against the port itself. No additional discrepancy should be labelled
confirmed until original-code or matched original-run evidence supports it.

## Adaptations to preserve or explicitly classify

- User-requested: Paula four-channel priorities without software mixing,
  hardware takeover, display-end refresh limiting, native installation/WHDLoad.
- Defensive storage/parsing: atomic saves, recovery files, bounds checks and
  explicit resource/I/O errors. Do not reproduce memory corruption or destructive
  partial writes merely to mimic malformed-input behaviour. Normal-game UI and
  semantics must still be compared with DOS.
- Deferred or unavailable evidence is not a supported-feature claim: manual
  joystick verification and the absent external order-form image remain so.
