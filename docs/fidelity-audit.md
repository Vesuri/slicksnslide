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
| F08 | Title animation cadence is not the full original title loop | Existing registration evidence explicitly excludes this; original `29f2c` selects between two renderers, while production uses a bounded translated title path. Detailed animation/state coverage remains unverified, not a completed feature. |
| F09 | Main-title label and counter shadows forced to black | Original `297c4..297e8` sets DS:1600 to nearest (10,10,20); native title font wrappers passed zero instead of the translated third-colour result. Corrected below. |
| F10 | Arcade title uses the ordinary title renderer | Original `29f31..29f47` calls the mode-5 predicate at `198b6`, then selects `29afa` instead of `29753`. Native presentation always uses the ordinary title; this also needs its associated input routes audited. |

The first pass also finds hardwired `lang1.txt` in live pause/intermission.
This is a **candidate**, not yet a confirmed bug: audit the original language
selection/startup consumer before changing it. Track catalogue sorting, limits,
shortcut routing and platform error screens need the same caller-level check.

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

The native UIMENU2 lifecycle run **did not pass**: it exits after phase 3,
before the normal successful open, with restoration status 3. Read-only
allocation tracing found 119,064 bytes free but a largest block of only 60,272
bytes: the 65,536-byte preview arena cannot be allocated. This fixture retains
an extra 64,000-byte verification snapshot, in addition to the actual menu's
saved screen. Its fault-injection phases can therefore return early for an
unintended allocation failure. Fix its memory arrangement and require each
injected failure to reach its intended boundary before claiming the old
17-phase failure/reopen test passes.

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
