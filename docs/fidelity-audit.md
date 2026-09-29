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
| F13 | Missing computer-car display-direction delay | Original `23d97..23e7a` retains a displayed direction in DS:3068 and a byte timer in DS:3069. Native `draw_car_reference` and `car_render.s` choose directly from the current heading and have no equivalent state. Found while mapping the demo return reset, which initializes these two fields. |

The first pass also finds hardwired `lang1.txt` in live pause/intermission.
This is a **candidate**, not yet a confirmed bug: audit the original language
selection/startup consumer before changing it. Track catalogue sorting, limits,
shortcut routing and platform error screens need the same caller-level check.

## Title input caller audit

### Demo setup/restoration boundary

`src/ui/title_demo.h` reproduces original `2a3db..2a4c9` setup and title
re-entry `2a2f4..2a343`. `make verify-title-demo` executes both original
blocks, including actual memcpy, RNG and integer-runtime helpers, for 960
paired cases: six discovered-track counts, five prior playlist lengths and
32 state/seed patterns. It compares all option values, selected profiles,
all 256 playlist entries, count, random stream, selection and active flag.
Intervening mutations establish that restoration overwrites the saved options,
profiles and entry zero, but preserves the RNG and the rest of the playlist.
Invalid native input rejects atomically; inactive restore is a no-op.

This helper is not yet connected to production. Demo activation, elapsed-time
input, signed DS:0459 consumers, race return and persistence suppression still
need implementation/verification. In particular, setting all four profile
IDs to 1 in the demo setup is not proof of final participation: `2bb70`
subsequently applies the ordinary Arcade override. Do not replace that caller
chain with a guessed all-computer native setup. No emulator or release build
was needed for this isolated comparison, and no live-demo pass is claimed.

The setup-session suite now composes the original demo setup, original
`2bb70` selection with `(suppress=0, choose=-1)`, and original title restoration
against the corresponding port helpers. All 960 cases pass across the prior
session's six modes, five track-count gates and 32 seeds; the mutable Arcade
override count spans -1..5. The fixture uses built-in-style profile flags and
random-vehicle selectors, and checks final profile IDs, participation,
vehicles, colours, human-first ordering, property callback order, RNG, restored
configuration and playlist. With those flags, slots below the override count
become profile 2/negative participation; remaining slots become profile
1/positive participation. The initial four profile-1 assignments must therefore
not be treated as a guarantee of four AI roles or replaced with a forced AI
configuration. The existing 3,840 setup-session transition cases also pass.
Log: `tmp/demo-selection-composed.log`.

This proves composition at the named instruction boundaries, not native
interactive demo entry/exit, hardware controls or the complete race owner.
Returning to title restores configuration without rewinding the consumed RNG;
refreshing the displayed selection remains a live-owner integration obligation.

The race-loop key boundary `23f65..23ff8` now has 65,536 independent
original-instruction comparisons, covering every demo-flag byte and keyboard
scan byte. Negative DS:0459 exits on scans below 128 except F11/F12 (57/58).
Positive flag values take a different delay/dispatch path; zero uses ordinary
dispatch. Release/idle scans do not exit. `slicks_title_demo_exit_key` records
that exact classification, but is not yet wired into the native race owner.
The oracle stops at the diagnostic, positive-mode delay, ordinary dispatch
and exit boundaries; it does not substitute the conditional instructions.

F11/F12 are not file captures: their branch loops across 320x190 pixels,
reading original `1b089` and writing through `3b55:000e`. F11 reads the
material byte shifted right by three; F12 combines two-bit packed data with
the material byte's low three bits. Near-call disassembly at `23fad` must
honour 16-bit IP wrap: the target is `1b089`, not linear `2b089`. Their native
display route remains unconnected. The earlier conversational
description as a screen capture was incorrect.

The isolated `track_data_view.h` renderer now passes eight full-image
comparisons against the original `23f87..23fd8` loops and `1b089` sampler.
The test intercepts only the final VGA pixel-plot boundary; the material
arithmetic and near-call wrap execute as original instructions. Four patterned
raw/packed maps exercise every byte value and both scan modes. Each case checks
60,800 original column-major plots, identical complete 64,000-byte output
(including the untouched ten HUD rows), and one `(0,0)..(320,190)` dirty region.
The renderer consumes the already-decoded native lower/upper material maps,
not a framebuffer dump. `make verify-track-data-view` passes; local log:
`tmp/track-data-view-oracle.log`.

This is isolated renderer evidence, not a live demo or Amiga publication pass.
The original preceding gate bypasses these views when DS:0459 is zero, so
ordinary race hotkeys must not enable them indiscriminately. Demo lifetime,
input mapping and publication/return wiring remain open.

### Demo presentation caller, not an AI selector

`19dae` is a negative-flag predicate used by the per-race presentation owner
`1f84d`; identifying that predicate alone does not prove any AI assignment.
For a negative flag, the owner draws the original DS:0bff label twice: nearest
palette colour to `(0,0,0)` at `(11,11)`, then nearest to `(50,50,50)` at
`(10,10)`, both unaligned/unshadowed text calls through font DS:0680. It changes
font colour slot zero before each pass. The owner skips its ordinary Arcade
countdown branch in this case. For zero or positive flags it continues at
`1f901` instead; positive mode must not be folded into idle-demo behaviour.

`make verify-demo-overlay` executes the complete original predicate/overlay
caller and intercepts only palette lookup, font-colour and text service calls.
All 768 flag/palette-result cases pass, checking both ordered text passes,
their coordinates/string/font/flags, and the normal-owner bypass. It is a
command-boundary oracle, not a final pixel comparison or production wiring.
Local log: `tmp/demo-overlay-oracle.log`.

The track-loading caller also differs: `1b540..1b58f` draws the constructed
ordinary caption for flag zero, the DS:09a0 demo caption for a negative flag,
and no caption there for a positive flag. The existing branch oracle covers
classification, but full native loading-caption lifetime remains unverified.
Both captions/overlay must be included in live demo integration rather than
merely selecting computer profiles and suppressing result screens.

`src/ui/demo_overlay.h` now implements the two-pass overlay with explicit
palette, font-colour and text callbacks. It returns whether it replaced the
ordinary Arcade overlay, takes the caller-owned original label/font, and
does not invent a new shadow mode or restore font colour that the original
left changed. `verify-demo-overlay` compares the complete ordered callback
trace directly with the original trace in all 768 cases, not just with a
second handwritten expected sequence. Log:
`tmp/demo-overlay-native-oracle.log`. The helper is not yet called by the
race owner; live font binding, dirty publication, screenshot comparison and
demo-return integration remain open. No emulator/build replacement was
needed for this isolated host/original comparison.

The idle scan replacement `2a387..2a39c` now passes 36,864 original-instruction
comparisons: all byte scans, twelve initial timestamps and twelve elapsed
boundaries including 19,999/20,000/20,001 ms, second boundaries, 16-bit carry,
signed 32-bit boundaries and wraparound. `slicks_title_demo_scan` preserves
the original strict signed elapsed >20,000 test, including overriding a
simultaneous scan when overdue. Clock acquisition is an explicit boundary;
the comparison itself executes from the original image. Original `37aad`
calls the DOS runtime date/time conversion at `14efc` and multiplies its
whole-second value by 1000. Native integration must not assume that 1000
rendered frames are the same thing, or use a >=20,000 comparison. The
existing demo setup/restore and race-key tests still pass. No live timer or
demo-return test has passed yet; production remains unchanged by this helper.

### Demo consumer branch inventory

Caller-lifetime audit (2026-09-29): not every DS:0459 reader is a per-demo
operation. The following startup readers are inside the original process
owner beginning at `25ac0`, before its title/race loop:

- `25c05` gates `2b486`, the existing configuration-file loader, not a race
  save. Its exact data handling is covered by `verify-configuration`.
- `25d76` gates `2c0b9`, whose argument-vector walk starts at argument 1,
  accepts `-`/`/` option prefixes and dispatches on the following character.
  It is command-line processing, not a demo exit dialog.
- `25eaa` bypasses the opening image/text/fade sequence and conditional
  unregistered/date warning through `2611e`. It is not the post-race results
  owner. `26175` similarly gates the startup wait with argument 500.
- `26304` is in the subsequent loop: positive mode bypasses the normal
  title calls, while negative and zero flags still call `296cd`/`2a2b4`.
  Do not give the title's negative idle-demo flag the positive-mode route.

Consequently, starting a demo from the already-running title must not reload
configuration, parse startup arguments or replay the opening warning. These
identifications come from caller order and instruction inspection; they are
not a claim that the complete process owner has run under an oracle.

Two race-time readers have more specific contracts than “skip demo UI”:
`25067` calls the filename/record HUD renderer `2adbe` for zero **and negative**
flags; only positive flags skip it. `25316` bypasses the timed result-key wait
`2b73b(50)` for any nonzero flag and synthesizes scan `0x44`, which takes the
existing early-dismiss branch at `253f4`. The normal wait implementation is
already covered by `verify-result-wait`, and the HUD by `verify-dos-hud`;
their demo callers still need native wiring. Do not hide the normal lower HUD
merely because an idle demo is active, or wait for human confirmation there.

The supporting suites were rerun after this audit: configuration defaults,
save streams and full-loader comparisons; 192 result-wait sequences; 768
composed HUD pixel transitions plus its component suites; and the existing
demo setup, input, timer and eleven-reader branch tests all pass. No native
executable, pacing or startup behaviour was changed by this documentation
audit. The caller distinctions above remain integration requirements, not
claims of a functioning native demo.

`make verify-demo-return` now executes the original `25552` return branch
through `25a05`, including the complete reset routine at `1c10b..1c24a`.
All 256 flag bytes and four patterned data segments pass (1,024 cases).
The test compares the entire 64 KiB data segment against the reset contract,
including all four participation combinations (inactive, positive and negative),
per-car damage/motion fields and timing defaults. The zero-flag case stops at
the normal-results entry `2555c` and must leave the data segment unchanged.

For every nonzero flag, reset precedes the saved-image restore at `34f15`.
The latter alone is stubbed: its image handle from DS:4c1c, destination from
DS:1d87 and six zero geometry/default arguments are checked, as is AL=0 on
return. This is not a palette load or screen clear. Actual saved-image pixels
and native publication are not covered by this test. The branch bypasses
`2555c..259dc`; it does not establish suppression of earlier finish-event
rewards. Native demo return must preserve the reset/restore sequence as well
as the separate configuration restoration. Live integration remains open.

`verify-title-demo` now executes eleven additional original flag readers for
all 256 byte values (2,816 branch comparisons). It stops before either branch's
side effects, so these are classification proofs, not completed native demo
integration or proofs of the called functions. Addresses below are linear
addresses in the loaded runtime, with DS:0459 as the tested byte.

| Reader | Branch condition | Taken boundary | Other boundary |
| --- | --- | --- | --- |
| 19db0 | signed flag >= 0 | 19dbd | 19db8 |
| 1b540 | flag != 0 | 1b562 | 1b547 |
| 1b562 | signed flag >= 0 | 1b58f | 1b56a |
| 25067 | signed flag > 0 | 25074 | 2506f |
| 25316 | flag != 0 | 2532a | 2531d |
| 25552 | flag == 0 | 2555c | 25559 |
| 25c05 | flag != 0 | 25c11 | 25c0c |
| 25d76 | flag != 0 | 25d8e | 25d7d |
| 25eaa | flag == 0 | 25eb4 | 25eb1 |
| 26175 | flag != 0 | 26185 | 2617c |
| 26304 | signed flag <= 0 | 26348 | 2630c |

In particular, 25552's nonzero route jumps to 259dd, bypassing the post-track
award block already identified in the completion audit. The reads at 25067 and
26304 distinguish positive mode from negative idle demo; converting the flag
to a boolean would erase this distinction. The earlier 23f65 race-key oracle
remains separate and passes along with 960 setup/restore pairs and 36,864 idle
timer comparisons. Log: `tmp/demo-consumer-branches.log`. No native runtime
behavior was changed and no emulator was launched for this host comparison.

### Title cadence: additional static boundaries

Original title loop 2a344 pushes 10 into 19878, which calls runtime 14be6;
that routine polls 14b97 against a computed time target. Rendering reaches
29ff3's call to 3b09c with two zero arguments. The display routine either queues
its register update through DS:1da3 when DS:1d9b==1, or directly polls VGA
status at 3b162 and 3b175 before its attribute-register write. The latter loop
tests vertical retrace bit 3. This is evidence that title cadence depends on
both a runtime delay and display publication, not just colour-counter updates.
The native full-cycle tests do not measure those PC timing boundaries. DOS
clock calibration, queue/interrupt mode and wall-clock comparison remain open;
no new pacing policy is inferred from this static inspection.

### Keyboard and F9

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
random-order flag values return 99 with selection unchanged. No submenu or
shuffle callback is stubbed on this path. Subsequent
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

The follow-up caller trace corrected an earlier interpretation of DS:0624:
it is the track Random Order flag, not registration. Original GO at `2a4e7`
calls playlist shuffle `26d34` when it is nonzero, whereas F9 bypasses the
shuffle. Production shuffled at startup and when enabling Random Order but
missed this additional GO boundary. The GO-only call is now restored before
empty-playlist fallback and race preparation. `verify-track-playlist` passes
576 GO/F9 caller comparisons (six list lengths, sixteen RNG seeds, three flag
values, both actions), comparing all 256 list words, count, RNG and return
value against the original instructions. The original real shuffle and RNG
execute without substitute callbacks. Its existing 480 playlist cases and
the title input/Help/F9 checks also pass.

The ordinary GO native regression passes after the shuffle fix: counts=15,
draws=30, checks=45, errors=0, options=1, starts=1, restore=31, with the
expected two-human/two-computer handoff. Run from `amiga/` using
`SLICKS_REGISTRATION_TEST=7 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/title-go-shuffle ./debug.sh '' diag_arcade_title.gdb`;
local evidence is `tmp/title-go-shuffle-native.log`. Muted FS-UAE exited.
This regression checks the native caller remains connected; the independent
original-instruction comparison above proves the shuffle/RNG semantics.

After removing mouse activation, the native `REGCHECKT` title-transition run
passes modes=31, roles=7, counts=3, checks=33, errors=0, restore=31. The
normal keyboard paths remain functional. Reproduction from `amiga/`:
`SLICKS_REGISTRATION_TEST=5 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/title-input-audit ./debug.sh '' diag_title_transitions.gdb`.
The run was muted and its emulator exited; local evidence is
`tmp/title-input-native.log`.

## Hidden Load Game entry audit

The supplied runtime contains the title's row-4 Load handler at `2a51e`, calling
`1987:40c4` (`1d934`). That routine invokes the saved-file chooser and selected
file loader; its existence does not establish a reachable menu entry. The title
starts with selection zero (`2a2be`). Its eight shortcut table entries select
exit, row activation, Help, F9 race start or F12 demo, not Load directly.

`verify-title-navigation` now computes closure over original-instruction mode
navigation (`2a25b`) and action mapping (`2a28f`), starting row zero in all six
modes, allowing all 256 scans, and permitting Options to return any mode with
the caller selection unchanged. It checks the actual shortcut table as well.
34 states / 17,152 transitions are reached; none selects row/action 4. This is
a title-owner boundary proof, not whole-program or runtime-patch reachability.
Independent exhaustive helper comparisons still pass (17,920 normal navigation,
51,200 mode-navigation and 655,360 action-map cases).

The former CHAMPLOAD/CHAMPEDIT sequence of four Downs then Enter relied on the
removed port-only row; it cannot be counted as current native resume evidence.
Do not fix it by inserting that row or injecting selection 4 and claiming a
faithful user route. An intentional accessibility extension requires the user's
choice; other possible original entry points remain an audit question. Repeated
save/overwrite/delete testing can instead begin at a real intermission without
depending on this unresolved Load entry.

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

## Demo race renderer integration — 2026-09-29

The race renderer now consumes a signed demo flag and the original extracted
DS:0bff label. Negative flags replace the ordinary Arcade overlay at its
existing race-start/update call sites. The original two text passes use the
race's `kirj` font and current palette, leave its foreground colour changed,
update the authoritative chunky surface and optional logical mirror, and
report a narrow rectangle through the normal dirty publisher. New runtime
fields are appended to preserve existing assembly field offsets. The exporter
also extracts DS:09a0 for subsequent loading-caption integration; original
bytes remain in ignored generated assets, not tracked source.

`make verify-demo-pixels` passes 1,024 comparisons across every negative byte
flag and eight palette/background patterns. It executes the original overlay,
nearest-colour lookup and text/glyph code against the production host renderer
and the translated 68020 text/glyph routines. Every visible pixel and complete
runtime font buffer matches. The test also checks logical-buffer mirrors,
changed-pixel dirty coverage and a bounded C2P-aligned publication area, and
changes the palette after binding to reject stale cached colour choices.
The initial fixture incorrectly supplied a truncated font; that fixture error
was corrected before accepting the comparison.

`verify-demo-overlay`, `verify-dos-hud`, `verify-arcade-hud` and
`verify-dirty-tracking` pass alongside the Amiga build. The native fixture
`diag_demo_overlay.gdb` checks the negative flag selected by the explicit
`SLICKS_DEMO_RENDER_TEST=1` launch mode and runs the existing dirty-sprite
publication audit. The initial debugger-write fixture did not activate the
overlay: its F1/CITY 600-update passes are ordinary-rendering regressions only,
and its WHACKO run was stopped. Corrected native runs now pass 600 updates on
F1 (32 actors, 2,068 marks), CITY (18 actors, 1,480 marks) and WHACKO
(5 actors, 1,854 marks), with no chunky/bitplane mismatch. All three log
`DEMO_RENDER_BIND_OK`, `DEMO_RENDER_STATE_OK`, `DEMO_RENDER_TEXT_OK` and
`DIRTY_SPRITE_AUDIT_OK`; the owned muted emulators closed afterward. Logs:
`tmp/demo-overlay-{f1,city,whacko}-native.log`. Reproduce with
`SLICKS_DEMO_RENDER_TEST=1 SLICKS_TRACK_ACTOR_TEST=1 SLICKS_LIVE_STATS=0`,
`SLICKS_TRACK_ACTOR_CASE=1|2|3` and `diag_demo_overlay.gdb`.
The fixture
now asserts the bound flag, actual rendering state and first text-call arguments
without debugger writes. This is explicitly renderer
coverage, not a claim that F12/idle entry, configuration restoration, input,
rewards, results or persistence have been integrated. Those remain open.

## Demo return field mapping exposed a renderer discrepancy (2026-09-29)

The original return reset `1c10b..1c24a` clears DS:303f/3041, which is
**fixed-point speed**, not the current lap clock. Existing original damage
comparisons (`verify_dos_damage.c`) establish that mapping. DS:3037/3039 is
the last lap duration. A blanket reset of all native clocks would therefore
change state that this original routine does not reset.

DS:3068/3069 are also not weapon selection fields. Their consumer at
`23d97..23e7a` divides the current heading by 1200, initializes a negative
stored direction (or immediately replaces it for negative/human participation),
then uses the stored direction for the actor update. A differing current
direction adds the update's low-byte timestep to a wrapping byte counter.
Only **after** the actor update, a signed counter greater than five replaces
the stored direction and clears the counter. A frame whose directions match
does not otherwise clear the accumulated counter.

Both native car drawing paths instead divide the live heading by 1200.
Their cache comparisons validate one native renderer against another and do
not cover this original caller-level direction state. This confirms F13 from
the original instructions and production consumers; the subsequent executable
comparisons and correction are recorded below. The new state belongs to
rendering, not steering, wheel-effect geometry or the simulation heading.

### F13 correction and verification

`car_display.h` now reproduces the original pre-submission and post-submission
state updates separately from simulation heading. `verify-car-display`
executes `23d9c..23e7a`, stubbing only the actor device call and inspecting
the frame argument at that boundary. Its 278,528 comparisons pass: every
byte counter/timestep pair for human, inactive and computer roles, all signed
word headings, all role bytes and repeated direction-change sequences.
The counter's signed test, wrapping addition, retained partial count and
one-last-old-frame threshold behaviour are included.

Production prepares the four display frames once per update; both the C
renderer and 68020 cache path consume them. The fields are appended to avoid
moving the existing assembly state. Initial preparation starts from -1/0,
while legacy isolated renderer calls without a prepared display state still
derive their frame from heading. Retention conflict geometry follows the
display frame too, not its potentially smaller physics-heading sprite.
Wheel-effect geometry and simulation heading remain unchanged.

Host checks pass: 4,096 car-cache pixel/state comparisons (including a
display frame different from heading), surface effects, dirty coverage,
10,000 retention-touch comparisons and 1,024 retention-group permutations.
A specific case verifies that a wider delayed car frame invalidates an
overlapping retained sprite outside the current-heading sprite's footprint.
Logs: `tmp/car-display-host.log`, `tmp/car-display-retention-host.log`,
`tmp/car-display-retention-groups.log`.

The first F1 native shadow run was **invalid**, not a pass: it reported zero
site-5 calls. Renderer shadowing requires a separate 64 KiB surface, whose
allocation was unchecked and attempted while temporary loading assets were
still resident. The fixture now allocates comparison buffers after those
assets are freed, allocates the surface only for rendering sites and rejects
allocation failure. The corrected F1 run passes 2,800 site-5 comparisons with
zero mismatches over the 603-update benchmark. The normal-build F1 audit
passes 600 updates (32 actors, 2,068 marks) and observes a delayed frame at
update 164, driver 2 (heading 9302, displayed frame 8). CITY passes 600
updates too (18 actors, 1,480 marks), with a delayed frame at update 176,
driver 2 (heading 18902, displayed frame 0). WHACKO passes 600 updates
(5 actors, 1,854 marks), observing its delay at update 147, driver 2
(heading 18902, displayed frame 0). All three runs report exact
chunky/bitplane agreement.
Logs: `tmp/shadow-car-display-buffers-1.log`,
`tmp/car-display-native-{1,2,3}.log`. Reproduce the latter with
`SLICKS_TRACK_ACTOR_TEST=1 SLICKS_TRACK_ACTOR_CASE=1|2|3 SLICKS_LIVE_STATS=0`
and `diag_car_display.gdb`; the fixture rejects an unexercised delay.

The F1 target retention-versus-full-redraw comparison also passes: 603
updates, zero pixel/particle mismatches, 575 geometry checks and 700 status
cache checks with zero mismatches, and unchanged immutable maps. Its final
positions and 2,244 marks match the native/reference run. This is a
correctness run, not a performance measurement. Log:
`tmp/car-display-retcheck.log`. The harness restored a successful normal
build (`tmp/car-display-final-build.log`) after closing its emulator.

The retention snapshot now also preserves the mutable tail appended after
the immutable terrain arrays, including demo and car-display state. Its host
test overwrites that tail before restoring and compares the entire runtime;
all mutable bytes restore and mutations to each excluded terrain array are
detected. This prevents the two-pass checker from advancing display state
twice and verifies the tail rather than silently omitting it.

### Native demo-return reset

`race_return.h` translates the mapped `1c10b..1c24a` reset, including the
new display-direction/counter state. `verify-demo-return` now compares its
native state against the original reset in all 1,024 existing flag/target
cases, in addition to the full original DS-byte comparison and saved-image
call assertions. Sentinel-filled unrelated native bytes must survive exactly;
positions, current/total clocks, inventory, RNG and the submitted display
frame are not cleared. Native derived speed/time/rank views are kept consistent
with the original reset fields. Log: `tmp/demo-native-return.log`.
At this checkpoint this was a verified reset helper, **not** a live demo-return route;
the title owner, saved-image restoration and setup-selection refresh remain
to be connected.

## Approved Amiga demo shortcut adapter (2026-09-29)

Shift+F1 and Shift+F2 map to original F11/F12 in the title/demo scan
adapter. Physical driving bindings and text entry retain their old mapping.
The keyboard interrupt snapshots both Shift keys into each queued event;
polling exposes the event-time state even if Shift has already been released.
The producer updates that state even when the queue drops a release, so later
events cannot inherit a stuck modifier. Synthetic diagnostic events have zero
modifiers. Shift itself maps to the non-command scan 0x80 in this adapter,
allowing the chord without making Shift alone a demo-exit command.

The title dispatcher now uses the adapter. F12's actual start action and the
demo race's view/exit consumers still require lifecycle integration; this
change does not enable or certify those routes. `make verify-amiga-key-scan`
passes 1,024 modifier/mapping combinations, delayed consumption with both
Shift keys, dropped-release recovery, and the existing Help/page-key/physical
binding regressions. The Amiga build passes. Native input/lifecycle coverage
remains part of the live demo work.

## Live keyboard demo entry, views and return (2026-09-29)

Shift+F2 now reaches the original demo setup through the normal title owner
and real race preparation. The temporary profile IDs precede the original
Arcade selection override; the diagnostic does not force four computer cars.
The actual race receives the negative demo flag and overlay. Finish-event
rewards remain connected, while the later track-reward callback is absent.
Overlay-only fixtures use a separate flag, avoiding an uninitialized demo
configuration backup.

During a demo, Shift+F1/F2 publish the original material/surface data views
from live maps, converting only the 320x190 plot and preserving the HUD.
Other accepted exit keys stop audio, invoke the verified race reset, restore
configuration and playlist, clear the flag, and refresh restored selections
through the original choose=0 ordering. The title is rebuilt from resident
assets without releasing hardware ownership. The shared RNG is not rewound.
Preparation failure and native program exit also restore temporary settings;
their failure/persistence consequences still need dedicated native tests.

`SLICKS_DEMO_LIFECYCLE_TEST=1 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/demo-lifecycle-pixels ./debug.sh '' diag_demo_lifecycle.gdb`
(from the configured amiga environment) passes two real entries, four views,
two key returns and system restoration. The fixture queues event-time Shift
chords, compares the complete configuration and all 256 playlist slots after
each return, and rejects normal-results or setup-save calls. At every view it
checks all 60,800 plotted indices against the map and all 64,000 displayed
pixels reconstructed from the actual eight bitplanes against chunky pixels.
Log: `tmp/demo-lifecycle-pixels.log`; build: `tmp/demo-lifecycle-build.log`.

The title-demo, setup-session, title-navigation, Amiga-key-scan, track-data-view
and demo-return host suites pass (`tmp/demo-lifecycle-host.log`), retaining
their independent original-instruction comparisons. The muted native run
exited and its emulator was closed.

This fixture exits after ten updates: it does **not** establish natural demo
completion, idle activation, loading-caption fidelity, all failure/exit paths
or exact saved-image/title return presentation. Those remain explicit open
work; the live return currently reconstructs the title from resident assets.

## Automatic title demo timing (2026-09-29)

The live title owner now calls the independently verified elapsed-time scan
replacement. Time comes from PAL VBI interrupts, quantized to whole seconds,
not title updates or rendering throughput. The first eligible elapsed value
is 21,000 ms; an overdue scan replaces simultaneous input as in the original.
No AmigaOS clock call or display teardown is needed. Non-title owners disarm
the interval; re-entry and the tail after handled title input restart it,
including return from a synchronous modal. Gameplay does not perform the
clock division. This is a PAL-target clock adapter, not a promise of matching
the DOS machine's wall-clock epoch or fractional-second phase.

The `DEMOIDL` native fixture (`SLICKS_DEMO_LIFECYCLE_TEST=2` with
`diag_demo_lifecycle.gdb`) waits ten real emulated seconds, sends an ordinary
Down key, then waits for automatic entry, twice. It does not inject a time
value or directly request a demo. At entry, before disk loading, it requires
at least 1,000 VBI ticks since that input and exactly 21,000 quantized
milliseconds since the owner reset. Each race then exercises both data views
and key return with the same full configuration, playlist and displayed-pixel
checks as the keyboard fixture. Long nested-menu/synchronous-modal waits
remain separate coverage work, not implied by this navigation test.

Build and native logs: `tmp/demo-idle-build.log`, `tmp/demo-idle-native.log`;
keyboard regression: `tmp/demo-idle-keyboard.log`. Independent title-demo,
title-navigation and Amiga-key-scan suites: `tmp/demo-idle-host.log`.
Both native runs passed two starts/four views, exact configuration/playlist
restoration and normal system cleanup. The idle run recorded two automatic
entries and 1,040 VBI ticks after the last navigation input; the keyboard run
recorded zero automatic entries. All three host suites passed. Both muted
emulators exited and were closed.

## Idle timing across Options and nested Help (2026-09-29)

`DEMO MNU` (command-line spelling `DEMOMNU`, debug selector
`SLICKS_DEMO_LIFECYCLE_TEST=3`) navigates from the title into Options and its
F1 Help using ordinary queued keys. It leaves Help open for 1,100 PAL VBI
ticks, closes it, leaves Options open for another 1,100 ticks, then returns
to the title. It requires both owners to remain present without a demo during
the waits. After title return it verifies the full new idle interval before
disk loading, then exercises both demo data views and key return. A bounded
watchdog rejects unexpected race entry or failure to reach a menu stage.

Two complete repetitions pass: four long menu waits, two automatic entries,
four track-data views, exact configuration/playlist restoration, displayed
pixel comparisons and normal system cleanup. The last post-menu idle wait
was 1,021 VBI ticks; the original-style quantized elapsed value was 21,000 ms.
Logs: `tmp/demo-menu-idle-build.log`, `tmp/demo-menu-idle-native.log`.
The muted emulator exited and was closed. These asynchronous Options/Help
owners do not establish coverage of synchronous modal-call returns.

### Loading presentation follow-up

Direct instruction inspection at `1b488..1b4e9` confirms two calls to the
original tint service `34654` before the caption: rectangle arguments
`(118,95,218,115)` with RGB `(20,20,20)` and percentage 50, then
`(110,90,210,110)` with RGB `(35,35,60)` and percentage 75. At
`1b4ec..1b519` the caller chooses the nearest `(60,60,40)` palette entry and
sets font DS:0680's colour. The signed-flag caption selection follows at
`1b540..1b58f`, centered at `(160,97)`. Thus the open loading presentation
work includes both background remaps and their lifetime, not only the text.
This inspection is not native implementation or a pixel-verification claim.

## Loading painter original/68020 comparison (2026-09-29)

`src/ui/loading_presentation.h` reproduces the complete ordered drawing
sequence at `1b488..1b58f`: both background tint rectangles, nearest-colour
selection, font colour mutation and centered ordinary/demo caption. Positive
flags suppress only the text; the tints and font mutation still occur.
The owner must provide its actual font, live palette and original labels.

`verify-loading-pixels` executes the original tint-table lookup/remap and
font routines, adapting only VGA accesses. The native side uses the shared
chunky tint renderer and actual 68020 font code with the original `kirj.@f`
asset. It compares all 64,000 pixels and the complete font state. Original
filename/suffix construction executes too, but its result is supplied across
the native renderer boundary: this does not validate native filename assembly.

The initial exhaustive run passed all 256 flag bytes with two palette/pixel
patterns (512 comparisons; `tmp/loading-pixels.log`). The committed bounded
fixture covers flags 0, 1, 127, 128 and 255 on both patterns and additionally
requires the original CPU to reach the exact endpoint. Exhaustive signed-flag
branch coverage remains in `verify-title-demo`. Final combined results are
in `tmp/loading-pixels-final.log`.

This is an isolated painter, not live loading-screen completion. Native race
preparation currently ends hardware ownership and restores the AmigaOS view
before disk loading; drawing this panel just before that would immediately
erase it. Live integration still needs the display lifetime, actual font
alias and filename construction resolved and verified. No emulator was
started or release executable replaced for this host/CPU-oracle work.

## Display-retaining disk-I/O handoff diagnostic (2026-09-29)

The platform now has `begin_io`/`end_io` boundaries separate from final
hardware teardown. They restore OS keyboard/VBI service, interrupts and
scheduling for file I/O, without loading the saved OS View or changing the
custom copper pointers. Raster/copper DMA remains enabled; OS sprites and
audio DMA stay disabled. The caller must first stop its Paula playback and
finish blitter work. Returning reinstalls the native VBI/keyboard ownership
and gameplay DMA/interrupt mask. Final teardown closes an outstanding I/O
window before normal system restoration. This API is currently used only by
the explicit diagnostic, not ordinary race loading.

The OS scheduling boundary is deliberate: file I/O can call `Wait`, which
temporarily breaks `Forbid`; keeping `Forbid` alone is not a guarantee of
exclusive hardware access. See the official [Exec Tasks documentation](https://wiki.amigaos.net/wiki/Exec_Tasks).

`SLICKS_DEMO_LIFECYCLE_TEST=4` / `DEMOIOS` enters the I/O window from the title,
reads the complete SLICKS.000 archive in 256-byte blocks, then waits 50
OS-serviced vertical blanks. It checks the unchanged bitmap and null OS View,
resumes native ownership and performs a real demo entry/views/key return.
The sequence repeats twice. The first run passed with 642,007 bytes and
rolling hash 811783429 each time, matching an independent host calculation
(`tmp/loading-io-native.log`).

The final fixture also requires an actual failed open of a child path under
the regular archive file, then verifies the full system-restoration status
31 after both cycles. It passes both I/O cycles, failed opens, two demo starts,
four data views, configuration/playlist restoration and the complete final
restore check; the muted emulator exits and is closed. Build/run logs: `tmp/loading-io-build.log` and
`tmp/loading-io-native-final.log`. No asset files are modified by this test.
An unchanged bitmap and null View do **not** prove that the hardware scanned
out the intended copper list throughout I/O. Visible scanout remains an
explicit integration gate; do not describe this as a completed loading screen.

## Loading caption construction and capture limitations (2026-09-29)

`slicks_loading_caption` now concatenates the supplied track stem with the
original DS:099c suffix, which the local asset exporter exposes alongside the
demo labels. The pixel oracle no longer borrows the constructed DOS caption:
it builds the native string independently, compares it with the result of
original `1b51c..1b53d`, then renders it. Five- and eight-character stems pass
all ten boundary-flag/palette cases. Atomic overflow rejection is also checked.
Log: `tmp/loading-caption-final.log`. This verifies the builder, not its
eventual production caller's choice of track stem or font alias.

Visible-scanout verification remains open. The desktop app inventory did not
expose the command-line FS-UAE window. A later emulator-side Lua attempt
reached its native checkpoint and completed cleanup but produced neither a
hook marker nor a screenshot; the installed binary also lacks the expected
Lua interface symbols. The unsupported capture script was removed rather
than retaining a falsely passing capture test.

`diag_loading_io_visual.gdb` is a manual checkpoint for diagnostic mode 4:
after the actual I/O and 50 OS-serviced refreshes, it verifies the active I/O
state and null OS View, holds for 30 host seconds, then continues the normal
lifecycle/restoration checks. A user visual check has been requested. The
earlier checkpoint run completed (`tmp/loading-io-visual.log` was subsequently
reused by the failed Lua capture attempt); neither run establishes visible
scanout. All emulators started for these attempts were closed.

## Natural demo completion and title return (2026-09-29)

`DEMOEND` / `SLICKS_DEMO_LIFECYCLE_TEST=5` starts demos through the ordinary
Shift+F2 title route, exercises both track-data views, then injects no exit
key. The native game must reach its real completion condition; only the
production automatic-return route may leave the race. The fixture captures
frame count, game clock and deadline before the return reset and rejects
an early return, a nonexpired deadline or a 15,000-update overrun.

Two natural returns pass: frames 2,523/2,115, clocks 4,593/3,850 and deadlines
4,591/3,848. Both restore the entire configuration and all playlist slots;
four data views pass the existing full displayed-pixel comparisons. Normal
results and setup-save calls remain excluded by the lifecycle audit, and the
final OS restoration mask is 31. Debug runs are muted; the emulator exited
and was closed. Build/native logs: `tmp/demo-natural-build.log` and
`tmp/demo-natural-native.log`.

Independent regressions also pass: 6,480 original composed completion
sequences with 55,296 line crossings across all six modes, active masks and
finish orders, plus 1,024 original/native demo-return reset cases
(`tmp/demo-natural-host.log`). These establish the bounded natural deadline
route, not pixel identity of the reconstructed return screen or production
program-exit/save-failure handling. Diagnostic modes intentionally suppress
implicit setup saves; their absence here is not proof of every persistence
caller. Those remaining checks stay in the actionable list.

## Failed demo preparation, restoration and retry (2026-09-29)

`DEMOERR` and `DEMOHUD` (`SLICKS_DEMO_LIFECYCLE_TEST=6` and `7`) exercise
two existing real-loader fault boundaries on the first demo attempt only:
an actual Open of a missing track, and a failed HUD resource lookup later
in preparation. No original/test assets are renamed, deleted or rewritten.
The next attempt uses its normally selected track and real resources.

Each fixture must show the real load-error owner with the expected code
(2 or 6), with the demo flag already cleared. It dismisses the error using
ordinary Enter, compares the entire restored configuration and all playlist
slots, then starts another demo from the title. The successful attempt checks
both data views, key return and configuration/playlist restoration again.
The debugger requires exactly one preparation error, one started race and
two views; an unexpected second error fails immediately. Final OS restoration
must be 31, and ordinary results/setup-save calls must not be reached.
Both fixtures pass all these checks. Runs were muted and both emulators
exited and were closed.

Logs: `tmp/demo-preparation-failure-build.log`,
`tmp/demo-preparation-failure-native.log`, `tmp/demo-late-failure-native.log`.
This covers early and partially initialized loader failures, not injected
allocation failures, all malformed assets, or production program-exit/save
failure. The latter remain separate open checks. Diagnostic implicit-save
suppression remains in force, so this is not an end-to-end persistence test.

## Demo program exit and save-failure cancellation (2026-09-29)

`DEMOSAV` (`SLICKS_DEMO_LIFECYCLE_TEST=8`) makes a real Options volume edit
using normal menu input, then enters a demo and exercises both track-data
views. It requests program exit through the native exit-request route rather
than injecting a demo-return key. At each setup-store entry, the debugger
compares the entire configuration with the pre-demo snapshot and requires
the temporary demo state to have been cleared.

The first save encounters an AmigaDOS-created `SLICKS.CFG.new` directory
obstruction in the isolated test disk. The real failure dialog appears;
ordinary Escape cancels it. The fixture checks full configuration and playlist
restoration, enters another demo, checks both views again, and exits. The
second save succeeds. It reopens the actual 142-byte configuration file and
compares every byte with the normal serialization of the pre-demo settings.
This is disk readback, not a fresh-process reload test. Only the obstruction
created by this fixture is removed; original assets are untouched.

The muted native run passes: two starts, four verified views, two save calls,
one save failure, successful file readback and final OS restoration mask 31.
The emulator exited and was closed. Logs: `tmp/demo-save-build.log` and
`tmp/demo-save-native.log`. An initial fixture expectation incorrectly assumed
a volume step of one; it was corrected to the original option table's step
of five. No production option behavior was changed.

This closes the bounded dirty-exit/failure-cancel/re-entry/success sequence,
not clean exit, direct retry, allocation failures or exact saved-image return
presentation. The shared lifecycle audit now also requires its completion
checkpoint before accepting system restoration, rejecting early exits.

## Adaptations to preserve or explicitly classify

- User-requested: Paula four-channel priorities without software mixing,
  hardware takeover, display-end refresh limiting, native installation/WHDLoad.
- Defensive storage/parsing: atomic saves, recovery files, bounds checks and
  explicit resource/I/O errors. Do not reproduce memory corruption or destructive
  partial writes merely to mimic malformed-input behaviour. Normal-game UI and
  semantics must still be compared with DOS.
- Deferred or unavailable evidence is not a supported-feature claim: manual
  joystick verification and the absent external order-form image remain so.
