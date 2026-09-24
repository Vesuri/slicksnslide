# Native player setup completion

Player-setup goal: configure a game entirely through native menus, save,
restart and recover the same configuration, then start a race with the
correct drivers, vehicles and colours.

Closeout (2026-09-25): the user explicitly deferred the manual joystick
check after reporting that the host keys may conflict with the window
manager. No successful joystick input is claimed. With that check deferred,
the requested native menu/configuration/save/restart/race-setup goal is
complete on the evidence below. Championship save/load and other broader
gameplay work remain separate, unfinished work.

## Current requirement audit

This section is the current closeout inventory. Entries further below are
chronological evidence and may describe gaps that later entries close.

| Requested requirement | Current implementation and evidence | Remaining limitation |
| --- | --- | --- |
| Original name-entry and colour-picker dialogs | Connected production actions; original pixel comparisons; native create/edit/reopen/cancel; allocation and late-open warning/retry tests on ports 25124–25127 | No known missing normal dialog action; this is not a claim of arbitrary-corrupt-resource recovery |
| Add/Edit acceptance, cancellation, repeated editing | `slicks_profile_editor_key` and `slicks_finish_profile_edit` drive production; native name/colour lifecycle observers pass after fault recovery | Preserve original immediate-property versus local name/colour edit semantics, rather than promising a blanket transaction on Escape |
| Profile/configuration persistence and failures | Native CFG/PLR load/save; defaults, invalid input, short writes, write-protected storage, recovery leftovers, retry/return; clean save/restart passes with all 142 CFG bytes and profile bytes compared | Configuration saving is not championship Save Game |
| Remaining setup screens, options and Help | All direct pre-race menu actions below have connected handlers and recorded native/original-code gates | Pressed joystick routing is not yet proven; gameplay consumers such as weapon firing/shop are separate unfinished functionality and must not be advertised as implemented |
| Human, computer, inactive and shared profiles | Unique profile moved through all four slots; mixed/shared Human/shared Computer and random vehicle save/restart/race cases recorded | Pressed-joystick human driving remains unverified; selecting a joystick is not evidence of actual input |
| Configure, save, restart, recover, start correct race | Clean-start ports 25129/25130 on the dialog-recovery build verify actual participation, vehicles, endpoints and active palette ramps; exact CFG/profile comparison | Representative mixed-player completion case; other combinations have their separate recorded gates |

### Direct setup action inventory

Audited against production dispatch in `slicks_diag.c` and the corresponding
original-code-derived UI helpers, not just searches for TODO strings:

- Title: GO, Players, Tracks, Options, Read This, Quit, contextual F1 Help.
- Players: all four assignments, vehicle shortcut, Add, Edit, Delete with
  confirmation, exit and Help. Driver-picker Escape returns the initial
  selection (`flags & 1`); it is not the negative result used by action lists.
- Editor: name, type/properties, vehicle, both colour endpoints and original
  acceptance/cancellation. Child open failures now retain the editor.
- Tracks: toggle, Lists, random ordering, All, Clear, random count/selection,
  Records/preview, exit and Help. Lists provide load/save/delete/cancel.
- Options: fifteen option records, Controllers, Clear Top 10s confirmation,
  exit and Help. Controllers provide device selection, key capture, Defaults
  and exit. Help entry points resolve original topics and support navigation.

No missing direct pre-race action was found in this dispatch audit. This is
not proof of every action's gameplay consequences: weapons firing/cycling,
shop, natural championship results and championship Save Game/load remain
unfinished in the broader port. The only explicit placeholder found in the
examined menu dispatch is `SAVE GAME NOT IMPLEMENTED` in the between-race
intermission, not the CFG/PLR setup-saving path.

### Joystick verification status

Deferred by the user. The replacement test on port 25141 reached
`JOYSTICK_READY` and its FS-UAE window was brought forward. The user reported
trying the keys but possible window-manager interference, then requested
that we skip this check. No passing input sequence was recorded. The isolated
emulator (PID 51113) was stopped; other emulators were left alone.

The preceding mapped-key run on port 25140 reached `JOYSTICK_READY` with
`axis_throttle=0`, then timed out at stage 1 with controls zero. The user
offered to test; successful host input delivery has not been confirmed. This
is neither a passing test nor evidence that joystick decoding is broken.
The installed emulator's built-in keyboard joystick uses arrows and right
Ctrl/right Option for fire; this saved configuration uses fire for throttle.
The old F6/F7/F8 configuration is not used by current `debug.sh`. The test
window was left open for inspection, but its GDB observer has ended; a new
observation run is needed to record a successful press/steer/release test.
The host key helper lacks event-posting permission. The available desktop
control interface also does not expose the running standalone FS-UAE binary
by name or executable path. No permission settings were changed. A manual
press/steer/release check has been offered and awaits user availability.

## Original implementation sequence (historical)

1. Connected: original name-entry wrapper, text field, cursor and keyboard
   boundary; Add/Edit create, rename, reopen and cancel pass on A1200.
2. Connected: original colour-picker wrapper and RGB controls for both
   endpoints; local-copy acceptance/cancellation and repeated editing pass
   DOS full-pixel comparisons and the A1200 gate.
3. Complete profile and configuration load/save lifecycle and original dirty
   state. Exercise missing, malformed, truncated and unwritable files in an
   isolated filesystem. Do not overwrite supplied reference files. Transaction
   fault tests (2,600 cases) and the Amiga DOS adapter now exist. Startup now
   loads CFG and PLR together, retaining defaults atomically on failure and
   reporting errors before hardware takeover. Save-on-exit and a native
   retry/return screen are connected; a profile created/coloured/selected
   through menus now survives a fresh Amiga process and reaches race setup.
   Broader option coverage and remaining failure/recovery UI gates stay open.
4. Inventory and port remaining original setup screens, options and help;
   remove pending modal actions and ensure every displayed action works.
5. Verify all four player assignments across human, computer, inactive and
   shared-profile cases, including vehicle/random choices and colour ramps.

### Accumulated setup and broader-port follow-ups

The current requirement audit above supersedes stale status statements in
this accumulated list. Broader gameplay work is retained here for traceability,
not silently counted as completed by the pre-race menu audit.

Championship load follow-up (2026-09-25): `slicks_load_game_bytes` now decodes
the original .SSS stream into caller-owned state and track-name storage. It
validates signature, count, lengths, every player record and full consumption
before changing either output. This is format decoding, not a completed load
menu: catalogue lookup, human-profile matching, original invalid-vehicle
fallback and live championship resume still need integration. The reader was
inspected at 1d20c..1d586; its partial writes on errors are intentionally not
reproduced. Tests use streams from the original writer, not just a round-trip
between two native implementations.

The selected-file OS loader is now implemented as
`slicks_amiga_load_saved_game`. It bounds reads, distinguishes missing files
from an empty/default game, refuses .new/.bak recovery leftovers, suppresses
OS requesters only during the operation, and publishes neither game nor
track-name storage on failure. The actual adapter passes 2,310 injected
read/allocation/close-failure and truncation cases, plus overflow/missing and
recovery checks. Existing 1,485 save-transaction cases still pass.

`saved_game_resume.h` now resolves stored names to current catalogue/profile
indices before live state is changed. Original code establishes **ASCII
case-insensitive, last-match** lookup (not first match), skipping profile zero;
human names must exist, computer entrants select profile one and inactive
entrants retain -2. Invalid signed vehicle bytes fall back to vehicle zero.
`make verify-saved-game-resume` executes the original reader's lookup regions
and string routine: 3,392 track/profile comparisons and 2,560 vehicle-fallback
comparisons pass. This is a tested resolution boundary, not yet a connected
load menu or live championship resume. Those integration items remain open.

- Intermission dispatch and composition have original-code differential gates.
  The Amiga owner and nested Change Cars now have a native resource/lifetime
  gate (below). Normal race-loop input and localized actions are now connected.
  Live repeated vehicle edits now reach the second race, with profiles unchanged.
  Save Game now has an original-byte serializer and failure-tested disk adapter,
  but its dialog, loading/resume and live storage checks remain unfinished,
  along with broader integrated failure coverage.

- Records-modal surround and records panel now pass full-screen/font original
  comparisons, with preview rendering verified separately for 194 tracks.
  Retain the verified allocation/resource recovery gates; RAILROAD's selectors are proven
  to exceed the captured original resource range and trigger stale-buffer drawing
  (see the resource-failure investigation below).
  Retain the large saved-list storage/restoration regression below. Basic two-track progression is
  connected and verified. Original standings/shop remain open; inter-track
  vehicle changes now pass the live repeated-edit gate below.
  The original in-race menu is a six-option pause menu, not a Yes/No abort prompt; its
  original dispatcher, renderer and language lookup have verified
  translations. Live nested dialogs, resume and a two-track skip/end sequence
  now pass, as do repeated live opens and in-race controller/Speed save/restart;
  Help allocation and Controllers allocation/resource failures now also pass
  live warning/dismiss/retry gates. Other late child failures remain unaudited.
  Initial pause-load failures now preserve the running race and permit retry;
  five native failure boundaries pass below, including a visible notice that
  uses resident resources and restores the race exactly on dismissal.
  Next-track load failure now retries without repeating rewards or selection
  refresh; the two-race retry/state-preservation gate passes below.
  Initial GO failure now rolls back setup and returns to usable native menus;
  early and late resource-failure retry gates pass below.
  Tracks, Lists and the records modal are connected and tested below.
- Help topic-entry composition, chapter/page/link refresh and saved font
  restoration now match the original below; retain the native close/resource
  recovery regressions during remaining integrations. Native Help
  open-failure recovery passes in Options, Players, Tracks and Title;
  title navigation-failure recovery also passes with exact restoration.
- Audit remaining Options consumers, particularly inter-track
  vehicle choices. Arcade clock/HUD/end policy and save/restart are verified;
  Sounds/BG Sounds and Clear Top 10s are also connected and tested below.
  Collisions now reaches the actual car-pair gate and passes native Off testing.
  Weapons now reaches the live enable flag, original capacities and verified
  initial selection/RNG path. Firing, cycling input and shop consumers remain
  unfinished; do not claim full Weapons support from the setup/HUD gates.
- Expand native player-selection permutation coverage. Unique-profile moves
  through all four slots now pass with displaced shared/None selections and
  fresh save/restart/race handoff. Shared AI and shared Human scenarios also
  pass, including inactive slots and active-driver palette ramps. Saved
  random-once selection now passes a native restart/race check alongside
  shared random-per-race computer selections, including exact RNG draws.
- Live unwritable saves and CFG/PLR recovery-leftover startup checks now pass;
  retain those regressions during the remaining setup integrations.
- Close pressed-joystick integration checks. Original controller preparation,
  redraw and all key-capture prompt pixels now match below; edited controls
  already pass the combined save/restart/race gate. Edited volume settings
  also pass a fresh save/restart/race gate below.
- Run the full configure/save/restart/race completion audit after these
  integrations. Existing end-to-end cases are evidence, not full completion.

## Completion evidence

### Post-recovery clean configure/save/restart/race

Preserved the earlier clean fixture in `.run/setup-clean-before-dialog-recovery`
and recreated `.run/setup-clean-v1` without CFG/PLR seed files. Ports 25129/25130
repeat the real menu creation/colour/assignment/save and fresh-process race
checks on executable SHA-256
`27de6e81bd5d22f62b8f71a201ca23935f853810430b97a53e26e01c255eecf1`.
Both observers exit successfully, including all six displacement checkpoints,
save-run restoration 0x1f and the race's actual roles/vehicles/palette ramps.
All 142 re-encoded CFG bytes and the saved/reloaded profile dump match.
This closes the post-patch round-trip regression; it does not close the
no-input joystick test.

### Name/colour open failures return to the existing editor

Fixed a concrete setup failure path: name-entry and colour-picker open errors
previously jumped to whole-program cleanup. They now use the allocation-free
resident warning, retain the active editor, and let the user dismiss and retry.
Late colour-open failure also cancels/restores its painted rectangle before
freeing the child; it never commits the temporary RGB values.

Muted 2 MiB A1200 gates on ports 25124–25127 exercise name allocation failure,
colour post-paint failure, name post-paint failure and colour allocation failure.
Use `SLICKS_PROFILE_DIALOG_FAILURE=NF/NL/CF/CL` with the corresponding
`diag_name_failure.gdb` or `diag_colour_failure.gdb` observer. Diagnostic input
prepends dismiss/retry keys while retaining the original queued edit sequence;
the debugger does not write guest state. Each run reaches the warning once,
dismisses to the still-active editor, retries and completes the existing
create/edit/cancel/reopen checks. All debugger exits are successful.

For every run, the entire 64,000-byte editor surface and complete profile
table before failure compare exactly with their dumps after dismissal.
Final name and colour close snapshots also match their displayed bitplanes
at all 64,000 pixels. Target compilation, runtime guard, executable conversion,
shell syntax and diff checks pass. Tested executable SHA-256:
`27de6e81bd5d22f62b8f71a201ca23935f853810430b97a53e26e01c255eecf1`.

These checks close name/colour open-failure recovery, not every menu's resource
failure path or the outstanding pressed-joystick check.

### Latest-build player setup closeout rerun (ports 25122/25123)

Preserved the previous clean-start fixture in `.run/setup-clean-pre-closeout`
and recreated `.run/setup-clean-v1` without seeded CFG/PLR files. Muted
2 MiB/no-Fast-RAM A1200 runs of `PLAYERSY` and a fresh-process `SETUPR`
both passed the shared save/reload observers. Native menu input creates and
colours ABC, moves the unique profile through all four slots, and saves.
The save run verifies six assignment/displacement checkpoints and restoration
mask 0x1f. The reload verifies the actual race's human/shared-computer/inactive
roles, vehicles, profile colour endpoints and active palette ramps.

The saved/reloaded profile dumps compare byte-for-byte; all 142 CFG bytes
match the re-encoded native configuration before race preparation. The PLR
is 61 bytes. The tested executable and current build both have SHA-256
`67cf0301b50573f11b05ff277e824847228fe09072af3af5bed0f234ac877da0`.
Both debugger runs exit successfully. No Guru is observed in these paths;
this does not establish the cause of the user's earlier crash.

The previously running full dialog regression also completed successfully:
900 name, 72 message, 104 colour, 324 Options and 390 list full-screen/font
comparisons, plus Controllers preparation, redraw and capture-prompt checks.
This includes the generalized eight-character filename field as a component;
it does not claim connected championship saving.

Pressed joystick integration remains unverified. Rechecking the targeted
host-event helper still reports macOS event posting unauthorized; no events
were sent and no permissions were changed. A manual emulated-joystick check
has been offered. Remaining integrated failure paths and original screen/action
coverage still need closeout; this rerun is not a declaration that the entire
player-setup objective is complete.

### 2026-09-24: original championship .SSS writer and safe disk adapter

Recovered the original Save Game wrapper at 1d975: it calls the common
saved-file picker at 1d76a in save mode, appends `.SSS`, then invokes writer
1d587. The common picker supports existing-file selection, a new-name field
(eight-character limit), deletion with confirmation, and cancel. Its native
dialog orchestration is not connected yet; the intermission still reports
Save Game as unimplemented rather than claiming a save occurred.

`src/game/saved_game.h` translates the writer's byte format: magic 53/08,
big-endian track count, exactly eight catalogue bytes per selected track,
big-endian next-track index, then four driver records. Each record contains
up to 20 name bytes (including NUL only when encountered within that limit),
vehicle/role/profile position-scale bytes, points and cash words, and 13
inventory words. Original helper 362a5 emits the high byte first. The format
does not contain options, profile colour ramps, or RNG state; do not invent
those fields or describe this as a complete process snapshot. Caller-side
catalogue conversion and next-index construction remain to be integrated.

`make verify-saved-game` executes original 1d587..1d769 and the original
word encoder, hooking only fopen/fputc/fclose and catalogue-name lookup.
420 successful streams compare byte-for-byte with the native encoder, and
420 original open failures produce no output. Cases cover 0/1/2/64/256
tracks, every bounded name length, nonterminated/high-bit names, permuted
profile indices, role bytes and signed-word extremes. The original returns
to the exact sentinel CS:IP on each call. Native short-capacity rejection
leaves the destination unchanged for all 2,266 undersized capacities of the
maximum-size record; invalid counts and absent track storage are rejected.
This is writer verification, not proof of the original loader's semantics.

`slicks_amiga_store_saved_game` encodes into an exact-sized allocation and
uses the existing temporary/backup transaction. `make verify-saved-game-storage`
executes this production adapter against the AmigaDOS mock: 1,485 single and
double faults, short writes, first-file creation and recovery-leftover guards
pass. Every failure preserves either the old destination or its backup;
success/cleanup-pending states contain exactly the new bytes. No allocations
leak and report paths do not retain stack-local suffix strings. Invalid
serialization input causes no filesystem operation. The saved-list storage
regression also passes, and the target build, strlen guard and executable
conversion pass. Actual Amiga disk saves, dialog interaction, load/resume,
overwrite/delete confirmation and their failure UI are still required.

### 2026-09-24: live repeated Change Cars edits reach the next race

The `OPTIONSTI` mode (`SLICKS_INTERMISSION_LIVE=1`) extends the existing
two-track native menu sequence using only raw-key queue input. After the
first race's original pause/skip and profile refresh, it opens Change Cars
with F2, increments the first visible driver's vehicle, selects the second
visible driver and increments theirs, closes with Escape, reopens with F2,
increments the first driver again and closes with Enter. F9 then selects
Next Track. No debugger writes or direct dialog/vehicle mutations drive it.

`diag_intermission_live.gdb` passes on the 2 MiB/no-Fast-RAM A1200 on port
25120. It checks all nine key transitions, both modal closures and refreshed
parent icons. The initial vehicles 0/0 become 2/1. At the second actual
`slicks_race_start` boundary, the race cars have vehicles 2/1, not merely
updated menu icons. The RNG is unchanged throughout these fixed-profile
edits and the next preparation; each race receives its track reward once.
After 100 updates of the second race the normal pause/end path restores the
system with mask 0x1f. Full `g_slicks_profiles` dumps from before editing and
second preparation compare byte-for-byte equal. Existing original Change
Cars semantics and Amiga raw-key mapping regressions also pass.

This live case checks two active drivers and repeated acceptance. Packed
inactive/mixed-role rows are covered by the earlier component gate, not
claimed as additional live permutations here. Championship saving remains
distinct from CFG/PLR persistence and is still unimplemented.

Intermission parent failure handling now offers Enter to retry or Escape to
end the match using the already-resident HUD font and emergency save-under.
Failed attempt resources are released before showing the notice. Retrying
stays inside `run_intermission`, so it cannot repay rewards, reroll profile
selection or advance the playlist. A dedicated late post-paint fault mode
(`OPTIONSTJ`, `SLICKS_INTERMISSION_LIVE=2`) is available for target verification.
That gate now passes on A1200 debugger port 25121: the injected post-paint
failure reaches the retry notice, Enter opens the real intermission, and
Next Track prepares BASICTRK. Full session dumps at the notice and successful
reopen compare equal, including selections, inventory, cash, points and RNG.
The shared two-race gate checks exactly one reward per track, exactly one
selection refresh, and final system restoration. This verifies retry after
late parent failure; Escape from the notice and nested child failure/retry
remain separate live cases to cover.

### 2026-09-24: connected original intermission between tracks

`run_intermission` now occupies the original post-selection-refresh boundary
before next-track preparation. It loads real DAT/track data and original
fonts/icons with the OS available, draws the next track's preview, and resolves
the four original action keys against lang1.txt. The original keys at DS:07c2
and CARS title at DS:0c3d are exported as typed data rather than recreated
English labels. The header keeps the completed-track index, while name/preview
come from the following playlist entry, matching 2442d..24444 and 244ff..24521.

Native raw-key polling now dispatches F2 into Change Cars, closes it with the
original acceptance rules, and refreshes the intermission icons from current
session vehicles. Profile refresh is not repeated after these edits. Resource
opens/closes happen only after restoring AmigaOS; painting uses the existing
dirty intervals and view zero. Next Track proceeds to the same-game prepare
path; End Match returns to the title. The archive remains open until all
children are released. Renderer allocations and preview scratch are bounded.

Championship Save Game is explicitly **not implemented**: its action displays
a dismissible notice, not a false success or a CFG/PLR save. Integrated parent
open failure currently shows a resident warning and ends the match; preserving
the completed state for an explicit retry remains needed. Live edited-car
handoff and nested open-failure recovery still need target coverage.

The target build and original dispatcher/drawing/preparation/Change Cars
regressions pass. `diag_pause_transitions.gdb` now requires actual intermission
entry rather than accepting an invisible skip past the new screen. Two early
runs mistakenly selected OPTIONSP (pause/save) instead of OPTIONST (skip/end)
and correctly failed the expected-row assertion; assertions were not relaxed.
The corrected muted A1200 run on port 25119 passes: BASIC runs 100 updates,
F9/Enter skips through the original pause menu, the intermission shows
BASICTRK.SS with NEXT TRACK / END MATCH, and F9 starts the second track.
The second race runs 100 updates before F10/Enter ends the match. Each race
is awarded exactly once, selection refresh consumes the expected RNG draws
exactly once, both race preparations preserve the same-game boundary, and
system restoration reaches 0x1f. This run does not edit cars in intermission.

### 2026-09-24: native intermission resource and Change Cars lifecycle

The Amiga intermission owner now copies bounded caller strings, owns its
button/car snapshots and loads a real track preview using temporary 64 KiB
scratch. The dedicated surface uses original fonts, ten car icons and the
clock marker. Closing or failure after painting restores the full parent
chunky page and font; destroying the owner closes a Change Cars child first.

`SLICKS_INTERMISSION_SURFACE=1 amiga/debug.sh '' diag_intermission_surface.gdb`
selects the isolated `UIMENU2` component diagnostic. On a 68020 A1200 with
2 MiB Chip RAM and no Fast RAM it reaches all 17 checkpoints: three parent
allocation/scratch/post-paint failures, two complete opens, six child
allocation/icon/post-paint failures, four repeated vehicle edits, and two
full closes. All failed opens compare every parent pixel and font colour;
close compares all 64,000 pixels against the retained parent. Mixed human,
inactive and computer slots verify packed dialog rows: editing the second
visible row changes slot two, not inactive slot one. Escape and Enter both
retain current vehicles, as in the original, while profile selectors and
the fixed-profile RNG remain unchanged. F9/F10 return the distinct original
next/end results. System restoration reaches mask 0x1f. Debug host audio is
muted without altering emulated DMA.

The rerun on debugger port 25116 also captures open, edited and restored
chunky/bitplane pairs. `tools/verify_amiga_frame.py` confirms all 64,000 pixels
match in each pair. Initial presentation explicitly converts the full view:
view zero previously held the title, so dirty modal intervals alone cannot
initialize the surrounding race background. Subsequent edits use dirty ranges.

This test uses private driver data and explicit label fixtures. It does not
exercise localized intermission labels, normal race-loop entry, championship
Save Game, or carrying the changed vehicles into a second prepared race.
Those remain required integration work. The host renderer lifecycle and
original dispatcher regressions also pass. The native build passes the
strlen guard, link and executable conversion.

### 2026-09-24: original intermission dispatcher

Recovered the indirect switch missing from the normalized live listing using
the captured runtime bytes. `src/ui/intermission_menu.h` translates the input
dispatch at physical 2470e..2479f. `make verify-intermission-menu` executes the
original instructions with Unicorn and compares all 256 input bytes, four
selected rows and three incoming redraw states (3,072 cases). Only the two
modal callees are intercepted: the test observes Change Cars versus Save Game
and returns to the original caller. Their internals are not covered here.

Original initialization at 24527 selects row 2 (NEXT TRACK), with both redraw
flags set. At 245bb the game computes 1 minus the Change Cars option, then
immediately overwrites that navigation bound with 2. Execution for all 65,536
option words confirms this in the supplied binary. Do not implement a
different bound merely from the option label. F2 explicitly selects row 0
and invokes Change Cars despite the arrow-navigation bound. Enter, Ctrl and
Space accept the current row; Escape/F9 request Next Track and F10 requests
End Match. Save Game is a separate row-1 action; this gate does not establish
that ordinary navigation can reach that row from the initial state.

The translated dispatcher is not wired into the Amiga race loop yet. The
original screen rendering, vehicle-change dialog, saved-game action and
end-match path still need implementation and live validation. No claim of
full intermission completion follows from this isolated gate.

### 2026-09-24: Change Cars dialog state and keyboard translation

`src/ui/change_cars_dialog.h` now translates the original active-row mapping
and keyboard dispatch from 24997..24af0. Unlike profile editing, this dialog
changes the session vehicle bytes DS:4bc2 directly: Escape, Enter, F9 and
F10 close without rollback. Space/Right advance and wrap, Left decrements
and wraps, and Up/Down navigate the packed active slots. Computer and human
slots are both included, in slot order rather than starting-grid order.
These changes must not overwrite the saved profile vehicle selector.

`make verify-change-cars-dialog` passes 32 row mappings across every nonempty
participation mask, and 81,920 original-instruction dispatch comparisons
(all 256 scan bytes, all ten vehicles and each valid active row). The mapping
test executes the original drawing loop while intercepting only its three
rendering calls; it does not claim pixel fidelity. The keyboard comparisons
execute the original indirect jump table and verify all four vehicle bytes,
the selected row and the close flag. The dialog is not connected to the
Amiga UI yet. Its opening random-vehicle refresh, save-under/font/palette
handling, original rendering and live modal integration remain to be ported.

### 2026-09-24: Change Cars opening and row draw commands

The opening loop at 2480d..24859 now has a translation and 62,208 original
execution comparisons: all 81 human/computer/inactive role combinations,
all 256 profile-selector bytes (offset across the four slots), and incoming
show flags -1/0/1. The test executes the real weighted chooser and Borland
RNG, checking final RNG state, all current vehicles, visibility and balanced
stack. A permuted slot-to-profile mapping prevents confusing slot indices
with profile indices. Only an active profile with signed selector exactly
vehicle_count+2 is rerolled here. Ordinary random-per-race selection uses a
different rule elsewhere. This opening side effect precedes allocation;
the future platform owner must preserve that ordering on allocation failure.

The same gate now compares the entire row-drawing command sequence for all
32 valid selections across nonempty active masks: saved-background crops,
highlight position/size/RGB, vehicle icon coordinates and call ordering.
`slicks_change_cars_draw` emits those original operations through callbacks.
This is command fidelity, not a full pixel or live-Amiga check. The opening
frame, font/palette state, owned save-under buffers and intermission platform
integration still remain. Neither new helper is wired into production yet.

### 2026-09-24: compact Change Cars renderer

`change_cars_renderer.h` provides the original 40x100 save-under lifecycle,
decorated snapshot, 15/15/50 at 60-percent tint, 40/40/60 font colour, title
and existing translated row commands. Both 4,000-byte buffers are supplied
by the owner; no full-screen snapshot or allocation is needed in takeover.
Close restores the original region and font colour. Title-draw failure rolls
those changes back before returning an error. Invalid storage or unavailable
vehicle icons are rejected before painting the relevant operation.

`make verify-change-cars-renderer` passes 45 lifecycle cases: each nonempty
participation mask at the actual dialog origin and two screen-edge origins,
multiple row/vehicle edits, exact whole-screen restoration, font restoration,
duplicate close rejection, reopen with simulated text failure and short
storage rejection. Text/icon callbacks are stubs in this lifecycle test;
it does not prove original full-pixel fidelity. The original state/RNG and
draw-command differential tests still pass. The platform owner, full-pixel
comparison and live intermission connection remain open.

### 2026-09-24: Change Cars original full-screen pixel gate

`make verify-change-cars-pixels` now compares the entire 320x200 surface
and font colour against original DOS instructions at opening, every active
row redraw, and closing. Eight cases cover one through four participants at
x=210 and x=211, y=71; sparse slot masks, both human/computer roles, all ten
vehicle assets, sequential highlights and exact restoration are included.
All eight open/close pairs and 20 redraws pass. The test uses decoded original
`players.bmp` as the underlying asset, not a captured frame; original
`kirj.@f` and vehicle resources are rendered by the real DOS routines on one
side and native 68020 font/icon code on the other. Only allocation/free are
intercepted in the tested stages. Every original stage must reach its exact
stop boundary. VGA source/destination pages are aliased to the one visible
comparison surface; this does not test actual display timing/page swapping.

This closes the earlier renderer pixel-fidelity gap for these cases, but
does not connect the dialog to the live Amiga intermission screen. Platform
ownership, input orchestration and race handoff remain next.

### 2026-09-24: Amiga Change Cars resource owner

`slicks_amiga_change_cars_open/key/close` now supplies OS-side ownership:
the ten original car icons are loaded/decoded against the caller's current
palette, with 8,000 bytes of compact save-under storage in the modal owner.
It borrows the parent font/painter/dirty-interval path, but owns its icon
table without replacing the parent's. Keyboard handling performs no OS calls;
close restores the font and underlying screen before releasing storage.
Parent destruction also closes this modal. No saved profile or configuration
field is changed by the key path; it holds a pointer to current session
vehicles. The original opening selection/RNG work precedes allocation.

Three one-shot diagnostic fault boundaries exist for allocation, final-icon
load and post-draw failure. Their live target runs are still pending. Opening
failure after painting restores the surface; pre-paint resource failure frees
the owner without touching the screen. Empty/mismatched participation and
conflicting active modal owners are rejected. The API documents open=1,
original show-flag skip=0 and failure=-1; key returns 1 when ready to close.

The A1200 build succeeds with the runtime strlen guard and elf2hunk conversion.
These new entry points are not yet called by the race loop and may be removed
by link-time section collection; successful compilation is not a live modal
test. The original intermission surface, call site and race handoff remain
unfinished, rather than inserting this dialog on an unrelated menu screen.

### 2026-09-24: original intermission driver panel and call site

The original caller at 25937 refreshes profile selections, checks whether
another track remains, then calls 241e2 at 25955. This is the correct boundary
for the new intermission, not the current in-race FINISH overlay. The native
race loop currently refreshes selections and immediately loads the next track;
it still needs this intervening screen and its Next Track/End Match result.

`intermission_draw.h` translates driver rows 242ca..243c9. Active drivers
remain in slot order, with palette index 4+5*slot, points right-aligned at
x=249, names at x=105, and y=77+10*visible_row. Every driver's full 32-bit lap
equal to the supplied best-lap value receives the marker at x=253. No sorting,
human-only filtering, unique-winner selection or reward mutation is introduced.

`make verify-intermission-draw` passes 1,296 original command comparisons:
all 81 role combinations times 16 tied-marker masks, varied signed point
words and extreme signed lap values, permuted selected-profile indices,
exact names/flags/colours/coordinates/order, and balanced original stack.
This is a drawing-command gate, not yet a rendered standings screen. The
backdrop tint, track preview/captions, action rows, live orchestration and
end-match/save-game handling still remain.

### 2026-09-24: intermission action rows and car refresh

`intermission_draw.h` now includes original action redraw 245da..24702
and current-car redraw 2453a..245bb. The former restores the 72x42 rounded
save-under at (125,77+10*participant_count), then draws only rows 2/3,
with original centred labels and rounded highlights. The latter restores
the 8x40 strip at (95,77) and redraws current vehicle icons in active-slot
order. Both preserve zero-redraw behavior and clear their own redraw flag;
neither modifies race vehicles, profiles, points or random state.

The original drawing gate now passes 96 action-row cases (four participant
counts, all four selected rows, three redraw states, present/absent saved
background) and 486 car-row cases (all 81 role combinations, three redraw
states, present/absent background, varied vehicles). Exact drawing commands,
labels, flags and stack balance are compared. Action labels in this gate use
the original language wrapper's fallback path; live translated labels must
still be supplied by the owner through the existing verified language lookup.
The 1,296 driver-row cases continue to pass. This completes these drawing
command translations, not the full intermission renderer or live integration.

### 2026-09-24: intermission backdrop and track heading

`intermission_prepare.h` translates the original tint table and two backdrop
rectangles: RGB 30/30/55 at 82 percent, main panel (80,60)..(275,120+10*count),
preview surround (20,30)..(95,85). The table comes from the retained race
palette DS:68ae, not implicitly from the current display palette DS:71b8.
The two paints remain separate so composition can preserve original order.
`make verify-intermission-prepare` passes 20 full-screen/tint-table/dirty-bounds
comparisons against original instructions, with distinct source/display
palettes and participant counts zero through four.

The heading command translation covers original 243e9..244fb: track name,
slash and current/total numbers, font colours, alignment and coordinates.
The draw gate passes 36 index/total combinations including signed-word wrap.
Catalogue-name resolution and mode-dependent total are mocked boundaries in
this isolated test; the native owner must supply their actual results.
The existing driver/action/car command gates remain passing. The preview's
track geometry, complete rendered composition and live integration are still
open; these tests are not evidence that intermission is running in FS-UAE.

### 2026-09-24: composed intermission renderer

`intermission_renderer.h` now composes the verified panel tint, original
shadow colour, compact button/car snapshots, driver names/points/lap markers,
preview surround, track heading, asset-preview callback at (25,35), and
initial current-car/action redraw in original order. It uses the existing
records text interface for shadow flags, not the menu text adapter that
rejects shadows. Icon IDs 1..10 retain the records vehicle convention; the
owner supplies a distinct fastest-lap icon ID. Button/car snapshots consume
3,024 and 320 bytes respectively. The owner remains responsible for loading
resources and restoring its parent page if preparation fails.

`make verify-intermission-renderer` passes 30 composition cases covering all
nonempty participation masks, successful opening and preview failure,
repeat redraw with no pixel changes, Next/End selection and F2 refresh.
These lifecycle callbacks use stub text/icons/preview and therefore do not
prove full-screen original pixel fidelity. Existing component differential
gates are separate evidence. Full composition pixel comparison and the live
Amiga intermission owner/call site remain unfinished.

### 2026-09-24: composed intermission UI pixel comparison

The real-font/car-icon harness now executes original 241e2 through the first
input boundary and compares the composed renderer across every nonempty
participation mask. Initial rendering plus all four selected-row redraws
produce 75 matching full-screen/font comparisons. Names, signed point values
(including -32768 and -1), tied lap indicators, tint/snapshot order, shadows,
vehicle icons and action rows are exercised with DOS and native 68020 font
and icon painters. Exact original execution stop addresses are asserted.
`make verify-intermission-pixels` runs this shared harness along with the
existing Change Cars full-screen regressions, which also remain passing.

This test deliberately intercepts track preview at its verified (25,35)
boundary on both sides; it does not compare composed track geometry. Name
lookup and track-total resolution are also boundary inputs. The fastest-lap
sprite uses the same real car-icon fixture on both sides, so placement and
drawing are tested but its real resource identity is not. The original loads
that sprite sequentially through the call at 19fec, storing DS:4c30, rather than using the existing
top10cc icon by name; trace that data stream before production integration.
Do not treat this fixture as the game's correct fastest-lap graphic.
The live owner, exact marker resource, preview composition and race-loop
connection remain unfinished.

### 2026-09-24: fastest-lap marker identified and loaded

The startup sequence beginning with `/PartII` maps directly onto archive
entries 8..27: palette, main-menu image, cup palette, two selection images,
three player-status images, cup image, car-info palette, three info images,
three controller images, lower-menu image, joystick button/stick images,
then `clock.@16`. The final RGB-icon load at 19fec stores DS:4c30, used by
the intermission fastest-lap painter. The original archive entry is at byte
159080, length 106; its decoded image is 7x7. This replaces the temporary
car-icon marker fixture from the previous composition test.

All 75 intermission full-screen/font comparisons pass with the real clock
asset on both DOS and native sides. Existing Change Cars pixel gates also
pass. Preview geometry remains a separate boundary in this composition test.
`slicks_amiga_intermission_surface_create` now loads a dedicated font/car/clock
surface without modifying the caller's pixels; marker slot zero is independent
of Players' computer icon and Records' top10cc icon. The shared car loader
preserves existing Records fault hooks; intermission does not consume them.
The target rebuild succeeds. This surface still needs the intermission owner
and race-loop call site before it can appear during a normal game.

Shortcut-only vehicle persistence now has its own native save/restart/race
regression; see the final section. This closes a case previously masked by
other menu edits setting the save flag.

Current name-entry gate: original-DOS full-pixel comparisons pass, and
`out/SlicksNameDialog2` passes native create/rename/reopen/cancel on 2 MiB
A1200. `out/SlicksColourDialog` additionally passes both-endpoint editing,
accept/cancel and reopening. These do not yet prove persistence or complete
native setup.

- Original-DOS comparisons for translated semantics and rendered dialogs.
- Muted-host FS-UAE A1200 with 68020, 2 MiB chip and no fast RAM.
- Ordinary menu-input sequence creates and edits profiles, sets all relevant
  options, saves and exits. A fresh process reloads the files and reproduces
  configuration and menu state without injected runtime state.
- Race entry preserves selected drivers, participation, vehicles and colours;
  displayed bitplanes match the authoritative chunky surface.
- Failure tests preserve recoverable prior files and show a usable error path.
- Clean exit restores the machine; normal run.sh audio remains enabled.

The broader ten-item native-port queue remains tracked in open-work.md;
this document scopes the currently active player-setup goal. Component tests
alone do not satisfy the save/restart/race completion check.

## Persistence boundary

The Amiga CFG tag is explicitly `0xa1` instead of a DOS BIOS-date-derived
signature. The original version-15 142-byte format and field encoding remain
unchanged. This byte is not authentication. A foreign tag is rejected, not
silently accepted or overwritten; explicit DOS configuration import is not
implemented. PLR retains its original signature and variable record layout.

The loader requires exact encoded lengths (stricter than the original's
unchecked reads). Missing files use original default-reader behavior; empty,
truncated, oversized, foreign and unreadable files do not become defaults
silently. `.new`/`.bak` leftovers stop startup with a recovery message rather
than loading a potentially mixed-generation pair. The transaction handles
reported I/O failures but does not claim power-loss atomicity across two files.

`make verify-setup-load` exercises the actual Amiga adapter with a mocked DOS
boundary: 254 checks cover short reads, each I/O/allocation failure, both-file
atomic publication, truncated/extra bytes, foreign tags, recovery artifacts
and all missing-file combinations. These are host tests, not target restart
proof. The calendar mapping follows original `2c03e`: day, one-based month,
full year, and day zero for dates before 1997. Amiga utility.library supplies
calendar conversion from DateStamp.

`out/SlicksSetupLoad` passed the existing startup-to-race handoff gate in
isolated `.run/setup-load-v1`, with no saved files: vehicles `0,0,5,0`, all
52 inventory slots and four steering values matched expected state on the
2 MiB A1200. This does not yet exercise a saved pair on the target. Original
save caller located at `26691`, near normal program exit, calling `2b097`;
the original routine tests and clears DS:06c0 dirty before writing. Native
failure handling must keep it retryable instead of clearing on a failed save.

## Native save/restart gate

`out/SlicksSetupSave2`, `SLICKS_PLAYER_MENU=9`, and `diag_setup_save.gdb`
drive ordinary menu input to create ABC, edit/accept its first colour endpoint,
cancel a second-endpoint edit, select ABC for driver one and exit. In isolated
`.run/setup-save-v1`, the target wrote a 142-byte CFG and 61-byte PLR and
restored system state (`0x1f`). This is diagnostic mode PLAYERSP; other diagnostic
modes still cannot implicitly save. The normal no-argument run saves dirty
setup on exit. Dirty state survives reopening the player menu and failed saves.

A separate FS-UAE process on the same isolated disk, using SETUP and
`diag_setup_reload.gdb`, loaded both files and started a race with ABC selected,
its saved vehicle and the correct participation/vehicle handoff for all four
drivers. All six colour bytes read back from target memory exactly matched
the saved PLR (`27,15,20,49,47,38`). This proves this profile sequence, not all
options or every shared/human/computer/inactive combination. No GDB writes to
game memory are used. The normal launch binary was not overwritten.

Save failures show a native platform-error screen (not an invented original
DOS dialog): Enter retries and Escape returns to the title, retaining edits.
A committed pair with failed backup cleanup gets a separate warning. Debug
forced-exit remains an explicit unsaved escape hatch; production right-mouse
exit now uses the same save path as the title Exit/Escape action.

`out/SlicksSetupRetry2`, `SLICKS_PLAYER_MENU=10` (PLAYERST), and
`diag_setup_retry.gdb` passed a live AmigaDOS failure/retry gate in isolated
`.run/setup-retry-v2`: a test-owned empty directory blocks CFG staging,
the save returns wrong-object-type error 212, the native error screen appears
with ABC and its selection intact, AmigaDOS removes the test obstruction,
and ordinary Return input retries successfully. Both files commit and system
restoration reports `0x1f`. The test-only directory is removed, not user data.
An earlier host-side mkdir/rmdir variant showed FS-UAE stale-directory behavior
and is superseded by this entirely target-side fault fixture. Escape-to-return,
unwritable volume and backup-recovery warning paths still need live gates.

## Original options inventory and input gate

The temporary two-setting service menu is not the original Options screen.
Original `28e19..296xx` has eighteen rows, with mode-dependent visibility and
navigation. The six modes are Classic, Weapons, Tuning, Tune+Weapons, Custom
and Arcade. Fifteen persisted settings, in original order, are play mode,
sound volume, background sound volume, laps, starting money, money per track,
money per position, weapons, tuning, fuel, damage, collisions, change car,
race time and track count. The final rows are Controllers, Clear Top 10s and
Exit. Their labels, suffixes, mode names and numeric specifications now export
from the initialized executable data into the ignored generated header.

`src/ui/options_menu.h` ports all fifteen numeric edits and the full original
navigation/dispatch state (including redraw markers, dirty flag and skipping
mode-disabled rows). `make verify-options-menu` passes 139,320 signed-value
comparisons and 1,350 original full-dispatch comparisons across all six modes.
This is not yet connected to production: the original renderer must replace
the service placeholder, and laps/mode/audio settings must reach their correct
consumers rather than the temporary title fields.

Dependencies identified directly from original calls:

- Controllers: `2d8ac`, called at position `(100,80)` by option row 15.
- Clear Top 10s: `1a96b`; includes a confirmation prompt. Do not implement
  it as unconditional deletion. The original also clears its in-memory record
  buffer before asking; distinguish that from persistent file effects.
- Help: common `327dc`, with the `options` topic supplied by F1. Player-menu
  F1 still needs the same viewer connected with its own original topic.

Rows 15..17 are actions, not valid option-table entries. The native key helper
does not reproduce original out-of-bounds option-table writes reachable with
arrow keys on those rows. Action callbacks still require implementation and
their own original-code/live gates; the input test intentionally excludes
those unimplemented callbacks rather than treating them as passing screens.

`options_menu_draw.h` now ports the original redraw loop `28f9d..293ef`.
`make verify-options-draw` passes 648 complete command/text/order comparisons
covering all eighteen selections in all six modes, minimum/maximum values,
full redraw, values-only redraw, idle, selected-row and preceding-row redraw.
The oracle runs original arithmetic and text-formatting code; only graphics
boundaries are recorded. This checks drawing requests, not actual pixels.

`options_menu_renderer.h` connects those requests to the existing saved-menu
restoration, native font callback, bevel and indexed-palette remapping
primitives. Its bar tint is the original `(30,10,30,75)` table. Native pixel
comparison, original screen preparation and production/target integration
remain required. No replacement screen is claimed from this command gate.

The renderer pixel gate is now green: `make verify-list-pixels` adds 324
sequential full-frame and font-state comparisons across all eighteen rows,
six modes and default/minimum/maximum values. Original DOS executes its real
drawing primitives against modeled VGA; the native path uses the real 68020
font renderer. Frames are retained between updates to test saved-background
restoration, not only fresh full draws.

Static preparation `28e7f..28f43` also matches all visible pixels and the
small-font state: original panel tint `(10,10,50,66)`, original `settings`
heading at `(75,25)`, and the resulting saved-background snapshot. The DOS
panel extends to row 220; the native surface correctly clips to its visible
200 lines. These tests use an asset-decoded input surface to isolate overlay
semantics; live title-to-Options composition is not yet claimed. Production
menu ownership, entry/return lifecycle and A1200 verification remain next.

## Live original Options screen

Normal interactive play now opens the original Options screen instead of the
temporary fuel/damage menu. The Amiga adapter reuses menu-surface ownership,
decoded original fonts, dirty bounds and the verified renderer. It loads no
player bitmap or car icons for Options. Exiting releases the surface, refreshes
profile participation for the chosen mode, and uses the persisted lap setting
for race entry. The old service menu remains only for its dedicated diagnostic
CLI modes. Legacy title car/lap arrow shortcuts are disabled in native setup
so they cannot alter non-persisted shadow selections.

`out/SlicksOptions2` passed `SLICKS_OPTIONS_MENU=1` / `diag_options.gdb` in
`.run/options-v1` on a muted 2 MiB A1200: ordinary input opened Options, selected
Custom mode, changed laps from five to six, returned, reopened with both values
intact, then returned and entered a six-lap race. All 64,000 pixels dumped from
the reopened screen's Amiga bitplanes matched its authoritative chunky surface.
The later `out/SlicksOptions3` build additionally disables the conflicting title
shortcuts; the default launch binary remains untouched.

Options is not yet complete: Controllers, Clear Top 10s and F1 help are explicit
pending actions. Sound/background-volume application and Arcade-specific
consumer semantics still need wiring/audit. The new Options edit sequence also
needs inclusion in the save/restart gate rather than assuming the earlier
profile-only persistence test proves every setting.

## Controllers translation

`controllers_dialog.h` ports input dispatch `2dd91..2df4b`, the last-row
column limit, and original accepted-key lookup `1e1e1`. The dialog has four
driver rows, each with device selection and five key bindings, plus a final
Defaults/Exit row. Return/Control advance the device, Space reverses it.
Keyboard capture validates against the original 69-entry signed-byte table;
unsupported keys leave the previous binding intact. Defaults replaces only
the twenty keys, not device choices. Edits are immediate, so Escape exits
without rolling them back. The caller's Options action owns the dirty flag.

`make verify-controllers` passes 2,640 complete navigation/device/default/exit
comparisons and 10,240 captured-key comparisons against the original routines.
Capture tests execute the original key validator, not an assumed PC key list.
Original key names and supported scan bytes now export as typed local data.

`controllers_dialog_draw.h` ports `2da76..2dd89` with exact row restoration,
selection boxes, device icons/numbers, greyed binding text and key-name lookup.
`make verify-controllers-draw` passes 120 command/text/order/column-limit cases.
This does not yet prove pixels or a connected dialog: native icon loading,
static preparation, capture-key platform mapping, Options entry/return and
target verification remain. Device choices must also be audited through the
live input path; storing `player_input[]` alone is not working hardware input.

### Controllers assets and native rendering

The three device images are archive resources `ohj_key.@I`, `ohj_joy.@I`
and `ohj_lptc.@I`; five action headings are `keys_m1.@16` through
`keys_m5.@16`. No extracted assets or captured screen data are needed.
`slicks_decode_indexed_menu_icon` handles the legacy indexed/RLE format.
The keyboard resource ends with an overlong zero run (222 decoded bytes for
221 logical pixels); native decoding accepts this while bounding writes to
the image. The original `3aa7c` painter skips colour zero: these icons are
transparent, not opaque.

`make verify-indexed-menu-icon` compares all three resources and four format
variants with the original DOS loader, including plane padding and dimensions.
Twenty-eight full-frame draws cover every destination alignment and verify
transparency. Truncated resources leave outputs untouched. The RGB icon test
now also covers all five heading icons: 144 original-versus-68020 full-frame
comparisons across sixteen resources pass.

`controllers_dialog_renderer.h` supplies compact outer/tinted snapshots,
original tint/heading/number preparation, redraw, capture prompt and restoration
of the underlying screen and font colours. `make verify-list-pixels` adds 120
sequential Controllers redraw comparisons with real DOS drawing routines and
the native 68020 fonts. Native close restores all pixels and both fonts.
These comparisons share the prepared input snapshot: they do **not** yet
independently prove static preparation or the capture prompt. The existing
name, colour, Options and list pixel gates remain green.

The Amiga adapter loads the eight resources before takeover, owns approximately
35 KiB of modal storage and releases it on close or menu destruction. It builds
successfully in `out/SlicksControllers1`; unused modal entry points are not yet
connected to Options and may be removed by linker section collection. The normal
binary was not replaced and no FS-UAE run was performed for this build.
Next: independently verify preparation/capture, connect Options entry/return and
scan-code capture, then test Defaults, repeated entry and controller persistence
on the A1200. Actual device routing remains a separate required check.

### Live Controllers entry and repeated editing

Options now opens the Controllers modal, presents the original capture prompt,
accepts supported scan codes and restores the Options surface on close. Device
and key edits retain the original immediate-commit semantics; the Options action
sets the persistent dirty flag. Resource loading and modal allocation/free happen
with AmigaOS available, not during hardware takeover. Captions come from the
original initialized data via the exporter.

The first connected build (`SlicksControllers3`) exited with return code 20:
the Options surface did not yet initialize its icon painter. The modal's
precondition rejected it. Adding the existing chunky icon painter fixed this.
`SlicksControllers4` passed `SLICKS_OPTIONS_MENU=2` / `diag_controllers.gdb`
in `.run/controllers-v2`, port 24989, on the muted 2 MiB A1200. Ordinary queued
input selected Custom mode, opened Controllers, changed device 0 to joystick 1,
bound Q, closed/reopened, restored all twenty default keys without changing the
device, closed/reopened again, bound Y and closed. Both capture prompts occurred;
all three closes retained edits, freed the modal and resumed active display.
The result was `CONTROLLERS_ENTRY_CAPTURE_DEFAULTS_REOPEN_REEDIT_OK`.

Original input gates were rerun: 2,640 dispatch and 10,240 capture cases pass;
120 drawing-command comparisons pass. The normal binary remains unchanged.
Remaining controller work: complete the platform key mapping beyond the current
navigation/default-key subset, independently check static preparation/capture
pixels, exercise save/restart with these edits and connect/audit actual device
routing. The successful dialog test does not claim working joystick selection.

### Complete classic keyboard scan mapping

`amiga_key_scan.h` replaces the navigation/default-only switch with physical
Amiga-to-DOS scan identities shared by menus, binding capture and race input.
All 68 nonzero scans accepted by the original dialog are reachable. Name entry
continues to use the installed Amiga keymap independently. Physical positions
and modifier codes were checked against the classic tables in the
[AmigaOS Keymap Library documentation](https://wiki.amigaos.net/wiki/Keymap_Library).
Amiga-only/unassigned keys remain unmapped; keypad/cursor aliases retain the
original single-byte scan model.

`make verify-amiga-key-scan` passes 1,360 combinations of original accepted
scan, driver group and action: capture, exact CFG roundtrip (keys and devices),
then press/release through the human-first driver ordering. This is a host
integration gate, not evidence of a disk restart or actual joystick routing.
`SlicksControllers6` passed the muted A1200 diagnostic in `.run/controllers-v3`
on port 24990: capture newly mapped W, attempt rejected F3 without losing W,
Defaults, reopening and rebinding Y. Three capture prompts and three clean
modal closes were verified. No normal-binary replacement was made.

### Original device polling semantics

Recovered `199cf..19d03` into `driver_device.h`, excluding the platform sample
boundary `36a05`. Unlike the keyboard callback's human-first key groups,
`player_input[]` is indexed directly by driver here. Only human drivers poll
devices. Analogue devices 1/2 decrement the signed countdown and sample when
due; digital/LPT devices 3..6 sample every update. The keyboard's fifth action
is preserved. Both original throttle schemes, weapon-related button/axis
edges and the four rising-edge flags are retained, including the original
previous-axis state updates.

`make verify-driver-device` passes 8,064 comparisons against execution of the
original routine: four drivers, seven device values (including keyboard),
three participation states and 96 combinations of axes/buttons/prior input,
throttle scheme, weapons and countdown. Only physical sampling is stubbed;
the original performs all control, timer and edge computation. This is not
yet connected to the Amiga game-port registers or production race loop.
Next work must supply hardware samples and preserve the direct-driver mapping,
rather than reusing the keyboard group ordering by assumption.

### Amiga device adapter and race-loop connection

`amiga_joystick.h` decodes JOYDAT's two quadrature XORs and active-low CIA/POT
buttons, following the Amiga Hardware Reference Manual's
[digital joystick table](https://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0183.html).
Native joystick 1 maps to JOY1 (the usual joystick socket), joystick 2 to JOY0
(the mouse socket). Sampling is read-only; no custom-register or CIA writes
were added. The original's throttle scheme and three-tick polling interval are
preserved; the interval comes from initialized DOS data. Imported PC/LPT device
values above two are rejected for human drivers at race preparation rather
than silently mapped to a game port.

The production race step calls the input adapter with its actual physics ticks
immediately before car updates, after the starting-light countdown. State resets
on each prepared race. Direct driver device assignments are retained, while
keyboard callbacks continue to use human-first group ordering. Joystick 2's
second button no longer triggers the global right-mouse exit shortcut during
the race. The recovered rising-edge state is maintained, but consumers beyond
the existing racing controls still require the broader weapon/input audit.

`make verify-amiga-joystick` passes 256 combinations of both ports, direction
switches, buttons and unrelated register bits. The 8,064 original device cases
remain green. `out/SlicksControllers7` built and passed `diag_device_poll.gdb`
via the ordinary Options-to-race sequence in `.run/device-poll-v1`, port 24991,
muted 2 MiB A1200: ten racing updates reached the attached callback with valid
configuration and nonzero physics ticks. This proves lifecycle integration,
not pressed physical joystick input; an actual/emulated joystick-driving gate
is still required. The normal launch binary remains untouched.

### Combined native configuration save/restart gate

`PLAYERSU` extends the existing original-menu profile sequence with ordinary
Options/Controllers input before exiting: create/select ABC, accept the first
colour edit and cancel the second, select Custom mode, change laps 5 to 6,
bind W for the first control and save. `SlicksSetupCombined1` passed
`diag_setup_combined_save.gdb` in `.run/setup-combined-v1` (port 24992), writing
the exact 142-byte CFG and 61-byte PLR, then restoring system state `0x1f`.

A fresh emulator process using `SlicksSetupCombined2` and `SETUPR` loaded the
same isolated volume (port 24993). Unlike the old SETUP diagnostic, SETUPR
enters via an ordinary Return on native GO rather than auto-preparing a race.
`diag_setup_combined_reload.gdb` verified the loaded ABC selection, Custom mode,
six laps, W binding, all four participation/vehicle handoffs and the selected
profile's six colour bytes. The saved and reloaded colour dumps compare equal.
The race entered with six laps and no reported race error.

During this integration audit, the device adapter's retained configuration
pointer was found to refer to a block-local configuration in older auto-race
diagnostic paths. It now owns a persistent copy made during race preparation;
the interactive path and fresh reload both use that stable storage.

This is a combined completion-scenario gate, not full goal completion. Remaining
requirements include original Help and Clear Top 10s, Options audio/Arcade
consumers, exhaustive native role/shared-profile restart cases, physical or
emulated pressed-joystick driving, static controller preparation/capture pixel
checks and the outstanding live persistence-failure return/recovery paths.

### Original help resource access

The original help viewer is `327dc..32eee`, configured through `318bc` with
the normal kirj font and the embedded `HELP.TXT` name. Its text is the final
archive entry at byte 628903, extending to EOF 642007 (13,104 bytes). Both
native and host archive readers previously rejected this last entry because
they required a next directory record. They now use EOF for the final resource;
the Amiga adapter correctly accounts for Seek returning the *old* position.

`make verify-resource-archive` compiles the actual Amiga adapter against
FILE-backed DOS calls: 182 named-resource comparisons pass, including complete
HELP.TXT loading, too-small destination rejection and faults in each of the
two EOF-position seeks, the resource seek and final read. Prior duplicate-name
marker handling remains intact. No original help bytes were extracted into
tracked files.

Help is not yet displayed. The source contains original formatting commands,
links and pages, with setup targets `plr_menu`, `track_menu`, and `options`.
`32464` builds a compact topic index using `3231d`: zero records hold uppercase
anchor names, type 1 marks a chapter offset, type 2 a page offset, each offset
stored in three little-endian bytes. `32631` searches with the country prefix
first, then the unprefixed name; it also resolves page/link positions. The
main viewer saves the full screen, preserves font state and handles navigation
and history. Porting this original parser/viewer remains required; a plain
text help replacement is not an equivalent implementation.

### Original help index and lookup

`src/ui/help_index.h` now ports `32464/3231d` index construction and
`32631..327db` topic lookup. The builder reads the real archive text, keeps
source slices for the initial (up to 20) formatting lines, and emits the
original compact chapter/page/anchor records. Chapter offsets, record counts,
header counts, initial body position and maximum chapter allocation match the
DOS routine. Header formatting interpretation is still pending. A measuring
pass rejects insufficient output capacity without modifying the output buffer.

`make verify-help-builder` executes original DOS code with file/allocation
services supplied by the test harness. Eleven byte-exact cases pass: actual
HELP.TXT, chapter/page/form-feed markers, long and underscore anchors,
end markers, unfinished input and the 19/20/21-header boundary. Every undersized
output capacity is also rejected without modifying its destination.
`make verify-help-index` passes 28 independent original-code comparisons for
prefix fallback, case folding, page/anchor selection, duplicate topics and
missing topics. No original resource bytes are added to tracked assets.

These are parser gates, not a completed help viewer: formatting, drawing,
navigation/history and native menu integration remain open.

The reported return-code-20 failure has not yet been reproduced: both the
current `SlicksHelp1` and the older normal-launcher `SlicksDiag` reached their
title-ready breakpoints in separate muted 2 MiB A1200 runs (ports 24994/24995).
The triggering menu/action and whether this was a debug or normal launch are
still needed to identify that specific failure. Neither launcher binary was
replaced during that diagnosis.

### Help chapter loading and preprocessing

`src/ui/help_text.h` ports the original `32048..32164` chapter loader and
`31923..319ef` in-place text preprocessing. Chapters terminate on `<e`, `<E`,
`<!`, DOS EOF or stream EOF; page boundaries remain within the loaded chapter.
The preprocessing preserves CR, removes LF, changes command introducers and
page separators to the original internal control bytes, and reproduces the
original handling of doubled `<` and compacted-buffer tail bytes. It is not
replaced with a generic markup parser. Insufficient chapter storage and
unterminated preprocessing input are rejected before modifying the destination.

`make verify-help-builder` now compares full buffers against those DOS routines
as well as the index builder: 11 indexes, 52 chapter loads and 79 preprocessed
lines pass, including all indexed chapter positions in actual HELP.TXT and
synthetic EOF/marker cases. The original code executes under Unicorn with only
file/allocation services supplied; text transformations are not stubbed.
The check compares bytes beyond the visible NUL too, covering the original's
in-place compaction behavior. Headers are still retained as source slices and
will be preprocessed on entering the viewer.

No visible help dialog is connected yet. The next work is the original line
formatter/draw routine `31b53`, followed by page layout, navigation/history and
menu entry/return. These passing resource/parser gates alone do not satisfy
the remaining native Help requirement.

### Original help line renderer

`src/ui/help_line.h` ports `31b53..3201f`, its span renderer `31a78`, numeric
argument scanning and bounded topic copying. It retains the original command
dispatch, centering, window changes, palette colour selection, line/character
spacing, rules, next/previous topics, country prefix, link counters/types,
selected-link backgrounds and text runs. Drawing uses callbacks so the native
font and authoritative chunky painters can be connected without another CPU
emulation layer. Lines are bounded and copied locally before temporary text
termination, as required by the original span drawing behavior.

`make verify-help-line` compares original DOS execution with the native
semantic renderer for every physical HELP.TXT line, plus additional supported
commands. Eight combinations cover centered/non-centered text, selected links,
country matching and already-accumulated page links: 4,728 comparisons of
ordered draw calls, text bytes/positions, returned Y and persistent formatting
state pass. Only the font/rectangle/palette drawing services are intercepted;
the original parser, argument handling and span decisions execute unchanged.
Both sides use the same deterministic font metrics for this command-level gate.

This does not yet prove real-font pixels or complete viewer behavior. Still
required: native drawing adapter and pixel comparisons, page layout,
navigation/history, full-screen/font restoration, and native menu integration.

### Help native font and pixel gate

`help_renderer.h` connects the line formatter to the existing chunky painters
and runtime font. New `slicks_help_measure`/`slicks_help_text` assembly bridges
pass the help-controlled signed character spacing and return the original
advance. `slicks_amiga_help_renderer_init` supplies the Amiga callbacks and
reports drawn rows through the existing merged dirty-bound list. It does not
yet open a help modal or change any menu action.

`make verify-help-pixels` passes 1,166 full-screen and complete-font-state
comparisons for every physical HELP.TXT line with two spacing/centering cases.
The reference runs the original DOS font, rectangle and palette routines;
only VGA hardware is modeled. Native font drawing and measurement execute
the actual 68020 bridges and translated assembly, not host substitutes.

This exposed a real glyph edge bug: the line at HELP.TXT offset 3028 extends
into VGA's invisible right margin. On a 320-byte native row, the old glyph
painter instead wrapped those bytes into the next visible row. The glyph
routine now retains its on-screen drawing loop but takes a clipped edge path
when necessary, using mult320 for visible rows. No extra framebuffer is used.
Extended original-glyph comparisons cover partially/wholly invisible right
and bottom glyphs, register preservation and native buffer guards.

`make verify-font-glyph` passes 7,584 glyph pixel comparisons across all three
fonts, 4,240 measurement cases per font and all existing full-string cases.
`make verify-list-pixels` also passes all prior name, colour, options,
controllers and list-dialog checks. Isolated `out/SlicksHelp2` builds and links
successfully; the normal launcher binary is unchanged. No FS-UAE help-viewer
claim is made: page layout/navigation, modal restoration and menu hookup remain.

### Complete help pages and renderer lifetime

`slicks_help_renderer_page` now ports `32165..32301`: reset page-local link
counters, draw preprocessed headers, select pages by explicit form feeds,
read original CR-delimited lines, stop at the original bottom/page boundary,
report the next-page marker and clear the remaining window. It does not
introduce word wrapping or replace explicit pages with host reflow.

The real-font pixel verifier now traverses every indexed chapter and explicit
page in HELP.TXT with two selected-link positions. All 98 complete-page
comparisons match original DOS pixels, complete font state, selected target,
link counters and next-page marker. The original page, line and font code all
execute; only VGA hardware is modeled. Existing 1,166 line pixel cases,
4,728 command/state cases and index/chapter gates still pass.

The renderer also has allocation-free open/close operations using a caller-owned
64,000-byte snapshot, the original default window and first four palette
queries, and saved font colour. All 98 page cases close back to the exact
underlying screen and original font colour. This verifies renderer lifetime,
not yet the complete original viewer's input/history/resource lifetime.
Buffers must be allocated with AmigaOS available before hardware takeover.

Still required before native Help is usable: original keyboard dispatch and
history, chapter changes/missing topics, resource ownership, native menu
entry/return, and FS-UAE interaction checks. No Help menu action has been
redirected to an incomplete viewer.

### Help keyboard dispatch, history and arrows

`help_navigation.h` ports the original post-getch block `32b81..32e20`.
It handles Escape/F10, F1, PageUp/PageDown, link cycling, Enter/Space and
Backspace/lowercase b. The original oddity that ASCII uppercase K moves up,
but extended Left does not, is retained. History preserves the original 101
slots and signed-byte page/selection values, including push/pop shifts and
the invalid selection sentinel. Missing targets leave the current location
unchanged; inline-information and `$` targets retain the original no-navigation
behavior rather than adding invented actions.

`make verify-help-navigation` passes 816 comparisons against original DOS
dispatch and lookup code, including the complete history arrays. Cases cover
empty/full histories, page and link limits, prefix fallback, missing topics,
inline links and non-handled keys. This verifies the input state transition,
not yet the redraw/reload loop or Amiga key-event adapter.

The renderer now also draws the original page arrows (`32abc..32b5d`) with
the fifth default palette query. All 98 complete-page pixel comparisons now
include that original arrow block; full font state and restoration still match.
The 1,166 line-pixel and 4,728 line-command regressions remain green.

Remaining Help integration: redraw/chapter-load orchestration, owned resource
buffers, menu entry/return and live FS-UAE navigation/persistence checks.

### Native Help entry and runtime stack-overflow fix

`help_viewer.h` now owns chapter/index/header buffers and the saved underlying
screen. The native Players and Options F1 actions load the actual HELP.TXT
resource with the OS available, then run the renderer and navigation under
hardware takeover. Close restores the menu and releases the viewer with the
OS available. OPTIONSH/OPTIONSJ and `diag_help.gdb` exercise opening, cycling
links, following a link, history return, closing and reopening.

The first Players Help run reproduced Guru #80000008. A hardware watchpoint
on SysBase caught the corruption inside the support library's strlen:
GCC -O2 had converted its counting loop into a recursive call to strlen.
Hundreds of recursive frames crossed the 4 KiB task stack's lower boundary
and overwrote SysBase. This was not a display-buffer or Help-allocation fault.
Compile the support implementation with -fno-builtin; its strlen now has a
constant-stack leaf loop. Do not change the shared support source or hide
the problem by enlarging the stack.

The normal Amiga build now runs `tools/verify_amiga_runtime.py`, which rejects
calls/tail jumps in strlen. It rejects the failing SlicksHelp6 binary and
accepts corrected SlicksHelp7. Muted A1200/2 MiB FS-UAE runs for both Players
and Options pass HELP_NATIVE_MENU_NAVIGATION_REOPEN_RESTORE_OK (five Help
ready checkpoints, two closes, system restoration 0x1f). Both underlying
menu buffers match byte-for-byte before and after Help. The standard
`amiga/out/SlicksDiag.exe` has been rebuilt with the correction.

This does not complete all Help work: the viewer orchestration still needs
its own original-code differential gate, keypad page-navigation mapping
needs review, and remaining Help entry points/failure handling need auditing.

### Classic-keyboard Help page navigation

The Help-only Amiga input adapter now maps keypad 9/3 to DOS extended
PageUp/PageDown rather than consuming their keymapped digit characters.
Name entry and controller scan bindings are unchanged. Releases are ignored;
other keys supply either keymapped ASCII or an extended scan, never both.
The mapping gate covers 65,536 raw/ASCII combinations; all 1,360 existing
controller capture/save/load/driver-key cases and 816 original Help
navigation comparisons still pass.

`SLICKS_HELP_MENU=3` (OPTIONSK) and `diag_help_pages.gdb` drive ordinary
Amiga key events through Players Help. SlicksHelp9 on the muted A1200/2 MiB
passes HELP_NATIVE_PAGE_KEYS_CONTENTS_RESTORE_OK: chapter 1581 page 3,
keypad 9 to page 2, keypad 3 back to page 3, F1 to body chapter 348 page 0,
Escape to Players and then clean system restoration. The underlying menu
is byte-identical before and after Help. No debugger writes supply the
navigation state. The earlier page-key mapping review item is resolved;
the broader Help orchestration/entry-point/failure-handling audit remains.

### Title Help entry points

The native title's Read This entry and F1 now open the real resource-backed
Help viewer; keyboard activation and the existing title mouse activation
path share the same destination helper. A standalone Help surface loads
kirj.@f and uses the current chunky title/palette, without drawing an Options
or Players background. Escape restores the saved title, then frees the
surface/viewer with AmigaOS available; normal and error cleanup own all
allocations. Failure currently exits through normal cleanup, so user-facing
Help failure handling remains an open audit item.

`make verify-title-help` executes the original Read This wrapper at 2a096
and F1 call site at 2a3cc up to the viewer entry, comparing their actual
topic arguments ("reg" and empty respectively). The helper maps the native
title's existing compact ordering (Read This 4, Quit 5), not raw DOS row IDs.
`verify-help-index` now checks the real HELP.TXT index as well as synthetic
edge cases: 64 original-DOS lookup comparisons. With this supplied resource
and empty country prefix, "reg" resolves to chapter 353/page 0 (contents)
and the empty topic resolves to 9589/page 0 (registration). Do not replace
empty-topic resolution with the in-viewer F1 body action: they are distinct.

SlicksHelp10, muted A1200/2 MiB, passes TITLE_HELP_READ_F1_RESTORE_OK via
ordinary queued keys: select Read This, open/close, title F1 open/close, Quit.
Both closes match the initial title's 64,000 bytes exactly and hardware
restoration returns 0x1f. Rendered native screens were visually inspected.
The standard executable is rebuilt. This verifies the title keyboard path;
the mouse branch shares the destination/open path but lacks its own live
input gate. Remaining setup work is still not complete.

### Failed-save cancellation, subsequent save and fresh restart

PLAYERSV (`SLICKS_PLAYER_MENU=12`) now provides the missing live Escape
failure-path gate. In an isolated DH1 it creates ABC via native dialogs,
edits its colours, selects it, then deliberately obstructs SLICKS.CFG.new
with an AmigaDOS directory. The first save reports FAILED_RESTORED and
IoErr 212. After removing only that diagnostic obstruction, ordinary key
events press Escape, reopen Players, close it and request exit again.

`diag_setup_cancel.gdb` passes SETUP_CANCEL_OK RETURN_REOPEN_SAVE_RESTORED:
exactly two save attempts, retained ABC selection, a real menu reopen,
successful second save, and hardware restoration 0x1f. Complete profile and
setup-session memory snapshots match byte-for-byte before and after Escape.
The cancellation path now also releases a title Help surface and clears
the Options configuration reference alongside the existing modal cleanup.

SlicksSaveCancel1 was then restarted as SETUPR in the same isolated DH1.
`diag_setup_cancel_reload.gdb` passes SAVE_CANCEL_RELOAD_RACE_OK: both
persisted files load successfully, ABC remains selected, its vehicle and
six colour endpoints reach the race setup, and all four driver participation
and vehicle values match the race handoff. CFG is 142 bytes and PLR 61 bytes
(three-byte header plus one user profile). No normal-launcher save files
were touched. The standard executable has been rebuilt.

The 2,600 storage success/fault/rollback cases and 254 load-fault checks also
still pass. This closes the live failed-save Escape/return/re-save case;
it does not replace remaining unwritable-volume and recovery-leftover live
checks or the broader human/computer/inactive/shared-profile matrix.

### Mixed roles and shared Computer profile: native save/restart/race

PLAYERSW (`SLICKS_PLAYER_MENU=13`) extends the native Add/name/colour/selection
sequence: ABC remains the human in slot zero, slots one and three share the
built-in Computer profile, and slot two is set to None using the Players
menu. No debugger or fixture changes the configuration. The resulting profile
indices are `[3,1,0,1]`, participation `[-1,1,0,1]`, active count three.

SlicksMixed1 passes `diag_setup_mixed_save.gdb` in isolated
`.run/setup-mixed-v1`, then a fresh SETUPR process passes
`diag_setup_mixed_reload.gdb`. The race has the expected participation in
all four slots, each active vehicle matches the resolved setup, and human
control order starts with slot zero. Both shared Computer colour records
match their profile; ABC's six endpoints and fixed vehicle match its profile.
The saved and reloaded eight-byte ABC setup record compare byte-for-byte.
Built-in random vehicle choices are deliberately not asserted identical
across different startup seeds; original built-in profiles are not serialized.

This closes one mixed-role/shared-AI end-to-end configuration, not the entire
selection matrix. Shared built-in human profiles (distinct fallback colours),
slot permutations and unique-profile reassignment still need live coverage.

The accompanying full regression run passes 276,480 original four-player
setup comparisons, 98,304 assignment/navigation comparisons, all 25,600 PLR
save streams and 800 loader cases, and 2,880 composed startup/new-game
transitions (including colour records, ordering, callbacks and shared RNG).
These broaden component coverage but do not substitute for the remaining
live selection combinations. The standard executable is rebuilt.

### Clear Top 10s: original track record writer

Traced `1a96b..1aa3b`: clear the 319-byte working table and trailer first,
show the original question at (160,100), and write tracks only when the
returned scan is 0x15 (Y). The loop constructs the configured track path,
track-list name and `.SS`, calls `1a7ea`, then shows its completion message.
It does not delete separate high-score files. The generic save-under message
dialog is `347da..3498a`; it still needs its own native renderer/input port.

`slicks_write_track_records` now ports the track writer's data transformation:
only format byte 2 is rewritten; signed lap-time comparison may promote
record one into record zero; eleven 29-byte records receive big-endian
checksum words and marker bytes, followed by the trailer and cumulative
checksum byte. It touches offsets 8..362 only. Header, track geometry and
all remaining bytes are preserved. Truncated buffers are rejected before
any mutation (including the working table). The pre-confirmation working
table clear is separate from persistence.

`make verify-track-record-write` passes 2,048 full-file and working-table
comparisons against original DOS `1a7ea`, with only file I/O hooked into
memory. Coverage includes signed time extremes, old-format no-op, checksum
wrapping, record promotion and untouched surrounding bytes. Every short
buffer is rejected atomically. Original `1a971..1a999` also matches the
native clear, and cleared output decodes successfully with valid checksums.

No track files were changed. Clear Top 10s is not yet connected: original
confirmation rendering, track enumeration/writes, safe failure handling,
cancellation and isolated live disk verification remain required.

### Original save-under confirmation renderer

`message_dialog.h` ports the blocking/save-under form (flags zero) of
`347da..3498a`, the form used for both Clear Top 10s messages. Its width is
half the original measured text width plus seven pixels on each side;
height is font height plus eight, starting four pixels above the text.
It preserves the original four-pixel-rounded save-under, grey-15 palette
tint at the caller's percentage, centred text colour (60,60,30), immediate
font-colour restoration and full background restoration on close.
Storage remains caller-owned; AmigaOS allocation and keyboard waiting are
separate boundaries. Unsupported nonblocking/shadow flag variants are not
silently approximated by this interface.

The full `verify-list-pixels` run passes 48 confirmation/completion open/close
cases against original DOS code and actual native 68020 font routines,
covering four alignments/positions, two tint percentages and six keyboard
results. Only allocation/free and the blocking keyboard boundary are hooked;
original palette conversion, text drawing, save-under and restoration execute.
Every full frame and font colour matches, and the original routine returns
the supplied scan unchanged. Existing name, colour, Options, Controllers and
list pixel gates also remain green. Zero-capacity save-under attempts are
rejected before touching the screen or font, followed by successful opens
on the same dialog instance. No track-file write or visible Options
action is enabled by this renderer alone; native ownership/input integration
and the failure-aware track-file adapter remain outstanding.

### Failure-aware track-file adapter

`slicks_amiga_clear_track_records` now reads a complete track using the
actual AmigaDOS adapter, applies the original record serializer to a private
buffer, and stages/publishes it through a single-file variant of the existing
setup transaction. The 8,192-byte ceiling matches the native race loader.
The `.SS` signature, minimum record size and full EOF read are checked;
missing, malformed, truncated or oversized files cannot become cleared tracks.
Original old-format tracks remain unchanged. Partial reads/writes are handled.

Same-directory `.new`/`.bak` artifacts stop the operation before mutation.
A reported failure rolls back the old track where possible; rollback failure
preserves recovery artifacts, while backup-cleanup failure reports a committed
update separately. The result includes the caller-owned track path and I/O
error, and `changed` is true only for a committed update. This is per-track
failure handling, not power-loss atomicity or an all-tracks transaction.

`make verify-track-storage` exercises the actual adapter with a deterministic
in-memory DOS filesystem: 1,225 single/double fault combinations cover reads,
creation, short writes, flush/close, renames, rollback and cleanup. Every case
preserves either the complete original track or the exact committed track;
additional guards cover existing artifacts, missing/invalid files, all short
record prefixes, oversized input and the old-format no-op. Setup persistence
regressions still pass 2,600 storage faults and 254 load checks; the 2,048
original-DOS record-writer comparisons remain green. Amiga target compilation
passes as SlicksTrackStore2.

The API is not yet called from the visible Options action, so no track files
have been changed. Remaining work: connect native message ownership/input,
enumerate the existing track list only after Y confirmation, report partial
or recovery failures accurately, and run cancel/confirm/failure tests against
isolated FS-UAE track copies.

### Clear Top 10s connected and verified on A1200

The Options action now owns a native save-under message dialog using the
original question and completion strings exported from the executable.
Any mapped non-modifier key other than Y cancels without a write; Y walks
the discovered track list with the failure-aware adapter. A failure stops
the loop, displays a partial-clear/recovery warning, then the affected path.
It never substitutes the success message for a partially completed clear.
Allocation and disk operations run with the OS restored; each message resumes
the established hardware takeover, and close restores the underlying Options.

`SlicksClear2`, `OPTIONSR`, and `diag_clear_records.gdb` pass on a muted
68020/2 MiB/no-Fast-RAM A1200. The test cancels, reopens, confirms, acknowledges
and exits with restoration status 0x1f. It observes zero adapter calls before
confirmation and exactly 195 afterward. `verify_cleared_tracks` compares all
195 complete isolated files against the independently DOS-verified serializer:
all match, including unchanged headers/geometry/tails, with no .new/.bak
artifacts. The native question screenshot was rendered and visually checked.

The first diagnostic incorrectly counted disabled Options rows as selectable
and navigated to Exit. Its corrected sequence selects the mode minimum then
Custom, and skips the two disabled rows, so saved settings cannot shift its
target. This was a test-input error, not a successful action or a Guru.

`SlicksClear3`, `OPTIONSS`, and `diag_clear_records_failure.gdb` exercise a
pre-existing `TRACKS/1WAY.SS.new` in a separate isolated filesystem. BASIC.SS
commits first, then the next track reports recovery-required. Both warning
and filename screens are reached; dismissal returns to Options and clean exit.
Host comparisons verify BASIC.SS matches the successful cleared version,
the other 194 tracks match the reference byte-for-byte, and the sole recovery
artifact retains its diagnostic text. Reference tracks were never modified.
This proves a partial/recovery failure; it does not claim every hosted I/O
fault was reproduced on a live Amiga volume.

Storage regressions remain green: 1,225 track-adapter faults, 2,048 original
track-writer comparisons, 2,600 setup-storage faults, and 254 setup-load checks.
The standard SlicksDiag is rebuilt with this action. Remaining goal work still
includes track setup, broader option consumers (audio volume is not connected),
Help orchestration/failure coverage and the rest of the live player-selection
and persistence-failure matrix.

### Sounds and BG Sounds now reach the Amiga audio adapter

Original `2a2e9`/`2b6fb` apply option 1 through `392ef`: consume its low byte
and clamp to 100. This is the master gain, not an effects-only control.
`2973f..29748` passes option 2 to `39bd9`; `39bf0..39bfd` produces the byte
`lowbyte(lowbyte(option)*5/2)`, and the WAV loader at `389e2..389fc` scales
signed samples by that gain over 255. Thus background audio is attenuated
in addition to the master; engine sound does not use the background option.

`audio_volume.h` preserves those option interpretations. The Amiga boundary
maps the master and background gains to Paula's linear 0..64 volume range.
This hardware quantization is explicit: it is not a claim of bit-identical
DOS software-mixer sample rounding. The engine and two effect channels use
master volume; the existing intermission WAV uses master times background.
At defaults this is 64/31 rather than the former hardcoded 64/32.

The adapter receives loaded configuration after audio creation and updated
values after Options edits. The setter writes memory only. Playback starts
use the selected gains; changes for already-running channels are applied by
the existing blanking-window audio update. Unchanged gains produce no new
volume stores, and changing gain does not change DMACON or playback priority.

`verify-audio-volume` executes actual DOS `392ef` and the background gain
instructions for all 65,536 word arguments, then checks all 25,856 valid
master/sample-gain mappings for bounds and monotonicity. The separate
`verify-amiga-audio-volume` compiles the actual adapter with allocation and
custom-register boundaries mocked: all 10,201 UI combinations check engine,
both effects, music, deferred writes, runtime mute and unchanged-write elision.

Muted A1200 `SlicksVolume3` / `OPTIONSV` / `diag_audio_volume.gdb` passes
`AUDIO_NATIVE_MENU_TO_RACE_VOLUME_OK`: ordinary native menu keys select Sounds
25% and BG Sounds 20%, return to title and start the race; the live audio
state contains Paula gains 16/3, the engine has started and pending volume
updates have been consumed. The standard executable is rebuilt. This gate
does not yet independently exercise volume save/restart or audibly compare
the results music; those remain part of the integrated completion audit.

### Shared Human profile survives save/restart with distinct colours

`SlicksShared2` / `PLAYERSH` uses ordinary native Players menu keys to select
Human in slots 0/1/3 and None in slot 2, then exit and save. The explicit
diagnostic save permission is separate from the Add/Edit persistence fixture;
the first version correctly hit the general diagnostic no-write guard and
was not counted as a passing save. No runtime selection is injected by GDB.

`diag_setup_shared_save.gdb` passes on the muted 2 MiB A1200: selections
2/2/0/2, participation -1/-1/0/-1, human order 0/1/3, count 3, original
fallback RGB endpoint pairs 0/1/2 and clean system restore 0x1f. CFG is 142
bytes and PLR is the valid three-byte empty-custom-profile stream; built-in
profiles are recreated by the original startup rules rather than serialized.

A fresh `SETUPR` process on the same isolated filesystem passes
`diag_setup_shared_reload.gdb`: both files load, all selections/roles/order
recover, every active vehicle equals the resolved setup vehicle, and the
palette actually passed to the race has all five expected shades for each
active driver. The human in slot 3 uses the third fallback colour, not the
fourth: the inactive slot does not consume a fallback entry.

Selection dumps compare byte-for-byte. All eighteen active colour bytes
also compare byte-for-byte. The six inactive-slot colour bytes intentionally
do not: original selection code leaves them untouched, so the pre-save slot
retains its formerly selected Computer colour, while fresh startup has zeros.
The test does not normalize those irrelevant bytes or claim they persist.
Likewise a random built-in vehicle is checked against the current resolved
choice, not required to survive a changed startup RNG seed.

This closes one shared-human save/restart/race case alongside the prior
mixed human/shared-AI/inactive case. Full slot permutations and live unique
profile reassignment remain outstanding, as do the other setup-screen and
failure-handling items. The normal executable is rebuilt.

### Original Tracks screen: navigation and playlist semantics

Title activation at `2a509..2a50f` calls `26dd5`, a playlist editor rather
than a single-track chooser. It displays 22 names at eight-pixel spacing,
retains cursor/top in DS:10b2/10b4, highlights selected names, and has six
right-hand actions: Lists, Random On/Off, All, None, Random subset, Exit.
The subset count is DS:0626 (already part of CFG); DS:0624 enables order
randomization. DS:0090 and the array at DS:062a hold the ordered playlist.
F1 uses topic `track_m`; T, E and F2 invoke Top 10 while in the names column.
Left/Right select columns, or adjust the random subset size on its row.
Home/End, arrows and PageUp/PageDown navigate the names; page stride is 21.

`track_menu.h` ports `274e5..2794a` navigation and action dispatch, with
explicit caller boundaries for playlist changes and modal screens.
`verify-track-menu` executes the original switch and compares 62,720 cases
covering all scan bytes, all seven columns, empty/small/page-edge/195/256
track counts, signed cursor overflow, scroll bounds and random-size extremes.
The menu's previous-cursor invalidation, done flag and random-order toggle
match; modal bodies/playlist mutations are hooked only at those boundaries.

`track_playlist.h` separately ports toggle/append/removal (`27583..2765f`),
All/None (`276bb..27711`), unique random subsets (`2771c..277ff`) and order
shuffle (`26d34..26dd4`). Shuffle draws over the entire list for every entry;
it must not be replaced by Fisher-Yates. Duplicate subset draws consume RNG
steps and retry. Removal/clear preserve unused array tails as the original
does. Caller-owned capacity guards reject impossible native requests before
mutating storage, count or random state.

`verify-track-playlist` runs the real original mutations, RNG and arithmetic
helpers, replacing only the name lookup boundary. All 480 cases match the
complete 256-word array (including unused tails), count and RNG state, across
six track counts and sixteen seeds/variants for all five operations.
This is not yet a visible Tracks menu: original rendering, saved-list and
Top 10 dialogs, resource ownership, playlist persistence and race sequencing
still need integration. The title's temporary single-track cycling remains
until those original behaviors are ready; no completion is claimed for it.

### Tracks redraw command port

`track_menu_draw.h` now ports `270b9..274d5`: skip unchanged cursor state;
restore the three original background rectangles; draw the left-name or
right-action bevel; paint six action labels and the original random-order
label; clamp/display subset count; display selected/total counts; position
the proportional scrollbar; tint every selected visible name and paint the
22 visible names. It updates the original previous-cursor marker after drawing.
The native callback boundary reuses the existing menu restore/bevel/font/
numeric/palette operations. It does not introduce a recreated screen image.

`verify-track-menu-draw` executes the original redraw and compares the full
ordered command/text stream and mutated menu fields for 392 cases: seven
track counts (including zero, page edges, 195 and 256), all seven columns,
eight scroll/selection/random/count/no-redraw variants. Original text, numeric
positions, selected-name tint bounds, font colours and scrollbar geometry
match. Only drawing primitives, name lookup and the display-page flip are
hooked; original loop arithmetic, label lookup and dispatch run directly.
The table-driven labels still come from the original runtime in this oracle.

This is a command comparison, not yet a pixel comparison or Amiga integration.
Preparation `26dd5..270b6` first loads `/trckmenu.@p`, builds a selection tint,
prepares title/footer/panels and saves its background. Port that resource and
preparation stage, then compare composed pixels with the actual native fonts
before wiring the screen into the title. The saved-list and Top 10 modal
bodies, persistence and ordered race playlist consumption remain open.

### Tracks static preparation pixels

`track_menu_prepare.h` ports original `26e62..2702e`: top/footer tints,
the resolved heading with iso font, instruction footer with pieni font,
right-hand panel and the scrollbar background only when track count exceeds
22. The caller supplies the decoded `trckmenu.@I` image, raw 768-byte
`trckmenu.@p` palette and original fonts. This is asset-based painting,
not a captured DOS frame.

The panel's two `0xacb7` arguments are not signed RGB inputs to the table
builder: wrapper `34654` recognizes blue `0xacb7` as a greyscale sentinel
and replaces both green and blue with red. The port therefore uses
`20,20,20` for that panel. Executing the wrapper in the pixel oracle caught
this distinction. Oracle font/palette storage is disjoint (the iso font
extends beyond `0x65000` when based at `0x64000`).

`make verify-track-prepare` passes 42 whole-screen and whole-font comparisons
against the original DOS preparation, using actual archive resources and
the actual 68020 string renderer. Cases cross counts 0, 1, 22, 23, 195, 256
with percentages 66, 0, 50, 100, 127, 128, 255, including signed-byte tint
behavior. No drawing or text primitives are replaced with native oracle
results. File loading, fades and snapshot ownership lie outside this gate.
The 62,720 navigation, 392 redraw-command and 480 playlist/RNG comparisons
also still pass.

Still open: composed dynamic Tracks redraw pixel testing, native screen
resource/snapshot lifecycle and title entry, saved-list/Top 10 modals,
persistence and ordered race playlist consumption. The standard Amiga build
does not yet expose the new Tracks screen. Its existing Guru guard was
rechecked directly against `amiga/out/SlicksDiag.elf` and passes.

### Tracks composed redraw and Amiga resource adapter

`track_menu_renderer.h` connects the original redraw to the existing native
saved-background, bevel, rectangle, palette and text painters. It owns the
selected-name tint and scrollbar colour, while the caller owns the surface
and name catalogue. Its number formatter handles signed word arguments
rather than the profile editor's three-digit byte percentages. Preparation
also snapshots its own completed background for subsequent restores.

The Tracks pixel oracle now includes 168 sequential redraws across all seven
columns and track counts 0, 1, 22, 23, 195 and 256. It compares all 64,000
pixels, the complete normal font object, previous-cursor state and subset
count. Cases exercise selection tints, scrolling, both random labels,
oversized count clamping, negative count rendering and skipped redraws.
Only track-name lookup and page flipping are hooked during original redraw;
DOS painting, tint generation, number conversion and text execute directly.
Both sides start from the independently compared original asset preparation.

`slicks_amiga_track_menu_create` loads the actual image, palette and three
fonts, prepares the screen and snapshot and marks all rows dirty. Failed
creation releases its allocations and clears the renderer surface pointer.
It follows the existing menu ownership and OS-available loading convention.
The adapter cross-builds successfully, but is not yet called by the title
loop and has not passed a live Amiga lifecycle/failure test. Title entry,
input/modal integration, saved lists, Top 10 and playlist persistence/race
consumption remain open; this is not a completed native Tracks screen.

### Native Tracks entry, toggle, reopen and first-race gate

The native title now opens Tracks on Enter (the legacy non-native diagnostic
title retains its single-track cycling). Native Tracks loads its own original
resources, uses interval dirty-row C2P, handles original navigation and
toggle/All/None/shuffle/random actions, opens Help, returns to the title and
retains cursor/top, random-order state and the ordered selection in memory.
Subset count updates the existing CFG field 0626. Saved Lists and Top 10
currently show explicit development notices, not substitute implementations.

`SlicksTracks1`, muted A1200/68020/2 MiB/no-fast-RAM, isolated
`.run/tracks-v1`, passed `TRACKS_NATIVE_ENTRY_TOGGLE_RETURN_REOPEN_RACE_OK`.
The TRACKS diagnostic enqueues ordinary title/menu keys: open, deselect BASIC,
select 1WAY, close, reopen, close, GO. Five draw checkpoints and two closes
passed; reopened selection is exactly [1], cursor 1, column 0, count 1.
The actual race loader received `TRACKS/1WAY.SS`, and race entry passed.
`verify_amiga_frame.py` compared all 64,000 menu pixels against the displayed
interleaved bitplanes without differences. Dumps/PNG are diagnostic-only.

The standard build additionally implements original GO's empty-selection
fallback (2a593..2a5e4): one RNG-selected track, rather than retaining the old
single-track choice. This branch still needs its own native input test.
The standard executable rebuilt successfully with the runtime Guru guard.

Visual inspection of the native dump reveals a missing heading: the literal
lowercase `tracks` fallback is not present in the iso font's character set.
The existing pixel oracle uses the same fallback with the language table
absent, so its pass does not prove correct original font/language startup.
Investigate the original font configuration/language lookup before claiming
the whole screen visually faithful. The archive contains lang1..lang8; lang1
does not map this key, while other languages do. Also audit the catalogue's
filename/display-name distinction (current native names include `.SS`).

Still open: these visual initialization issues, full Tracks action/live Help
and failure tests, original saved-list and Top 10 modals, playlist persistence,
ordered multi-race consumption and clean-exit restoration for this gate.

### Original screen confirmation and corrected catalogue startup

The missing English heading is **original behavior**, not a port regression.
A fresh bounded DOSBox-X 286 run from disposable `tmp/pc-tracks-heading`
opened Tracks with ordinary Down/Down/Enter input. Its video frame at 24 s
also has no heading. Original startup `19dd8..19e28` loads kirj, pieni and
iso sequentially; English lang1 does not replace the lowercase heading key,
and iso does not contain those lowercase glyphs. No invented uppercase title
or modified font was added.

The live reference instead exposed genuine integration differences:

- Original catalogue `35d28` with strip-extension=1 stores eight-byte base
  names. The native display now omits `.SS` while retaining full IO filenames.
- Original startup `26164/26169` selects all tracks with `26ced`, then shuffles
  the ordered playlist with `26d34`. Native setup now does the same using the
  already differential-tested routines and setup RNG. It no longer starts
  with a singleton BASIC selection.
- Native setup keeps the alphabetical catalogue; the old BASIC-first promotion
  remains only for legacy non-native race diagnostics.

`SlicksTracks2` passed the revised muted A1200 test on port 25018: initial
195 selected tracks; ordinary None action; select 1WAY; close/reopen/close;
GO; actual loader path `TRACKS/1WAY.SS`; successful race entry. Nine draw
checkpoints and two closes passed. The first frame and reopened frame are
local diagnostics in `.run/tracks-v1`.

`verify_menu_reference_frame.py` compares the initial native chunky indices
and six-bit palette against the original DOS capture, checking every pixel
of the capture's integer-scaled blocks. **All 64,000 pixels match in six-bit
RGB** (the 640x400 capture contains four checked source pixels per native
pixel). The reopened Amiga frame also passes the complete chunky/bitplane
comparison. Reference captures are test evidence only, never game assets.
The standard `SlicksDiag` was rebuilt and passes its runtime recursion guard.

This resolves the English heading and displayed catalogue questions above.
Language switching, original saved lists/Top 10, persistent playlists,
multi-race sequencing and the remaining full setup audit are still open.

### Original saved-track-list file codec

`track_lists.h` implements the `SLICKS.TRK` record layout read by `2d28a`
and written by `2d4db`: six-byte SSTrk/1a header, big-endian list count;
per record, big-endian track count, 21 title bytes and eight bytes per base
name. Like DOS the reader checks only the first four header bytes and
ignores trailing bytes. Unlike unchecked DOS IO, it validates complete
records, signed count limits and title termination before publishing a view.
Truncation and output-capacity errors leave outputs unchanged.

Loading a selection matches catalogue base names with original `11035`
ASCII-only case folding. It preserves file order and duplicates, chooses the
first catalogue match and omits missing tracks. The untouched playlist tail
remains unchanged. The comparison caught and corrected an initial mistaken
case-sensitive lookup assumption.

The writer creates a complete append/delete image in separate caller-owned
storage. Existing records are copied byte-for-byte; new records use the
original header, 20-character title plus terminator, and padded eight-byte
base names. It validates indices and capacity before writing and checks
sizes without wrapping. The caller must install it through the existing
safe-save transaction, not emulate DOS's rename/delete-before-commit behavior.

`make verify-track-lists` passes 60 complete playlist/count/untouched-tail
comparisons against the original reader and 80 entire-file comparisons
against the original writer. Original arithmetic, record scanning, name
comparison and serializer execute directly; file IO and catalogue lookup
are host boundaries. Cases cover multiple lists, missing and duplicate names,
eight-character names, case folding, empty selected lists, arbitrary skipped
header bytes, every truncated input and every short output capacity.

The codec is not yet connected to Amiga file transactions or the Lists
dialog. That integration, native save/restart/reload and failure handling
remain required; no saved-list persistence completion is claimed here.

### Amiga saved-list file adapter and failure gates

`amiga_setup_storage.c` now provides `slicks_amiga_load_track_lists` and
`slicks_amiga_store_track_lists`. Both require the OS available. Catalogue
files are bounded to 64 KiB for the 2 MiB target; oversized data is rejected,
not truncated. Load uses temporary storage and publishes the caller's bytes
and borrowed view only after full read, close and format validation. Missing
SLICKS.TRK produces an empty catalogue without creating a file. Existing
`.new`/`.bak` files block loading and saving with recovery-required status.

Save uses the original codec and existing one-file transaction: write/flush/
close a new file, back up the prior file, install the new one, then remove the
backup. Results distinguish retryable failure, recovery required, committed
success and committed cleanup pending. No caller playlist or catalogue is
changed by the storage operation. The future dialog must reload after commit
and keep an actionable error/recovery path on failure.

`verify-track-list-storage` executes this production adapter against the
short-read/short-write AmigaDOS mock. It passes 361 single/double save-fault
cases for append/delete, checks all result categories, retains either the old
or committed file and leaks no allocations. Load failures cover allocation,
open/read/close, every truncated prefix, insufficient caller capacity and
recovery leftovers without publishing any bytes/view. Missing-file first save
followed by a fresh load recovers the exact ordered selection [2,0,2,1].
The existing 1,225 track-record storage faults and 60/80 original codec
comparisons also pass. The standard Amiga cross-build and runtime guard pass.

These are hosted production-adapter checks, not native UI save/restart proof.
The Lists dialog still needs to call the adapter, provide title entry and
delete confirmation, handle errors, and pass live A1200 lifecycle tests.

### Lists caller semantics and native picker ownership

`track_list_dialog.h` ports caller dispatch `2d7b5..2d88f`, including the
original numeric return ranges rather than a guessed action-bit mask.
Load requires a nonempty catalogue; Save requires a nonempty selection;
name entry accepts original AL=0; deletion accepts only Y scan 0x15.
`verify-track-list-dialog` passes 648 action/index/prompt/write/reload/free
trace comparisons covering cancellation, empty catalogues/selections, every
range boundary, and accepted/cancelled name and confirmation results.
The oracle runs original branch code with only dialog/storage calls hooked.

The Amiga menu adapter now owns a `SlicksAmigaTrackLists` context and exposes
open/picker/choice/close functions. Open uses the checked storage loader,
copies record titles and opens the existing original list renderer at
(150,20)..(280,90), stride 21, normal font, initial row/action zero, flags zero.
Close restores nested dialog backgrounds and releases catalogue/picker
allocations; destruction also releases the context. The original action,
name-prompt and delete-question strings are exported from initialized DS.
Cross-build, runtime guard, caller tests and storage regressions pass.

This picker adapter is not yet called by the Tracks input loop. The existing
Lists development notice remains until name/confirmation/commit/error phases
are connected. The shared list renderer currently caps catalogues at 100;
larger files are rejected without mutation and need a deliberate target-limit
decision or expanded renderer coverage before declaring full compatibility.
Also resolve original list-name dialog geometry (185,40): its invisible
right-margin field save can exceed the native 320-pixel surface. Do not move
the dialog to guessed coordinates; compare clipped visible pixels against
the original wrapper. These are next integration tasks, not completed gates.

### Original track-list name prompt geometry verified

The `(185,40)` name prompt now uses visible save-under helpers that retain
the original four-pixel-rounded saved stride but pad invisible VGA-margin
bytes and clip native restoration/dirty bounds at x=320. Text remains at
the original coordinates; the existing 68020 glyph renderer clips edge
glyphs. Cursor capture/restore uses the same rule, including a completely
offscreen cursor. The strict ordinary rectangle APIs remain unchanged.

`verify-list-pixels` now runs both player-name and track-list-name positions
against the original wrapper: 276 full-screen/font-state comparisons pass
through initial names of length 0/3/20, typing, deletion, blinking, accept,
cancel and outer restoration. The VGA oracle retains actual invisible
margin bytes independently; native padding is never used as oracle data.
The other dialog/pixel gates also pass. `verify-saved-rectangle` adds 21
edge alignments with guarded storage/surface, bottom-row and clipped dirty
bounds checks; its existing 192 original-code comparisons still pass.

The Amiga adapter exposes `slicks_amiga_name_dialog_open_at` for an owned
21-byte buffer and explicit coordinates. The profile wrapper keeps its
existing geometry and active-editor requirement; closing a non-editor
prompt no longer changes profile-editor state. The standard Amiga build
and runtime leaf-function guard pass. Lists input/storage integration and
live A1200 proof remain outstanding; this is not yet a connected Lists flow.

### Native Lists save/restart/load and cancellation connected

The Tracks Lists development notice is replaced by the original picker,
name prompt and delete confirmation. Dispatch uses the original caller
decoder; accepted names append the current ordered playlist through the
transactional SLICKS.TRK adapter. Load uses original basename matching and
updates selection; deletion uses the original question, iso font and Y scan.
Closing releases nested modal/catalogue allocations and redraws Tracks.
Storage operations run with the hardware takeover released. Failed load/save,
recovery-required and committed-with-backup-leftover results have distinct
messages, rather than reporting success or silently overwriting leftovers.

Muted A1200/2 MiB `SlicksLists1` runs in `.run/track-lists-v1`:

- `TRACKSL` uses ordinary menu keys to clear selection, select catalogue
  indices 0/1, save list `A`, and start the race. The 47-byte file contains
  the canonical header, one list, and `1WAY` then `66` in original format.
- A fresh `TRACKSR` process begins with only track 0 selected, loads `A`
  from disk, recovers exactly [0,1], and reaches the race without errors.
- `diag_track_lists.gdb` passes both runs; the name prompt is checked at
  (185,40). The reload run never enters name entry.

`SlicksLists2` / `TRACKSD` then passes
`TRACK_LIST_NATIVE_CANCEL_NAME_CANCEL_DELETE_CONFIRM_REOPEN_OK`: reject
Delete with N, cancel Save after typing A, confirm Delete with Y, reopen an
empty catalogue, and start the race with the unchanged selected track.
Catalogue counts are checked after each reopen; the question uses font 2.
Only the isolated fixture's saved list is deleted; the final SLICKS.TRK is
the eight-byte empty catalogue. Reference assets and user setup are untouched.

The original caller's 648 comparisons, 60 reader/80 writer comparisons and
361 storage-fault cases still pass. Native filesystem failure-message paths,
the 100-list renderer limit, complete Tracks Top 10 and multi-race playlist
sequencing remain open. These are component gates, not full goal completion.

The expanded message oracle loads the actual iso font for the original
track-list deletion question: all 72 open/close/full-screen/font comparisons
pass, alongside the other dialog gates. The first expanded harness run
failed because that font fixture had not been loaded; this was corrected
before the successful rerun. The refreshed standard SlicksDiag also passes
the ordinary Tracks entry/toggle/return/reopen/race regression (port 25024).

### Native list-load failure return paths verified

Muted A1200/2 MiB runs of the standard binary now exercise actual filesystem
failure inputs, not injected storage callbacks:

- `.run/track-list-recovery-v1`: an existing `SLICKS.TRK.new` produces
  `SLICKS_SETUP_LOAD_RECOVERY` and a warning with the catalogue/picker/name
  allocations released. Acknowledging it returns to Tracks and permits a
  race with the original one-track selection.
- `.run/track-list-invalid-v1`: a malformed `SLICKS.TRK` produces
  `SLICKS_SETUP_LOAD_INVALID`, shows the load-failure warning, and follows
  the same safe acknowledgement/return/race path.

`diag_track_lists_failure.gdb` and its invalid-file wrapper check active
display/message ownership, exact load result, unchanged selection, absence
of lingering modal ownership and error-free race entry. Both pass (ports
25025/25026). Before/after SHA-256 checks match for each fixture; directory
inspection shows no replacement/new backup was created. The original game
assets and normal run's saved state are untouched.

This closes the live malformed/recovery **list-load** UI checks only.
Unwritable save UI and CFG/PLR live recovery checks remain separate work.

### Write-protected saves: requester stall fixed and live UI checked

`debug.sh` now accepts `SLICKS_DEBUG_READ_ONLY=1`, mapping only the test's
DH1 volume read-only. `run.sh` is unchanged. The first TRACKSF run stalled
after accepted name entry, with hardware released and no save report: DOS
was waiting rather than returning the write-protection error. The previous
hosted fault tests could not expose a blocking DOS requester.

The platform adapter now suppresses DOS requesters around each storage
transaction by saving `pr_WindowPtr`, setting it to -1, running the existing
transaction, and restoring the exact prior value. This applies to CFG/PLR,
saved track lists and Clear Top 10 writes; it does not globally disable
requesters. The native gates explicitly check restoration of the old value.

Muted A1200/2 MiB `SlicksLists4` tests pass:

- TRACKSF, `.run/track-list-readonly-v1`, port 25028: saving a two-track
  list returns DOS error 214, presents the native save-failure warning,
  dismisses cleanly and starts a race with the unchanged selected tracks.
  The existing empty catalogue remains byte-identical (SHA-256 checked)
  and no temporary or backup file is created.
- PLAYERSX, `.run/setup-readonly-v1`, port 25029: creates/edits/selects
  profile ABC through ordinary menu keys, fails CFG/PLR saving with error
  214, cancels the failure screen and reopens Players with all four profiles
  and the new profile still selected. No CFG/PLR files are created. This
  proves retained in-memory edits, not persistence to an unwritable disk.

Gates: `diag_track_lists_save_failure.gdb` and `diag_setup_readonly.gdb`.
Storage regressions still pass: 361 saved-list faults, 1225 track faults,
254 setup-load cases and 2600 setup transaction cases. The normal binary
has been refreshed with the fix and passes its runtime guard. Live CFG/PLR
recovery-leftover startup checks and the other setup completion items remain.

### CFG/PLR recovery-leftover startup gates

`SlicksRecovery1` adds a diagnostic checkpoint after the existing startup
error message, before the cleanup branch. Four muted A1200/2 MiB fresh boots
with known-good CFG/PLR copies and exactly one leftover each pass
`SETUP_NATIVE_RECOVERY_REFUSED_BEFORE_MENUS` (ports 25030..25033): CFG.new,
CFG.bak, PLR.new and PLR.bak. `diag_setup_recovery.gdb` checks recovery status,
the reported path, no published CFG/PLR load, no menu ownership, no active
hardware takeover and no race entry. It stops at the failure checkpoint;
this gate does not claim to exercise the subsequent process-exit code.

All twelve fixture-file SHA-256 values (four CFGs, four PLRs, four leftovers)
match before and after. No normal-run or reference configuration is touched.
The isolated directories are `.run/setup-recovery-{cfg,plr}-{new,bak}-v1`.
Together with the preceding read-only native saves, this closes the specifically
listed live unwritable/recovery-leftover setup checks. Other setup screens,
selection permutations, options/controller coverage and final completion
audit remain open.

Next original Tracks inspection: the records/info action at 27812 restores
the parent crop, calls 267c6 with selected track and interactive flag 1, then
restores the wider parent crop. The callee loads the selected track, draws
its information/preview and waits while animating the preview. Preserve this
whole original modal rather than replacing the pending action with a generic
table. Address 1a4ef reads the track's string at file offset 0x17a into
DS:4c7c; 1a32d is the preview callee and 1aabc is the earlier panel callee.
Their rendering/data contracts still need tracing and differential tests.

### Original Top 10 drawing decisions translated

`src/ui/track_records_draw.h` ports original `1aabc..1ae8c` as ordered draw
commands: fallback/highlight colour, five-stripe rank highlights, both-font
colour changes for every row, valid-time filtering (signed 1 < time < 30000),
rank/name/time, computer-driver marker/name offset, date fields with original
date-order positions, and vehicle icons. It uses the existing byte-exact
29-byte record entries and retains original coordinates and flags. It is
not yet called by the Amiga renderer or Tracks modal.

`make verify-track-records-draw` runs the original routine under the x86
oracle, hooking only its downstream painter boundaries. All 108 ordered
trace comparisons pass across time/rank boundaries, empty/dated records,
computer flags, vehicle markers, date orders and three origins. The initial
harness needed byte-argument decoding for palette and character calls:
their unused high stack bytes are not colour/character values. This is a
command-level gate, not a full pixel comparison or native modal completion.

Next: connect the command adapter to real fonts/time/icon primitives, verify
pixels, load metadata/preview from the actual selected track, implement the
original interactive preview and restore the Tracks parent surface on exit.

### Records text bridge verified

The existing `slicks_hud_time` already implements formatter 2aceb's unsigned
17999 clamp and truncating raw*5/9 conversion; reuse it in the records
adapter instead of creating another time representation. The existing menu
adapter explicitly rejects flag 4 (shadow/highlight text), which the original
records painter uses for names and times, so it cannot be reused unchanged.

`slicks_records_text` now provides a 68020 C-ABI bridge to the existing
string rasterizer with caller-supplied highlight colour, spacing 1, tab 10
and original shadow offset (1,0). `verify-font-glyph` now exercises this ABI
alongside direct rasterizer calls against original 301ab. All three fonts
pass (650 small-font and 640 each normal/large-font combined string cases),
including full-screen pixels, alignment/flags, font-palette restoration,
return advance and ABI-preserved registers. Existing glyph and measurement
gates also pass. This bridge is available for the upcoming records adapter;
the records screen still is not connected to the Tracks input loop.

### Records renderer composition and Amiga adapter

`track_records_renderer.h` now consumes the verified draw commands: palette
lookup, row rectangles, both font colours, signed integers, shared HUD time
formatting, shadow text and explicit original icon IDs. The expanded oracle
runs original 302b6 integer and 2aceb time wrappers down to text calls. All
216 comparisons pass (108 draw-decision plus 108 composed-renderer cases),
checking string contents, coordinates, flags, font/highlight colours, icons
and row-rectangle pixels. Single-character separator calls are normalized
to one-character text at the capture boundary; full font rasterization is
covered separately, not by this composition test.

Signed date/integer boundaries exposed original 302b6's 16-bit NEG overflow:
-32768 formats as `--.)*(` rather than widened `-32768`. The records formatter
now retains that original behavior; valid dates are unaffected. Tests include
signed day/month bytes and years 0/2026/32767/32768/65535.

The Amiga adapter loads computer/car icons from original archive resources,
draws through the verified records text ABI and includes the one-pixel shadow
extension in producer dirty bounds. Original 2a03b loads carimage16 at DS:4e44
while records index DS:4e40, proving saved IDs 1..10 map directly to native
icon slots 1..10; computer marker -1 maps to slot zero. Cross-build/runtime
guard pass. The adapter is not yet invoked by the Tracks modal; full composed
pixel and live-modal/preview/return tests remain outstanding.

### Full records pixels and original track-info input

`make verify-track-records-pixels` passes 12 complete 64,000-pixel and font
buffer comparisons. It executes original 1aabc using actual Tracks artwork,
palette, fonts and computer/vehicle icons, with only VGA access/ports emulated.
The native composition uses real 68020 text and icon routines. Existing
palette-remap and list-pixel regression targets also pass after the test
adapter extension.

`track_info.h` translates the description reader 1a4ef/character mapping
362d4 and the preview stream/coordinates from 1a32d. Descriptions start at
byte 378; preview objects use the signed big-endian offset at bytes 6..7,
signed count, signed X division by five and original unmasked rotation byte.
Malformed/truncated inputs fail without publishing partial output. No race
framebuffer or screenshot is used as a preview source.

`make verify-track-info` executes the original with only file I/O, unchanged
key input and downstream painting boundaries hooked. All 195 supplied tracks
and 16 synthetic character/signed-coordinate/old-format cases match complete
description buffers and preview object calls. Capacity and truncation guards
are checked separately. This proves decoding/draw decisions, not preview
sprite pixels: the original 35719 scaler, original resource-bank selection,
interactive preview animation and full modal open/close remain to connect.

The current standard executable again passed the runtime leaf-function guard
and muted A1200/2 MiB Help navigation/reopen/system-restoration regression
(port 25034). Parent-menu pixels match before and after Help. The ordinary
`.run/dh1/SlicksDiag` is an older copy; `amiga/run.sh` refreshes it on launch.

### Asset-driven preview scaler and composition

`slicks_track_preview_sprite` translates the scale-five, rotation-zero path
of 35719 after resource-bank orientation. It preserves transparent zero,
the original pre-increment (+1,+1), clipped output and producer-reported
dirty bounds, using mult320 for destination rows. The source remains in its
unrotated chunky form; coordinate lookup avoids an extra rotated allocation.
`make verify-track-preview-pixels` passes 228 complete-screen comparisons
against the original scaler/VGA writes, with exact dirty bounds, varied
sizes, all orientations and clipping at screen edges.

`slicks_build_track_preview` now composes the actual SS object stream from
the existing DAT decoder. The original 1bf20..1bf96 initialization has six
special object types (31,32,33,34,63,64; DS:0777): their bank orientation is
`(rotation & 1) * 3`, not the requested quarter-turn. The preview preserves
this rule. It neither reduces a race framebuffer nor uses captured artwork.

`make verify-track-preview-scene` executes the original bank-construction
decisions and original 1a32d/1a1d4/35719 composition. All 64,000 pixels match
for 194 supplied tracks. The oracle's resource fixtures share the established
DAT decoder; this gate independently checks bank selection, coordinates and
rendering, not DAT decompression itself. The separate scaler gate executes
real VGA writes; composition substitutes the final rectangle/pixel boundary.

RAILROAD.SS contains five unsupported rotation-bank selectors (18 three
times, 27 and 31). The original preview passes them unmasked into resource
lookup. The native composer currently rejects this input before painting;
the test confirms the entire surface remains unchanged. Original resource-failure
behavior is now established in the investigation below; allocator-dependent
garbage is not a valid sprite port. Do not silently mask these values or
claim all 195 rendered previews pass. Description/object-call tests still
cover all 195. The records modal is not yet connected; opening/restoration,
header/description composition and interactive animation remain next.

### Native records modal connected, with original preview shimmer

The Tracks F2/E/T action now opens the selected track's actual records,
uppercased description and DAT/SS-derived preview. The caller loads track
and DAT bytes with AmigaOS available, and the modal owns its saved parent
pixels, two font colours, immutable 64x40 preview and source palette.
Failure unwinds temporary allocations and restores the parent; unsupported
or failed loads report a dismissible warning. Closing consumes the key and
restores the parent rather than also activating an underlying Tracks action.
The historical pending-records placeholder has been removed.

The first live attempt exposed an integration error: the main startup archive
handle had already been closed. The modal caller now opens/closes its own
archive, consistent with the other setup dialogs. It no longer receives that
closed handle. Native opening-phase diagnostics are retained for fault tests.

Original initialization 19dd8..19e7e reads sequential archive resources after
kirj: pieni, iso, helmet, helmetc, **top10cc.@16**, then **peli.@p**. Therefore
DS:4c2c is the Top10 computer marker, not computer.@16, and DS:68ae is the
gameplay palette used by preview shimmer. Corrected the records-only adapter
and full-pixel fixture; the player-menu computer icon is unchanged. The earlier
records pixel test supplied the same wrong icon to both sides, so it did not
prove that asset identity. All 12 records pixel/font comparisons pass again
with the corrected original marker.

`slicks_track_preview_shimmer` translates one original 26a53..26b68 iteration:
three RNG draws, immutable preview sample, shared RGB delta with byte wrapping,
and nearest-colour mapping from gameplay palette to menu palette. All 1,024
sequential full-screen/RNG comparisons pass with distinct palettes and byte
overflow. Native menu scheduling bounds each update to 64 such iterations;
this preserves iteration semantics, not the DOS busy loop's machine-dependent
wall-clock throughput.

TRACKSI (`SLICKS_TRACK_MENU=6`) drives ordinary keys to open records, animate
256 samples, dismiss, reopen, animate again, dismiss, leave Tracks and start
a race. The muted A1200/2 MiB test on port 25037 passes
TRACK_INFO_NATIVE_OPEN_ANIMATE_CLOSE_REOPEN_RACE_OK. The restored 64,000-byte
parent dump is identical, and the native records screen was rendered and
visually inspected. The standard executable includes this connected path.
Full modal/original pixel orchestration, additional tracks, RAILROAD handling
and injected allocation/resource failures remain verification work; this
does not complete the overall player-setup goal.

The final port-25038 rerun after extracting the verified shimmer helper also
passes, additionally asserting both font colours, cursor and playlist count
survive each close. The regenerated before/after parent dumps still match.

### Records rejection, dismissal and retry on the A1200

TRACKSJ (`SLICKS_TRACK_MENU=7`) navigates with ordinary Down/F2 keys to
RAILROAD, exercises the unsupported-bank failure, dismisses the warning,
uses Home/F2 to open 1WAY, animates, closes/reopens, then starts a race.
`diag_track_info_failure.gdb` passes on the muted 2 MiB A1200 (port 25040):
TRACK_INFO_NATIVE_REJECTION_DISMISS_RETRY_REOPEN_RACE_OK. Parent pixels before
the failed open and after warning dismissal are byte-identical. Font colours,
cursor, playlist count and modal ownership are checked; the later successful
opens and race prove the failure did not poison the menu's state. This is a
real asset rejection, not allocation/resource-I/O fault injection.

The first test attempt searched the disk catalogue for a basename rather
than its .SS filename and exited through normal cleanup before opening the
modal. Correcting that test also exposed and fixed the records heading:
it now uses the existing original-style basename callback, not `1WAY.SS`.
The shared mouse/title dispatch now also excludes an open Tracks menu, so a
click cannot accidentally activate the hidden title menu underneath it.

The Options-consumer audit confirms `options[13]` (race time) and
`options[14]` (track count) currently have no consumers outside their settings
serialization/UI. They remain explicit implementation gaps, not covered by
the passing Options drawing/navigation or volume tests.

### Edited audio volumes survive save and a fresh process

OPTIONSW (`SLICKS_OPTIONS_MENU=6`) uses the existing volume-edit key sequence
to set Sounds=25 and BG Sounds=20, then exits through normal native saving
instead of starting a race. Only this explicit diagnostic is allowed to
persist; other Options diagnostics retain their no-implicit-save behavior.
`diag_volume_save.gdb` passes VOLUME_NATIVE_MENU_SAVE_RESTORE_OK with
system restoration 0x1f on the muted A1200 (port 25041).

A separate SETUPR process on the same isolated `.run/setup-volume-v1` disk
passes `diag_volume_reload.gdb` / VOLUME_FRESH_RELOAD_RACE_OK (port 25042).
Both CFG and PLR were actually loaded; the saved settings remain 25/20 at
race preparation, and the audio consumer receives Paula volumes 16/3 with
engine playback started and no pending volume update. No runtime settings
are injected by GDB. The debug host stays muted; run.sh is unchanged.

Before/after restart SHA-256 values match: CFG
`7e02e590c1cc6c68eddc1adbebe0f8f220ade7880ee94a48f4148b2cfded9fe6`, PLR
`1664013f3c435ecfafca7a43e3470cf93395c7a582287bb1f5a3064b1d063684`.
Only the isolated test's configuration/profile pair was created; normal-run
files and reference assets were not changed. This closes the listed edited
volume persistence check, not the remaining Options-consumer work.

### Arcade policy oracle and lap-limit storage prerequisite

`verify-arcade-setup` executes the original 19894/198c9/198fe/1991f and
241cc routines without replacement hooks. The elapsed/remaining/expiry
clocks, 8,400 lap-limit cases and 640 effective-track-count cases pass.
The policy retains wrapping 32-bit multiplication before signed division
by 90, the 9999 initial lap sentinel, all four signed completed-lap counters,
and the signed minimum for Arcade's effective track count. It does not
truncate the saved playlist. These helpers are not yet runtime consumers.

The runtime's lap target was only one byte; it now holds the original word
and the setter no longer truncates values above 255. `verify-race-lap-limit`
checks all 65,536 input values, adjacent fields and the null setter call.
This is a storage regression check, not an independent x86 comparison.
The standard Amiga executable rebuild passes, including its strlen leaf
guard. No emulator run was performed for this storage-only change.

Still to connect: Arcade game-clock progression, timeout-to-final-lap
transition, original timer HUD, and effective multi-track sequencing. Passing
the isolated policies does not establish complete Arcade behavior or finish
the native player-setup goal.

### Arcade race-clock and final-lap consumer connected

Race preparation now supplies mode and race-time settings to the runtime.
Arcade starts with the original word-sized 9999 sentinel; the later generic
lap-setting call cannot overwrite it. A separate wrapping 32-bit game clock
advances from the existing PIT accumulator, including stationary countdown
time. It is not the BIOS-rate status clock. Queries at the original service,
checkpoint and completed-lap consumers apply 1991f lazily, preserving the
important lap-increment-before-query ordering. Expiry selects one more than
the highest completed lap; it does not immediately stop the cars.

The 8,400 original-code cases now also check native one-based lap-counter
mapping through the runtime consumer. `verify-race-lap-limit` additionally
executes 1,800 stationary frame updates across all six modes and checks
clock accumulation, mode protection, expiry/lap crossing and the subsequent
finish. Its target-only particle symbol is an aborting link guard, not an
emulated successful particle call. Existing `verify-dos-ai` and
`verify-dos-hud` suites pass, including ordinary lap/checkpoint behavior.

The freestanding target has no libc stdint header, so the shared policy uses
32-bit int with a compile-time size assertion and explicit wrapping unsigned
arithmetic. The Amiga build and strlen leaf guard pass.

Muted A1200 tests:
- Port 25043: existing saved-volume fresh-reload/race gate still passes.
- Port 25044: OPTIONSA (`SLICKS_OPTIONS_MENU=7`) chooses mode Arcade and the
  minimum five-second time through ordinary Options keys, closes the menu
  and starts a race. `diag_arcade.gdb` passes
  ARCADE_NATIVE_MENU_CLOCK_EXPIRY_FINISH_OK at ticks=1181, target=1, finished=1.
  The debugger observes settings/clock/finish state; it does not inject them.
  This isolated diagnostic does not save over normal configuration files.

The timer HUD, post-expiry display behavior, multi-track sequencing and
Arcade-specific save/restart coverage remain open. The general player-setup
goal is not complete.

### Original Arcade HUD connected and tested

`arcade_hud.h` translates 1f901..1fb46: the 55,188..85,197 background,
integer-second countdown width, alternating LAST/LAP text centred at 70,189,
and the one-row finish-grace bar at y196. Original nearest-palette requests
are grey 4, 40 and 60. `verify-arcade-hud` passes 2,100 complete drawing-command
sequences against the real 1f84d routine, with only graphics/font-colour
boundaries intercepted; its original timer and arithmetic helpers execute.
This is a command comparison, not a full original VGA/font pixel oracle.
The oracle caught and corrected the distinct post-expiry bar top (196,
not the active timer's 189).

The finish deadline helper passes 3,456 instruction-slice comparisons of
22c4d..22cda, including signed long comparisons and overflow. The runtime
updates this deadline after a driver finishes. Arcade supplies current+1800
when no deadline exists; with no unfinished active drivers, current+230 caps
the deadline. The HUD consumes it separately from the time-limit countdown.
Deadline-based race termination/control suppression is not yet connected;
the current all-drivers-finished completion remains a separate open gap.

The renderer writes the authoritative chunky surface (and legacy VGA when
requested), uses mult320, and records dirty bounds. It caches drawing
commands, not framebuffer pixels: unchanged timer phases generate no dirty
updates. Host tests verify exact expected timer/LAST/LAP/grace pixels using
the real font, VGA/chunky agreement, dirty coverage and unchanged-frame
behavior. Existing original HUD regressions pass. The standard Amiga build
passes with its strlen guard.

Muted A1200 port 25045 passes the expanded `diag_arcade.gdb`: ordinary native
Options keys select Arcade/5 seconds, runtime checks observe both text phases
and the finish-grace bar, and a car finishes at tick 1181. Native buffers in
`.run/arcade-v1` are diagnostic output only; timer/last PNGs were inspected.
Their convenience rendering uses the existing race-palette reconstruction,
so it is a layout check, not a verification of selected driver colours.
No captured pixels are used by production code.

Still open for Arcade: deadline termination/control behavior, multi-track
sequencing, full original pixel orchestration and native save/restart
coverage. The remaining Help, records, controller and player-selection
checks listed at the top of this document also remain in scope.

### Finish grace controls and strict race termination

The subsequent original-code audit corrected the first finish-deadline
integration: DS:4bc6 is signed participation, not a human-only flag. Both
computer and human unfinished entrants retain the long Arcade grace period.
Only no unfinished active entrants permits the current+230 cap. The earlier
human-only runtime count was wrong; the description above is corrected.
The oracle now includes mixed positive/negative/inactive participation and
finished/unfinished ranks: 13,824 deadline cases pass.

Another 1,008 comparisons execute 2037d..203f2 and verify the suppression
flag, the two cleared drive bytes and the strict signed deadline comparison.
The connected runtime clears drive latches and skips throttle/brake/input
steering for finished entrants, or unfinished entrants with fewer than 270
ticks remaining. Momentum and damage yaw continue. A race ends when current
time is strictly greater than the nonzero deadline, not at equality or
immediately when the last entrant finishes. Host runtime tests exercise the
269/270/271 boundary, cleared latches and equality/expiry. The former host
test expecting immediate all-finished completion was corrected to match the
original; its active/inactive checks remain intact.

Original 22b71 also resets the lap target to zero after a finish, allowing
lapped entrants to finish at their next lap crossing. The runtime now does
this, and the host test includes an entrant several laps behind the winner.
The original AI/input/runtime policy tests pass; normal and Arcade paths
share this finish behavior rather than having a separate invented shortcut.

The first muted native deadline run (port 25046, before the lap-target-zero
correction) passes ARCADE_NATIVE_MENU_DISPLAY_GRACE_RACE_END_OK at tick 2982,
deadline 2981, with three finishers. A fresh run of the final lap-target
correction passes on port 25047 with the same tick/deadline/finisher outcome.
The native gate also checks that unfinished AI entrants retain the initial
1800-tick deadline. The standard executable contains both corrections.

Multi-track integration must separate new-game initialization from track
preparation: `prepare_race` currently calls `slicks_setup_new_game`, whose
inventory reset and profile-selection RNG consumption must not repeat for
the second track. The current results Return key always goes back to Title;
it does not yet advance the playlist. Multi-track flow and the broader
setup completion audit remain unfinished.

### Native playlist transition integration

`prepare_race` now takes an explicit new-game flag. Initial GO retains the
original inventory/new-game setup; a subsequent track load skips that reset.
On completed-race Return, the native path advances the selected playlist in
order, applies Arcade's effective track count, transfers remaining inventory
back into the session, restores the OS for loading, and re-enters hardware
takeover with the next track. Escape still returns to Title. Completing the
last selected/effective track returns to Title without modifying the playlist.

Original 25937 deliberately refreshes profile selection after a race. This
is now a separate `slicks_setup_after_race` operation: fixed/random-once cars
retain their selection rules, random-per-race profiles consume the next RNG
draws, and cash/inventory are not initialized again. The session gate now
passes 3,840 startup/new-game/post-race transitions against original code,
including selections, colours, callback order and shared RNG. An additional
540 cases execute original 263fb..26439 with only the filename/race boundaries
substituted, verifying order, duplicate entries, count limits and index reset.

OPTIONSB (`SLICKS_OPTIONS_MENU=8`) is the native two-track diagnostic. Its
first key sequence incorrectly toggled entries out of the initial All list;
port 25048 stopped at the playlist assertion. The next version used Clear
and selected the first two catalogue entries; port 25049 reached 1WAY but
failed the race-error/6,000-frame guard before loading track two. Detailed
state was not printed by that version, so the exact cause needs reproduction;
a race timeout was suspected but is not established by the captured output.
That failure is not counted as a passing sequence test.

The current diagnostic selects BASIC then BASICTRK by ordinary navigation
keys and prints detailed failure state. It observes the new-game flag,
track paths, expected post-race RNG draws, and dumps the session before/after
the second preparation to detect resets. Return is injected as a keyboard
event after each completed race; no game state is forced. Its final native
result passes on muted A1200 port 25052: BASIC completes at frame 1408,
BASICTRK at frame 1527, then NATIVE_TWO_TRACK_MENU_RACES_RETURN_OK. The
before/after second-load session dumps compare byte-for-byte equal. The
Return events are injected by the diagnostic on the target, using the same
input queue as other menu tests. Earlier debugger-side input writes did not
read back as intended; those attempts are not passing evidence. No debugger
writes to game state are used by the passing test.

This connects the basic track transition, not every original intermission:
the current results overlay is not the complete original standings/shop
flow, and points/cash awards, inter-track vehicle-choice options, abort and
load-failure recovery need further original-code integration and checks.

### Saved-list capacity audit

The 100-entry guard remains in place. Removing the fixed title array and
count checks alone is insufficient: original list redraw markers narrow the
row to a byte, and scrollbar positioning retains a signed 16-bit product.
Larger catalogues need boundary tests for both before lifting the guard.
The temporary allocation/count expansion was withdrawn; no unverified
larger-list support is included in the normal build.

### Arcade options native save / fresh-process reload

OPTIONSZ (`SLICKS_OPTIONS_MENU=9`) enters Options with ordinary target-side
key events, selects Arcade, sets five seconds and two tracks, returns to
Title and exits through the production save path. Muted A1200/2 MiB port
25053 passes ARCADE_NATIVE_MENU_SAVE_RESTORE_OK, including restoration 0x1f.
The isolated files live in `.run/arcade-persistence-v1`; no reference files
or the user's normal launch configuration were changed.

A separate emulator process on port 25054 loads both CFG and PLR, then starts
an actual race through the existing SETUP diagnostic entry. It passes
ARCADE_FRESH_RELOAD_RACE_OK: Arcade mode and seconds reach the runtime,
the lap target starts at 9999, the timer HUD is initialized, and all four
participation/active-vehicle assignments agree with the setup session.
All fifteen saved/reloaded option words compare byte-for-byte equal.
Debugger scripts only inspect state; native menu events perform the edits.
This closes Arcade option persistence, not the remaining complete setup
audit or original inter-track standings/shop flow.

Configuration regressions also pass: 256 original startup/default cases,
256 complete original save streams, 1,024 original loader comparisons and
141 truncated-prefix checks; 254 adapter-load checks and 2,600 persistence
fault cases retain their existing guarantees.

### Menu Help open-failure recovery

The existing-menu Help entry no longer exits the game if the archive cannot
open or the viewer cannot be allocated/initialized. It shows a native
platform warning using the original message renderer, then restores the
menu on a key press. The warning has one shared reserved 8 KiB save-under
buffer; reporting an allocation failure does not allocate more memory.
It has separate ownership/input handling from profile deletion and Clear
Top 10s prompts, and menu destruction releases that ownership.

OPTIONSL (`SLICKS_HELP_MENU=5`) tests a real absent-path archive open and an
explicit one-shot viewer-allocation failure. It then dismisses each warning,
opens real Help, closes it, exits Options and restores the system. Muted
A1200/2 MiB port 25055 passes HELP_MISSING_ALLOCATION_DISMISS_REOPEN_RESTORE_OK.
Both restored 64,000-byte menu buffers compare exactly with the pre-failure
buffer in `.run/help-failure-v1`. GDB only observes; fault selection and key
events are diagnostic code on the target. Allocation exhaustion itself is
not simulated, and the warning text is platform error UI, not original DOS
text.

Regressions pass: 816 original navigation cases, 98 complete page/font/link
comparisons, 1,166 full-frame/native-68020-text comparisons, 64 topic lookups,
11 byte-exact indexes, 52 chapters and 79 preprocessed lines.

Still open: title Help surface/font allocation recovery (before a menu
surface exists), errors encountered during Help navigation, explicit native
Players/Tracks failure runs, and full original viewer orchestration. The
passing Options failure run does not establish those remaining cases.

### Title Help and navigation-failure recovery

Title Help now handles archive/surface/font/viewer creation failure without
terminating the program. Its error message uses the already resident title
font and an allocation-free 6,160-byte save-under reserve. Dismissing it
restores the exact pre-warning logical pixels, then converts them normally.
Mouse title actions are suppressed while the warning is open. Navigation
errors close/free the Help viewer, restore its underlying screen, and show
the shared reserved warning. Closing a title-owned navigation warning also
frees that title Help surface; subsequent Help opens do not retain ownership.

HELPF (`SLICKS_HELP_MENU=6`) on muted A1200 port 25058 passes
TITLE_HELP_ARCHIVE_SURFACE_VIEWER_NAVIGATION_RECOVERY_OK. It tests an actual
missing-path open, an injected null surface, an injected null viewer, and a
malformed chapter byte rejected by the real renderer on link navigation.
It dismisses all four warnings, reopens valid Help, closes, and exits with
system restoration 0x1f. All four restored 64,000-byte surfaces compare
exactly with the initial title in `.run/title-help-failure-v1`. These are
target-side diagnostic faults, not claims of testing genuine RAM exhaustion
or every malformed resource. Options failure/reopen also passes again on
port 25057.

The earlier port 25056 passed control flow but failed exact restoration:
rebuilding the title changed the track subtitle from the startup hardcoded
BASIC label to the selected playlist track. The final save-under fix avoids
that unrelated redraw. The initial hardcoded track label itself remains a
setup-display issue to correct; `discover_tracks`/playlist selection currently
occur after the first title draw. Original full Help orchestration and native
Players/Tracks failure coverage also remain open.

### Startup track subtitle handoff

Track discovery and original startup playlist selection now precede the first
title draw. The existing title configuration display receives the selected
catalogue name, not hardcoded BASIC.SS. Checksums are taken after that draw;
legacy diagnostics retain their BASIC-first discovery policy.

`diag_startup_track.gdb` with SETUPR opens native setup and queues immediate
GO without editing runtime state. The fresh unattended muted A1200 run on
port 25060 passes NATIVE_STARTUP_TITLE_IMMEDIATE_GO_TRACK_OK: the initial
draw receives SILLAT2.SS and the real race loads TRACKS/SILLAT2.SS. This is a
title-argument/loader handoff check, not a new original-DOS title pixel
comparison. Port 25059 also completed after correcting a debugger variable
name and resuming that same process; port 25060 verifies the final script
without intervention. The title display still uses the existing native
configuration overlay; this fix does not claim to port additional original
title artwork or orchestration.

### Players and Tracks Help recovery coverage

OPTIONSM/OPTIONSN (`SLICKS_HELP_MENU=7/8`) extend the existing target-side
missing-archive and viewer-allocation-failure test to Players and Tracks.
They enter the real menus, dismiss both warnings, reopen real Help, close
it, return to Title and exit. The debugger checks all four profile selections
and profile count remain unchanged after each failure; no setup state is
injected. It also checks viewer/warning ownership and system restore 0x1f.

Muted A1200/2 MiB ports 25061 (Players/context 1) and 25062 (Tracks/context 2)
pass HELP_MISSING_ALLOCATION_DISMISS_REOPEN_RESTORE_OK. Each restored menu
compares byte-for-byte with its own 64,000-byte pre-failure surface after
both faults. The shared `.run/help-failure-v1` capture files were compared
after each run; the final files contain the Tracks case. This closes the
missing native menu-context coverage, not the full original Help viewer
orchestration comparison.

### Unique-profile reassignment and persistence

PLAYERSY (`SLICKS_PLAYER_MENU=16`) creates ABC, edits its first colour endpoint,
cancels a second-endpoint change, and accepts it through the existing native
Add/Edit flow. It then picks ABC into each driver slot, including assignment
over None. Six observed zero-based layouts are:
`3,1,1,1` -> `0,3,1,1` -> `0,1,3,1` -> `0,1,1,3` -> `3,1,1,0` -> `1,1,3,0`.
Profile 3 remains non-shared; displaced owners receive the destination's
previous selection. All transitions use the real picker, not direct writes
to configuration/session state.

Muted A1200/2 MiB port 25063 passes
UNIQUE_PROFILE_ALL_FOUR_SLOTS_DISPLACEMENT_SAVE_OK and restores the system.
A fresh native process on port 25064 reloads CFG/PLR and reaches a race with
shared AI in slots 0/1, ABC human in slot 2, slot 3 inactive, and human input
order starting with slot 2. Its vehicle and colour endpoints match the saved
profile; the saved/reloaded profile records compare byte-for-byte equal.
The expanded fresh-process gate also passes on port 25065, verifying all
45 RGB components of the active drivers' five-shade ramps supplied to the
race, in addition to assignments, input order, vehicles and endpoints.

The original profile oracle also passes 98,304 assignment/navigation cases,
276,480 complete setup-state/call-order cases, all 65,536 game-type values,
7,680 vehicle/RNG choices, 8,032 non-modal keys, 96 modal dispatches, 512 menu
draw sequences and 864 editor draw sequences. Native coverage is the explicit
sequence above, not a claim of every permutation of several unique profiles.

The complete profile lifecycle regression also passes: 25,600 original save
streams, 800 loaders, 768 built-in initializers, 4,419 deletions, 1,280 new
profiles, 630 editor commits/cancellations, 6,108 editor key cases and 1,024
editor limit cases, retaining malformed/truncated-input rejection checks.

### Original reward arithmetic and Options-consumer audit

Starting money already initializes session cash via new-game setup. No
production finish-position, per-track or fastest-lap reward consumer existed
at this audit. `race_rewards.h` now ports the original arithmetic slices,
but is deliberately not yet called by production until finish-ranking and
the post-race ordering are connected faithfully.

`make verify-race-rewards` executes original runtime bytes, with no mocked
calls inside either tested slice. 28,672 finish-reward cases (22cf1..22d1b)
and 9,604 post-track cases (2557e..255ff) match all resulting cash/point words.
Coverage includes every signed point byte/multiplier byte, signed monetary
limits, word overflow, tied fastest laps, the 29999 sentinel and dword lap
times outside signed-word range. The original minimum accumulator is a
signed word even though lap times are dwords. All four slots get the track
payment; every exact tie gets the fastest-lap point/cash bonus.

The same oracle audits 245bb..245c8 for all 65,536 Change Car setting words:
the original computes 1 minus the setting byte, then unconditionally writes
2 into the intermission initial-selection local. Do not invent a setting
effect that this executable does not have. This only proves that consumer
slice, not the complete intermission UI.

The finish-ranking dependency is now connected and verified below. Supplying
the original rank/table/multiplier to the reward helper, connecting track
rewards, cumulative points/cash and original intermission/shop consumers
remain open. This audit is not evidence that cash/options are fully
functional in a native championship yet.

### Original finish ranks connected to the native race

`finish_rank.h` ports 22b89..22bd6 (four-pass rank assignment) and
22bfd..22c4d (winner-branch lap adjustment). The original-code oracle passes
40,000 rank assignments and 40,960 lap adjustments, including signed byte
wrap, signed lap words, ties and inactive slots. The scan includes all four
slots; the native runtime counts only active drivers as finished.

The runtime now retains the original signed rank bytes instead of assigning
positions using an incrementing finish counter. Its regression fixture
finishes drivers in chronological order 0,1,2 but correctly assigns ranks
1,3,2 after the winner's lap adjustment. Inactive-driver accounting, all
65,536 lap-limit values, Arcade countdown/deadline and drive-physics
regressions pass. The Amiga executable rebuild and strlen leaf guard pass.

Muted A1200/2 MiB port 25066, OPTIONSB and `diag_finish_ranks.gdb` reach
the real results boundary without forcing a finish. The native result is
NATIVE_ORIGINAL_FINISH_RANKS_RACE_END_OK: signed ranks -2,3,1,2, three
finished drivers, game clock 2563. The debugger checks the completed race,
deadline, positions and finished count against the retained rank bytes.
It detaches at results; this run is neither a system-restoration check nor
an independent end-to-end original-DOS race comparison.

### Independent controller preparation and capture pixels

The controller pixel test previously copied the native prepared surface into
the DOS oracle before testing redraw. That was valid redraw coverage, but
could not prove the preparation itself. `verify_list_pixels.c` now starts
both paths with the same unmodified asset-backed background and executes
original 2d8e3..2da3b for font-colour changes, tinting, five action icons and
four driver labels. Only action-resource loading/freeing is substituted;
the original draws the actual RGB icon resources. This entire prepared
64,000-byte screen and both font colours match the native renderer.

After the existing 120 sequential redraw comparisons, the test executes
original 2dece..2df10 for each of the 20 key fields and compares the complete
screen and small-font colour with the native capture prompt. Every case
passes, and native close restores the original pixels and both font colours.
Each tested original slice must reach its exact stop address; instruction
budget exhaustion cannot count as a passing comparison. The optional
SLICKS_CONTROLLERS_ONLY environment switch runs this focused gate, while
the default full dialog regression retains it alongside all other dialogs.

The full dialog regression passes: 276 name, 72 message, 104 colour,
324 Options and 120 list comparisons, plus the controller cases above.
Controller regressions also pass 2,640 navigation/device/default/exit,
10,240 capture, 120 draw-command, 1,360 capture/configuration/driver-key,
8,064 original device-polling and 256 Amiga register-decoding cases.

This closes static preparation/capture pixel coverage. It does not prove
pressed physical/emulated joystick input reaches a moving car; that native
integration check remains open.

### Joystick menu handoff and unavailable Lua injection

OPTIONSQ (`SLICKS_OPTIONS_MENU=10`) reloads the isolated unique-profile
save, enters Custom/Controllers, changes driver 3 to joystick 1 and starts
GO using queued native menu keys. Input batches stay within the platform's
16-byte queue. Muted A1200 port 25067 reached the racing device callback
with participation 1,1,-1,0 and devices 0,0,1,0; the saved ABC human remains
in slot 3. No driver-selection/configuration state was written by GDB.

The proposed emulator-side Lua event fixture did not run: this installed
FS-UAE accepts uae_lua in configuration but has no Lua runtime entry points
or script-load activity. Upstream v3.1.66's fs_uae_send_input_event function
is also a no-op; diag_joystick.lua instead uses the input-event dispatch
behind uae_write_config, which requires a Lua-enabled emulator. Do not
claim joystick press/steer/release coverage from this run. It was stopped
after inspecting the native handoff, not marked passing. The native build,
strlen guard, shell syntax and diff whitespace checks pass.

Next gate needs a supported external event source (for example FS-UAE's
keyboard-to-joystick mapping with targeted host key events), or a verified
Lua-enabled debug emulator. The optional Lua harness is not used by normal
run.sh and does not write guest input state or hardware registers directly.

The alternative host-key fixture is now available under
SLICKS_DEBUG_JOYSTICK_KEYS=1: F6 maps to up, F7 to right and F8 to fire on
the joystick socket. FS-UAE calls these action_joy_1 events even though
its UAE core names them JOY2. This option affects debug.sh only.
`tools/fsuae_key_event.c` compiles on macOS and restricts event posting to
an explicit PID whose executable basename is fs-uae. Its permission check
reported event posting unauthorized; it sent no event and did not request
or change accessibility permissions. A manual press/steer/release run is
therefore still needed with this installed environment. Do not count the
helper or menu-handoff result as proof that a car responds to joystick input.

### Original Help refresh orchestration

`make verify-help-refresh` compares the actual production
slicks_help_viewer_refresh function with original 32a35..32abc. All 3,024
cases pass, covering changed/unchanged chapter, page and selection; negative,
zero and positive redraw flags; signed link-mode bytes; signed selection
limits; and one, two or seven links. Comparisons include chapter-load/page-
draw ordering and arguments, cached chapter/page/selection, corrected
selection, next-page state and pending redraw flag. Each original execution
must reach its exact stop address.

This oracle substitutes chapter loading and page rendering at matching
boundaries on both sides; it does not replace the refresh decisions under
test. It compares the original's pending corrective redraw without a new
key with the native immediate corrective pass. Arrow rendering, whole-screen
pixels and chapter preprocessing remain separate component gates. This
proves refresh orchestration, not the entire viewer entry/exit sequence.

Related regressions pass: 816 navigation/history cases, 64 topic lookups,
11 byte-exact indexes, 52 chapters, 79 preprocessing lines and 4,728 line
drawing-order/formatting cases. No production behavior change was needed
for this refresh comparison.

### Original Help topic-entry composition and close font

The full `verify-help-pixels` gate now drives the production viewer open,
not only isolated pages. Sixteen cases cover empty, Options, Players, Tracks,
registration, missing, language-prefixed and main topics with country 0/358.
Original 3284e..328c0 selects palette colours; 329bc..32a32 draws headers
before resolving the requested topic; 32a35..32b5d performs chapter/page
refresh and arrows. All 64,000 pixels, complete font state, chapter/page,
selected link, link count and meaningful target text match. Original
32ed3..32ee7 also restores the same saved font state as native close; native
close separately must recover every original background pixel.

The oracle preloads header/index resources and substitutes chapter loading
using helpers already compared against original builder/reader instructions.
It therefore proves viewer composition/control flow, not a second independent
DOS file-system/allocator implementation. The original snapshot-window restore
is not executed by this new gate; native underlying-screen restoration and
resource release remain covered by the existing A1200 menu-context tests.
Link targets are compared as bounded NUL-terminated strings: DOS retains
irrelevant trailing bytes after shorter targets across viewer entries, while
the fresh native allocation clears them.

The full regression passes 1,166 line pixel/font cases, 98 complete page
pixel/font/link cases, these 16 composed entry cases, 3,024 refresh cases
and 816 navigation cases. No production renderer change was needed. The
SLICKS_HELP_ENTRY_ONLY switch allows focused entry diagnosis, but the normal
verify-help-pixels target includes the new comparisons unconditionally.

### Records modal allocation/resource recovery on A1200

TRACKSK (`SLICKS_TRACK_MENU=8`) opens the records dialog through Tracks and
injects five one-shot failures: dialog allocation, preview-arena allocation,
missing palette resource, icon scratch-buffer allocation and a missing sixth
icon after five icons have loaded. Each allocation fault returns null only
at that boundary; the resource faults perform an actual failed archive lookup.
Normal runs leave the diagnostic selector zero.

Muted A1200/2 MiB port 25068 with `diag_track_info_faults.gdb` passes
TRACK_INFO_FIVE_FAULTS_DISMISS_RETRY_REOPEN_RACE_OK. All five warnings are
dismissed using native input, preserving both font colours, track cursor and
playlist count. Every post-dismissal 64,000-byte screen compares exactly with
the pre-failure menu. The same process then opens/closes the real animated
records dialog twice and enters a race without a reported race error.
This checks these explicit boundary failures, not physical RAM exhaustion,
every upstream file allocation or system restoration after race entry.

Related original regressions pass: 12 full-screen records-panel/font cases,
195 supplied-track description/object-call cases plus 16 synthetic cases,
and all 194 supported preview compositions. RAILROAD's five out-of-bank
selectors remain explicitly rejected. Inspection confirms the original
resource loader 3528a rejects indices outside its initialized resource count,
but the preview caller ignores that status and still attempts drawing its
allocated scratch buffer. Its actual initialized resource range/content is
still required before choosing faithful handling; do not mask the selectors.

### Large saved-list boundaries: original overflow confirmed

The original list oracle now covers counts through 2,849, including 101,
127/128, 255/256, 512, 1,000 and 1,560. All 28,224 initialization,
2,520 drawing-call/font, 1,300 pulse and 172,032 key/scroll/result comparisons
pass. The generic dialog and renderer permit the packed-result range of
0..4,095; the Amiga saved-list adapter still deliberately limits storage to
100 entries.

The expanded composed-pixel gate passes 390 full-screen/font comparisons,
including navigation to the ends of large catalogues and close. However,
matching DOS is not sufficient to promise clean restoration: the original
signed 16-bit scrollbar product wraps and can paint above its saved window.
A separate native restoration assertion exposes this at count 512, top 46:
pixel (304,10) remains changed after closing. The composed DOS/native images
match after close, including these original out-of-window marks. The test now
records this behavior explicitly.

A count-sized title-allocation implementation was built, then withdrawn
before target testing because exposing those large lists would expose this
corruption. Removing the production limit remains open; it requires safe
save-under coverage (and memory/failure/reopen tests), while retaining the
verified original scrollbar arithmetic. Existing small-list lifetime tests
remain the clean-restoration contract. No large-list A1200 pass is claimed.

The rebuilt standard executable also passes the muted A1200/2 MiB Options
Help navigation/history/reopen/exit gate on port 25069:
HELP_NATIVE_MENU_NAVIGATION_REOPEN_RESTORE_OK, five ready checkpoints,
two closes, restore status 0x1f. Its before/after 64,000-byte menu images
compare equal. The strlen leaf build guard and all 16 small-list renderer
lifetime/restoration cases pass; the original startup Guru remains fixed.

### Large saved-list storage connected

The Amiga picker now owns a count-sized title allocation instead of a fixed
100-entry array. It copies validated catalogue records in one pass and frees
the names on cancellation, selection, open failure and menu destruction.
The 64 KiB file bound remains: at most 2,849 empty records, or 2,848 records
with two track names in the last record. Allocation failures are reported as
I/O/no-free-store errors instead of malformed files.

The original signed scrollbar calculations and all open/navigation pixels
remain unchanged. A platform save-under stores its four screen columns in
800 bytes before opening and restores them after normal dialog close. This
deliberate cleanup differs from the original's leaked off-window pixels;
it does not alter the original scrolling or hide a rendering mismatch.
All 88 host lifetime cases through 2,849 entries now restore the complete
screen and font, including the previously failing count-512 case.

Muted A1200/2 MiB port 25070 passes
LARGE_LIST_CANCEL_REOPEN_END_LOAD_RACE_OK using a synthetic 65,528-byte,
2,848-entry SLICKS.TRK. Actual menu input reaches row 2,847, cancels, reopens,
reaches the last row again, loads its 1WAY/BASIC playlist and starts the race.
All 64,000 pixels before opening and after cancellation compare equal.
The fixture generator refuses to overwrite existing files; reference assets
were not modified. This supersedes the retained-100-entry-limit notes above.

Reproduce with `make build/make_large_track_lists`, generate SLICKS.TRK in
an empty isolated run's dh1 directory, and run debug.sh with
SLICKS_TRACK_MENU=9 and diag_track_lists_large.gdb. The supplied GDB script
stores captures under .run/track-lists-large-v1; match its run directory.

TRACKSN (`SLICKS_TRACK_MENU=10`) arms two one-shot faults in the native
diagnostic input sequence: picker allocation, then title allocation. Normal
runs keep the fault selector zero. Muted A1200/2 MiB port 25073 with
diag_track_lists_alloc.gdb passes
LIST_TWO_ALLOCATION_FAILURES_DISMISS_RETRY_RACE_OK. Both failures report
SLICKS_SETUP_LOAD_IO_ERROR / ERROR_NO_FREE_STORE (103), clear picker/catalogue
ownership, allow warning dismissal with exact 64,000-byte menu restoration,
then permit loading the last list and entering the race. This checks explicit
allocation-return boundaries, not physical exhaustion of all RAM. Earlier
debugger-armed runs did not consume faults and failed the gate; they are not
failure-handling evidence.

Related original regressions pass: 28,224 list initialization cases, 2,520
draw/font cases, 1,300 pulse cases, 172,032 key/scroll/result cases, 60 saved
playlist-reader cases, 80 whole-file writer cases and 648 Lists caller cases.

The same final build passes the existing native delete lifecycle on muted
A1200/2 MiB port 25074: TRACK_LIST_NATIVE_CANCEL_NAME_CANCEL_DELETE_CONFIRM_REOPEN_OK.
A separate one-entry synthetic catalogue is retained after cancelled delete
and cancelled name entry, deleted after confirmation, then reopened empty
before entering the race. The resulting SLICKS.TRK is the eight-byte empty
catalogue. No supplied or user configuration was used as the deletion target.

### Finish-position cash and points connected

Native checkpoint finishes now call the setup-session award helper once at
the original finish-event boundary, using the stored rank after lap-based
adjustment. It reads the session's entrant count and resolved money-per-position
option, and the original executable's four rank-point bytes exported by the
local-only defaults generator. Awards update session cash and cumulative
points directly; entering/drawing the results display does not pay them again.
Legacy standalone race diagnostics without a setup session have no callback.

The reward oracle now executes the larger original 22cda..22d1b range,
including count-minus-rank calculation and table lookup. All 28,672 composed
session awards pass, alongside the existing 9,604 post-track arithmetic cases,
40,000 rank assignments and 40,960 lap adjustments. New-game points reset
matches original 2a2ce..2a2dd's four-word clear; all 3,840 setup transitions
also check that selection refresh after a race preserves accumulated points.
The clear is a separately executed original slice, not proof of the full
unported championship/intermission orchestration.

Muted A1200/2 MiB port 25075, OPTIONSB and diag_finish_rewards.gdb passed
NATIVE_FINISH_REWARD_EVENTS_SESSION_CASH_POINTS_OK. Three real finish events
credited rank 1/2/3 with cash 260/240/220 and points 3/2/1, each exactly once.
The gate checks initial balances/zero points, each callback's resulting words,
and finish-rank coverage when the race ends. No race state is forced by GDB.
The final build, explicitly passing the stored post-adjustment rank rather
than the pre-adjustment return value, repeats the same gate successfully on
port 25076 with the same three awards. The runtime leaf guard and whitespace
checks pass; generated point bytes remain ignored/local-only.

Per-track and tied-fastest bonuses remain arithmetic helpers only. Original
standings/shop, post-race award/abort ordering, inter-track vehicle selection
and the final complete setup audit remain open; this is not a completed
championship-money implementation.

### Post-track awards and frozen completion connected

The original caller returns from race loop 1fb4a to 254d2, frees race-owned
resources, then reaches 25552. Only nonzero demo byte DS:0459 skips the award
loop; an early race-loop exit does not skip it. Normal native setup races now
bind a post-track callback to the session. It pays the configured base amount
to all four slots and applies the original tied-fastest calculation and
point byte before results/selection refresh. It is guarded once per race;
leaving a completed results screen cannot pay again. Native Escape on an
unfinished race also reaches this guard. Standalone legacy race diagnostics
without a setup session have no award callback.

Inactive drivers previously kept zero best-lap time because native startup
skipped their initialization. Original 1c111..1c241 initializes all four to
30000 regardless of participation. Native startup now does likewise, and
all 81 combinations of human/computer/inactive original bytes confirm the
sentinel. This prevents inactive drivers from winning a zero-time bonus.

Completed native races now stop advancing physics, lap clocks and finish
events while results are displayed. Subsequent steps clear stale sound events
but otherwise preserve the race. The host runtime gate checks a once-only
award and 100 further steps against the complete frozen state. Original
post-track comparisons now execute 25552..255ff through the session wrapper:
all 9,604 cash/point cases pass, including tied minima and signed truncation.

Muted A1200/2 MiB port 25077, OPTIONSB and diag_track_rewards.gdb pass
NATIVE_TWO_TRACK_REWARDS_ONCE_PRESERVED_RETURN_OK. First-track balances are
cash 250/270/330/310 and points 0/1/4/3, including tied fastest laps. The
second track preserves these on entry, then finishes at cash 300/400/420/380
and points 0/5/6/4. Exactly one post-track callback occurs per race, with
matching configured amounts and actual lap minima, before returning to title.

The original standings/shop screens, early-exit confirmation UI, next-track
load-error recovery and inter-track vehicle-choice consumers remain open.
Connecting these awards does not stand in for those screens or the full
configure/save/restart/race completion audit.

The final build also passes the muted early-exit gate on port 25079:
NATIVE_EARLY_EXIT_TRACK_PAYMENT_INACTIVE_SENTINEL_RETURN_OK. SETUPA
(`SLICKS_SETUP_ABORT=1`) loads a separate copy of the saved mixed setup,
presses GO and Escape during the starting countdown, and returns to title.
The gate explicitly checks roles 1,1,-1,0, all four 30000 best-lap sentinels,
one base payment to each slot, no fastest-lap points, and no completion flag.
Port 25078 passed the earlier version checking the inactive slot only;
25079 adds the complete role assertion. No saved files were changed.

Final runtime regressions pass: 65,536 lap-limit values, six modes and 1,800
countdown frames, finish suppression/expiry, original lapped rank order,
100 frozen result steps and 3,840 setup transitions. The Amiga build,
strlen leaf guard, shell syntax and whitespace checks pass.

### Vehicle-shortcut-only save/restart regression

PLAYERSZ (`SLICKS_PLAYER_MENU=17`) loads a copy of the unique-profile fixture,
opens Players, moves to driver slot 2, presses C once and exits. It never
opens a picker/editor or changes a selection. The first muted A1200 run on
port 25081 reproduced VEHICLE_SHORTCUT_SAVE_MISSING: the profile vehicle
changed, but save-on-exit was skipped and restoration still returned 0x1f.
The original player-menu key helper deliberately has no dirty-byte update
for A/C. Its DOS differential tests remain unchanged; the native persistence
adapter now notices a changed vehicle byte and sets its own setup-dirty flag.

Port 25082 passes VEHICLE_SHORTCUT_ONLY_SAVE_RESTORE_OK with ABC vehicle 0
changed to 1. A fresh process on port 25083 uses SETUPR and passes
VEHICLE_SHORTCUT_FRESH_RELOAD_RACE_OK. The selected profiles remain 1,1,3,0,
roles remain computer/computer/human/inactive, all active race vehicles match
the prepared selection, and all six colour bytes per active driver match
their selected profiles. Saved/reloaded ABC records compare byte-for-byte.
The CFG is unchanged; the PLR differs from the seed at byte 54 only (0 to 1).
Test disk: `.run/vehicle-shortcut-v1`, scripts `diag_vehicle_shortcut_save.gdb`
and `diag_vehicle_shortcut_reload.gdb`. Tests use ordinary queued menu input
and read-only debugger assertions, not injected profile/race state.

The build and strlen guard pass. Host regressions pass: 98,304 profile-menu
assignment/navigation cases, 8,032 original nonmodal key cases, 96 modal
dispatch cases, 512 menu and 864 editor drawing cases, 276,480 profile setup
cases, 254 load-adapter cases and 3,840 composed setup transitions. This is a
specific persistence fix, not completion of the remaining setup-screen,
controller and full end-to-end audit work.

### RAILROAD resource-failure investigation

The existing original-DOS `slicks-race-data.bin` captures resolve the missing
resource bounds: DS:1706=886 total entries, DS:53ac=6 first scenery entry,
DS:53b0=110 scenery types. All eight available race-data captures agree.
The capture patch explicitly copies 0x7000 bytes starting at SegPhys(ds).
These are initialized race snapshots, not a new capture at the Tracks menu.

`make verify-track-preview-failure` reads the local captured DS and supplied
RAILROAD.SS, then executes original 1a1d4, the real 3528a resource lookup and
35719 scaler. RAILROAD objects 5/7/8 request resource 2011, object 9 requests
3001 and object 10 requests 3443. All exceed 886. The real resource routine
returns error 1 without writing the destination; its caller ignores that
return and still draws and frees the allocation.

Ten cases (five real invalid selectors times two synthetic valid scratch
contents) prove that the unchanged allocation determines the drawn colours.
Only allocation/free and final pixel output are substituted; lookup bounds,
error return, caller control flow and scaling are original instructions.
This does not reproduce DOS heap contents or claim an authentic RAILROAD
preview image. Masking bank numbers would invent different behavior, and
there is no valid resource to port for these captured bounds. Native rejection
and recoverable warning remain intact. All 194 supported preview compositions
still match the original's 64,000 pixels; no runtime rendering change was made.

### Records-modal surround composition

`track_info_renderer.h` now contains the original 2682c..269e5 composition:
records-panel tint and drawing, heading tint/text, optional description and
preview surround. The live Amiga dialog uses this helper, replacing its
private copy of the same sequence. Background save/restore, resource loading,
preview rendering, animation and ownership stay with the existing adapter.

The expanded `verify-track-records-pixels` runs those original instructions
through the actual DOS tint, records, font and VGA routines, comparing every
screen pixel and both complete font resources with the production helper
using native 68020 text/icons. Twelve cases cover short/long names, present
and absent descriptions, empty/populated records and percentages 0/50/100.
String-table lookup and description loading supply fixture strings at their
already-tested boundaries; this gate stops before preview drawing, whose
194-track full-screen original comparison remains separate. All twelve
compositions and the existing twelve records-panel cases pass.

Muted A1200 port 25084 passes TRACK_INFO_NATIVE_OPEN_ANIMATE_CLOSE_REOPEN_RACE_OK
with the standard rebuilt binary: two opens, animation, two closes, then race.
The saved parent and restored 64,000-byte surface compare identical; fonts,
cursor and playlist count are checked by the gate. The build's strlen leaf
guard passes. Port 25085 passes
TRACK_INFO_FIVE_FAULTS_DISMISS_RETRY_REOPEN_RACE_OK: dialog allocation,
preview-arena allocation, palette load, icon scratch allocation and icon load
failures all dismiss, followed by two successful opens/closes and race entry.
All five restored parent surfaces compare byte-for-byte with the saved parent.

### Saved random-once versus random-per-race vehicles

PLAYERSB (`SLICKS_PLAYER_MENU=18`) loads a copy of the menu-created unique
profile fixture, moves to slot 2 and presses the normal C shortcut ten times.
ABC's fixed vehicle 0 becomes random-once value 10. It then exits and saves
without opening a picker/editor. Port 25086 passes
RANDOM_ONCE_PROFILE_NATIVE_SAVE_OK and restoration 0x1f. The seed CFG remains
identical; the PLR differs only at byte 54, from 0 to 10. Profiles remain
1,1,3,0 (shared computers, unique human, inactive), with built-in computer
vehicle choice 11 (random each race).

A fresh SETUPR process on the same isolated `.run/random-vehicle-v1` disk,
port 25087, passes
RANDOM_ONCE_RETAINED_RANDOM_EACH_REROLLED_NATIVE_RELOAD_RACE_OK.
Read-only debugger checks independently calculate weighted choices from the
original table and RNG recurrence. All three startup choices match the fixed
diagnostic seed 0x1234, including each callback's driver, vehicle and RNG state.
At GO, the two random-per-race computers consume exactly two further draws
from the then-current stream; the random-once human keeps its startup vehicle.
All three vehicles reach the actual race with the correct participation,
and slot 3 stays inactive. The profile keeps choice 10, not the concrete car.

This verifies fresh startup and new-game entry, not a new two-track run or
shop/vehicle-change dialog. Existing original-code session tests cover the
post-race selection rule separately. The production selection algorithm was
not changed; only the diagnostic input mode and assertions were added.
Amiga build/strlen guard, shell syntax and diff whitespace checks pass.

### Recoverable GO preparation failure

The initial GO caller previously sent every preparation error to program
cleanup with return code 20. `prepare_race` also changed inventory, cash,
points, random choices and possibly selected profiles before discovering a
missing asset or unsupported controller. Preparation now backs up its setup
session/configuration/request state and restores it on failure. Static
backups avoid growing the small task stack; preparation is synchronous and
non-reentrant. Each attempt resets the previous diagnostic error.

The interactive GO path shows a native platform-error screen, restores the
title on Return/Escape and leaves native Players/Options/Tracks available.
It distinguishes memory, setup/controller and game/track-file categories.
This is deliberately not claimed as an original DOS dialog. Corrected assets
or settings can be retried without losing the setup. Fatal platform failures
and the separate next-track caller still follow their existing paths; this
change does not implement championship retry or the original abort prompt.

Muted A1200 tests on `.run/race-load-failure-v1`:

- Port 25088, SETUPF (`SLICKS_SETUP_FAILURE=1`), performs an actual missing
  track-file read after new-game preparation has modified session state.
- Port 25089, SETUPG (`SLICKS_SETUP_FAILURE=2`), fails an actual archive lookup
  for the HUD after scene/mask construction has overwritten rendering state.

Both pass NATIVE_GO_LOAD_FAILURE_DISMISS_PLAYERS_RETRY_RACE_OK: show warning,
dismiss with Escape, open/close the real Players menu, retry GO and enter race.
Complete session and configuration dumps before/after failure compare equal,
including RNG, cash, points and inventory. No debugger game-state writes are
used. The saved CFG/PLR remain identical to their seed copies. These are
one-shot diagnostic boundary faults, default off, not deleted reference files.
The final Amiga build/strlen guard, shell syntax and whitespace checks pass.

### Next-track load failure preserves the current game

The next-track caller no longer exits the program on preparation failure.
Its platform-error screen offers Return to retry preparation and Escape to
return to the title. Retry does not reenter the completed-race branch: track
awards, inventory transfer, selection refresh and playlist advancement happen
only once. It calls preparation with `new_game=0`, retaining the session and
continuing the same game. The loading boundary now marks the race inactive
before rebuilding its storage, so an error is not reported as a live race.
The shared error-screen helper also retains the initial-GO recovery path.

OPTIONSU (`SLICKS_OPTIONS_MENU=11`) uses the normal native Options/Tracks input
sequence to configure Arcade/5 seconds and BASIC/BASICTRK. The first race
finishes normally; the diagnostic makes the next HUD archive lookup fail
after scene construction, then queues ordinary Return input to retry. No
race completion, rewards, game state or session values are forced.

Muted A1200 port 25090, `.run/next-track-failure-v1`, passes
NATIVE_NEXT_TRACK_FAILURE_RETRY_TWO_RACES_REWARDS_ONCE_OK: three preparation
calls, two actual race starts, one recoverable failure, exactly two track
award events and final return to title. The entire 188-byte session compares
identical before the failed next-track attempt, after failure, before retry
and at successful race start. This includes cash, points, inventory, driver
selections, vehicles, colours and RNG. The Escape-to-title branch is present
but this run exercises Return-to-retry, not cancellation.

Original setup-session regressions pass all 3,840 transitions. Race runtime
regressions pass 65,536 lap-limit values, six modes/1,800 countdown frames,
finish suppression, lapped ranks and the 100-step completed-race freeze with
one track award. The Amiga build/strlen guard and whitespace checks pass.

### Original in-race menu identified; dispatcher translated

The original race caller at 2401c does not ask a Yes/No abort question.
It invokes 1e472 with six rows: Back, Help, Controllers, Speed, Next Track,
Main Menu (DS:079a's string-key table). F9 selects row 4 on entry; F10 selects
row 5. Other entry keys start on Back. While the menu is open Escape returns
1 (resume), F9 returns -1 (next track), and F10 returns -2 (main menu).
Enter/Control/Space and decoded joystick button 108 select the current row;
Up/Down and joystick values 100/101 navigate. Help and Controllers return to
the same menu. Speed redraws it after returning from its dialog.

`race_menu.h` translates original 1e664..1e718 without a CPU-state layer.
`make verify-race-menu` executes those original instructions, substituting
only the Help, Controllers, Speed and save-under restore call boundaries.
All 17,920 key/row/count combinations match row, signed redraw/result and
modal action, including signed-byte boundary rows/counts. The test checks
the original empty Help topic and Controllers coordinates (45,65).

This is a dispatcher foundation only. Original menu drawing/save-under,
Speed dialog, pause timing/audio, caller result handling and native integration
are not yet complete. The existing immediate-Escape production path has not
been changed, and no partial menu is advertised as a completed replacement.

### Original Speed dialog controls and full pixels

`speed_dialog.h` translates the dialog called by the in-race menu at 1e00a.
It edits configuration field_05de immediately; Escape, Return, Space and F1
all accept/close rather than cancelling. Left/Right change by one, Up/Down by
five, Home/End reset to 100. Every event performs the original signed-word
wrap then clamp to 50..200. Entry marks configuration dirty even without an
edit (the integration caller must apply this). The original initial displayed
value is zero, so an initially zero speed does not draw a number until changed.

`make verify-speed-dialog` passes 786,432 original-instruction comparisons:
all 65,536 speed words and twelve meaningful/ignored key bytes, including
overflow, clamping and close flags. `verify-track-records-pixels` now also
executes the entire original 1e00a dialog. Twelve eight-key sequences compare
all 64,000 pixels at every keyboard boundary (96 frames), including out-of-range
initial speeds, every close key, the percent label, numeric repaint and both
palette changes. The native side uses the production 68020 font bridge.
Close restores the full font exactly, and original dirty state and timer-call
argument match. Existing twelve records panels and twelve info surrounds pass.

The raw close argument is wrapped speed*5, passed to original 37bc2. That
routine calculates a timer divisor from 100*65536 divided by its argument;
the argument must not be treated as a frequency in hertz. Native configuration
already serializes field_05de, but its effect on runtime timing is not connected.
This turn does not expose an ineffective Speed control: in-race menu rendering,
live dialog lifecycle, dirty propagation and clock integration remain open.
The Amiga build/strlen guard and 17,920 race-menu dispatcher comparisons pass.

### Original pause-menu background and selection renderer

`race_menu_renderer.h` translates the font-colour/tint/save-under setup at
1e481..1e53f and selective row redraw at 1e54f..1e65c. The saved rectangle
contains the already tinted menu background, not the original race page.
Selection movement restores the previous/current row strips at source
y=row*10+8, then draws the original rounded bevel and centred label. The
existing saved-rectangle helper correctly adds that source offset to the
destination origin. Close restores the previous font colour; the caller
must separately restore the race page. Storage is caller-owned and checked
before changing the screen; native integration must handle allocation failure.

`make verify-track-records-pixels` passes 18 original openings (six initial
rows, tint percentages 0/50/100) and 270 full-screen redraw comparisons,
including repeated movement and end stops. The DOS side executes the real
capture, crop restore, bevel, tint and font routines; the native side uses
the production 68020 font bridge. Only allocation, keyboard flush and label
lookup are substituted. Labels are supplied test translations, so this gate
does not verify localization loading. The harness now supports the separate
VGA read-plane index/data writes used by the original capture routine.
Existing records/info/Speed pixel gates also pass.

The renderer is not connected to the live race yet. Original label resolution,
page lifecycle, Help/Controllers/Speed orchestration, pause audio/timing and
caller result handling remain open. In particular, existing immediate Escape
behavior is not claimed to implement the original menu.

### Original pause labels and language-table lookup

The six pause-menu DS strings are lookup keys, not display labels. English
`lang1.txt` supplies `< BACK TO GAME`, `HELP`, `CONTROLLERS`, `GAME SPEED`,
`NEXT TRACK >`, and `MAIN MENU >>`. The setup-default exporter now emits the
six original key pointers as local-only typed strings for future integration.

`language_table.h` ports original 3601a text-table construction and
36182/361e9/36227 lookup. It preserves the original case-sensitive first-prefix
match (not an invented exact-key dictionary), duplicate ordering, empty values,
AF-to-linefeed conversion, dot termination and key fallback. The native loader
checks storage capacity in a first pass; failure leaves output and length
unchanged. Source and destination storage must be distinct.

`make verify-language-table` executes the original loader with only archive
open/read/close and allocation boundaries supplied. All eight real resources
and a duplicate/empty/multiline/terminator fixture match byte-for-byte. There
are 162 original lookup comparisons and 324 real fallback-wrapper comparisons
(table present and absent), plus atomic insufficient-capacity checks.

The pause renderer pixel gate no longer substitutes label resolution. It loads
each real language table and executes original 36227; 24 openings and 360
full-screen redraw comparisons pass across all eight languages, six initial
selection rows and 0/50/100 tint. Existing records, track-info and Speed pixel
gates also pass. This supersedes the earlier supplied-translation limitation.

The standard Amiga build and strlen leaf guard pass. The live race still needs
menu lifecycle wiring, submenu orchestration and timing/audio integration;
neither this component work nor the build proves that live pause/resume works.

### Native pause-menu owner, rendering and recovery gate

The Amiga adapter now owns the translated pause menu, tinted background,
loaded language table and resolved labels. It uses a dedicated shared menu
surface; that surface's existing 64,000-byte saved buffer preserves the race
page. Open loads and validates resources before changing pixels. Close restores
the original font colour and complete race surface, reports dirty rows, and
releases the modal allocation. Calls that allocate or free require AmigaOS
available; redraw and the original key helper do not call the OS.

`UIMENU` / `SLICKS_PAUSE_SURFACE=1` is explicitly a component diagnostic, not
the finished interactive pause loop. It prepares a real track scene, injects
three one-shot failures (allocation, missing language, rollback after draw),
checks exact pixel/font restoration after each, then opens and traverses the
six rows down/up in two separate opens. It displays through the real platform
and dirty-row C2P, closes twice, and restores system state.

Muted A1200/68020/2 MiB/no-fast-RAM runs on ports 25091 and 25092 in
`.run/pause-surface-v1` pass `NATIVE_PAUSE_SURFACE_REOPEN_RESTORE_FAILURES_OK`:
19 checkpoints, three recovered failures, two closes and restore status 0x1f.
Before/after race surfaces compare byte-for-byte. Menu and restored-scene
bitplane dumps each match all 64,000 authoritative chunky pixels. The rendered
native menu was visually inspected; the original English labels are present.
The Amiga build/strlen guard, 17,920 original navigation cases and language
loader/lookup regression also pass. The diagnostic emulator was stopped.

Normal race Escape still follows the existing exit path. This does not yet
connect Help/Controllers/Speed, preserve elapsed-time/audio state across a live
pause, or implement the original menu caller's resume/skip/end handling.

### Native pause-menu children and original Controllers position

The dedicated race menu surface now loads both original fonts and supplies
the text/icon callbacks required by Controllers. Its new coordinate-taking
adapter opens the pause-menu Controllers at original (45,65); the existing
Options wrapper still uses (100,80). No shared Options coordinates were changed.
The Speed adapter owns its active state, updates field_05de with the translated
keys, sets the supplied dirty flag on entry and accepts Escape. Close returns
the original wrapped speed*5 timer argument, restores the tinted parent bitmap
and redraws all pause rows. This returned argument is not yet applied to the
running race clock. Pause redraw/close reject active children.

The native UIMENU gate now dispatches Enter on Help, Controllers and Speed
through the original pause key helper and exercises their real Amiga adapters.
Help opens its empty/default topic and exits. Controllers changes player one's
device then exits. Speed edits 100 to 106 and accepts Escape, retaining 106
and returning timer argument 530. Every child close compares all 64,000 parent
pixels and both font colours with its pre-open state. It repeats the entire
sequence on a second parent open and finally restores the race scene.

The first run (25093) exposed a diagnostic error: multiple navigation keys
were dispatched without their intervening redraws, leaving a stale highlight
that Speed's full repaint removed. Correcting the driver to follow the
original draw-after-each-key sequence yields the passing run on port 25094:
19 parent checkpoints, 16 child checkpoints, three recovered parent failures,
two final closes, exact scene restoration and system restore status 0x1f.
Help, Controllers and Speed displayed bitplanes each match all 64,000 chunky
pixels. Controllers and Speed target renders were inspected. The emulator
was stopped; no test profile/configuration files were saved.

`SLICKS_CONTROLLERS_ONLY=1 build/verify_list_pixels` now compares both original
positions, including unaligned x=45: each passes preparation, 120 sequential
full-screen/font redraws, 20 key-capture prompts and close restoration. The
Amiga executable rebuild and strlen guard pass.

This is still a component diagnostic, not a completed interactive race pause.
Normal input dispatch, persistent dirty propagation from live edits, timer and
audio pause/resume, caller skip/end handling, and nested failure UI remain to
be connected and verified before claiming the in-race menu complete.

### Configured game speed now drives the race timer

Before connecting live pause input, the Speed consumer needed correcting:
`next_physics_ticks` still hard-coded the 100% PIT divisor. `race_timing.h`
now translates original 37bc2: signed argument >=100 programs
floor(6553600/argument); smaller/negative words disable the custom IRQ clock
and program a count of 65536. The Speed argument is the wrapped word speed*5,
not a frequency in hertz. `prepare_race` now applies configuration.field_05de
through the public runtime timer setter after initialization.

The per-update clock retains fractional PIT input cycles at nominal PAL 50 Hz,
using a configured divisor*50 threshold. The default 100% sequence is unchanged.
Division happens only at speed configuration, not in the frame loop. Reprogram
resets the fractional phase but preserves accumulated race/lap time, mirroring
the original caller's new-delta boundary rather than resetting the race.
This does not change the existing nominal-update scheduling policy or claim
wall-time-perfect pacing on slow frames.

`make verify-race-timing` executes original 37bc2 for all 65,536 argument words,
checking both PIT data writes, stored divisor, custom timer enable state and
counter reset/preservation. Only interrupt-vector OS boundaries are supplied.
All 151 UI speeds then pass 453,000 exact-rational accumulator comparisons.
`verify-race-lap-limit` additionally executes the actual race step for 45,300
stationary-grid updates across every UI speed, checking race clock, driver
elapsed time, countdown and frame count. Reprogramming to 106% preserves elapsed
time and uses divisor 12365; disabling the timer freezes clock advancement.
Existing six-mode countdown, finish/ranking and completed-race checks pass.
The 786,432 original Speed key comparisons and Amiga build/strlen guard pass.

This removes the fixed-100% configuration consumer, but the native live pause
caller still needs to apply the returned Speed argument on resume. Ordinary
pause input, elapsed-menu-time exclusion and audio handling remain unfinished;
no new native timing or live pause/save/restart gate is claimed here.

### Live race pause input, nested edits and resume

Normal interactive races now route Escape, Control, F1/F2 and F9/F10 to the
original six-row menu. F9/F10 choose initial rows 4/5; Enter follows the original
dispatcher. Existing legacy diagnostics retain their old Escape behavior;
`LIVEMENU` / `SLICKS_PAUSE_LIVE=1` explicitly uses the production pause boundary.

The blocking boundary renders all menus in view 0, leaving the race bitmap in
view 1 untouched. No race step or status-clock update executes while paused.
Help, controller device/key editing and Speed use the real configuration;
entry to editable dialogs marks setup dirty. Speed close applies the verified
timer argument, and the live device configuration is refreshed before resume.
Status-clock vblank baseline is reset after the OS/resource transitions so menu
time is excluded. Queued entry/exit keys and drive latches are cleared to avoid
stuck controls; held-key parity with the original remains to be audited.

Audio is stopped in display blanking before handing resources back to the OS.
After final cleanup/takeover, the engine loop is restarted in blanking and its
pitch is updated from the unchanged car velocity. One-shots are not replayed.
This verifies safe engine resumption, not exact original muted-one-shot tail
behavior or sample-phase continuity. Those are still audio fidelity gaps.

Resume, skip and end are distinct return values. Skip reaches the existing
track-award/selection-refresh/next-track path even before natural completion;
main-menu return restores the title palette. These two exits are implemented
but have not yet passed their own native transition/reward-once gates. Initial
pause resource failure still follows fatal cleanup; nested Help and Controllers
have warnings, but their live fault paths remain unverified.

The initial live gate on ports 25095..25097 did not advance after injection.
Inspection proved vblank/interrupts healthy. This debugger build silently failed
target-memory writes: setting key_tail=0 and keys[0]=Down left tail=1 and the
old Escape byte unchanged. Do not use debugger writes as input or state evidence.
The diagnostic now enqueues ordinary keys on the target; GDB only observes.
The obsolete port-25096 processes were explicitly terminated after inspection.

Muted A1200/68020/2 MiB run on port 25098 in `.run/pause-live-v1` passes
`NATIVE_LIVE_PAUSE_CHILDREN_SPEED_RESUME_OK`. At actual race frame 100 it opens
the menu, visits Help, changes a controller device, edits Speed from 100 to 106,
and resumes. Thirteen input checkpoints preserve frame count, race ticks and
status clock while display vblanks advance. Full car structures and all 64,000
scene pixels compare byte-for-byte across the pause. At frame 150, the race has
advanced by the exact 96 ticks prescribed by the new divisor, engine playback
is active, the menu owner is released, and clean exit restores status 0x1f.
The final process inventory confirms none of these diagnostic sessions remains.

Still required: native skip/end and repeated-pause gates, live resource-failure
recovery, and save/exit/fresh-restart verification of these in-race controller
and Speed edits. This is not the full player-setup completion audit.

### 2026-09-24: initial pause-load recovery and retry

`run_race_pause` no longer terminates the game when opening the archive, creating
the menu surface, or opening the parent menu fails. The parent open is
transactional (including restoration after its post-draw failure boundary).
The caller frees resources with AmigaOS restored, resumes untouched view 1,
excludes loading time from the status clock, and restarts the engine in blanking.
Configuration and race input latches are not changed on an unsuccessful open.
Display takeover errors and later unexpected modal errors remain fatal; this
does not classify arbitrary runtime errors as recoverable resource failures.

`SLICKS_PAUSE_LIVE=2` / `LIVEMENUF` injects five sequential failures into the
actual running race: parent allocation, missing language, post-draw rollback,
missing archive, and null surface creation. The allocation cases are boundary
injections, not a claim to have exhausted physical memory. After each failure
the game advances and the next ordinary Escape event retries. The sixth attempt
opens normally and runs the existing Help/Controllers/106% Speed/resume sequence.

Muted A1200/68020/2 MiB gate `amiga/diag_pause_failure.gdb`, port 25099,
`.run/pause-failure-v1`, passes `NATIVE_PAUSE_FIVE_FAILURES_RETRY_RESUME_OK`.
All five before/after full race structures, 64,000-byte chunky screens and full
configuration structures compare identically with `cmp` (15 comparisons).
The observer also checks unchanged status time, refreshed vblank baseline,
active display/engine, released menu owners, successful retry, subsequent frame
150 and final system restoration 0x1f. No debugger target writes are used.

The first harness attempt stopped because this GDB has no Python support;
it was explicitly exited, then replaced with binary memory dumps and host
comparisons. Build, runtime strlen guard, shell syntax and diff checks pass.

Reproduce the state comparisons after the gate (required alongside its marker):
```
for stage in 1 2 3 4 5; do
  for kind in race chunky config; do
    cmp "amiga/.run/pause-failure-v1/before${stage}.${kind}" \
        "amiga/.run/pause-failure-v1/after${stage}.${kind}" || exit 1
  done
done
```

Recovery currently returns silently to the race; an allocation-independent
visible notice remains to be added. Nested live failure paths, repeated
successful pauses, skip/end transitions and in-race edit save/restart remain
open. The full native-player-setup goal is not complete.

### 2026-09-24: native pause Next Track and Main Menu transitions

`SLICKS_OPTIONS_MENU=12` / `OPTIONST` extends the existing native two-track
setup diagnostic. Ordinary menu input selects Arcade, its 5-second minimum,
two tracks, and BASIC then BASICTRK. At frame 100 in each race, before natural
completion, ordinary F9/F10 input opens the production pause menu at row 4/5;
Enter selects Next Track/Main Menu. The diagnostic supplies input only; it
does not assign playlist positions, race completion, driver state or rewards.

Muted A1200/68020/2 MiB run on port 25100 in `.run/pause-transitions-v1`
passes `NATIVE_PAUSE_SKIP_END_TWO_RACES_REWARDS_ONCE_OK`, observed by
`amiga/diag_pause_transitions.gdb`. There are exactly two prepares/starts,
two menu opens/closes, and two reward callbacks. Only the first prepare is
new-game initialization. On skip, the session random seed advances exactly
once for each selected random-per-race profile, with no reseeding/repeated
refresh. The second exit reaches the title redraw, then clean system
restoration 0x1f. Native frame counters and clocks do not advance inside the
menus. Full car arrays and all 64,000 scene bytes compare identically before
and after each pause (four `cmp` checks).

The original-executable dispatcher oracle was rerun: all 17,920
key/navigation/result/modal comparisons pass. Amiga build/strlen guard,
shell syntax and diff checks also pass. This gate proves the implemented
two-track early-exit transitions; it does not prove the missing original
championship standings/shop or every last-track/mode combination. It also
does not test in-race edits surviving a saved fresh restart. Those remain open.

### 2026-09-24: live pause edits saved and recovered after a fresh restart

`SLICKS_OPTIONS_MENU=13` / `OPTIONSP` uses the original Options/Tracks input
sequence, then opens the pause menu at race frame 100. Native Help is visited,
the first controller device changes from 0 to 1, and Speed changes 100 to 106.
After resuming, frame 150 has advanced by exactly 96 custom timer ticks. F10
opens a second pause menu in the same race; Enter returns to Main Menu and
Escape uses the normal program-exit save path. This diagnostic is explicitly
allowed to save only to its isolated run directory, like other persistence
gates; ordinary diagnostic modes still never write setup implicitly.

Muted A1200/68020/2 MiB `amiga/diag_pause_save.gdb`, port 25101, passes
`NATIVE_PAUSE_EDIT_RESUME_END_SAVE_OK speed=106 device=1`. It checks two opens,
two closes, fourteen input checkpoints, the live device consumer, successful
save and final hardware restoration 0x1f. `.run/pause-save-v1/dh1` contains the
142-byte configuration and 3-byte empty profile file. No user profiles were
created by this particular gate; it must not be cited as a nonempty-profile
persistence test.

A fresh emulator/process on port 25102 uses `SLICKS_SETUP_RELOAD=1` and
`amiga/diag_pause_reload.gdb` against the same disk. It passes
`NATIVE_PAUSE_EDIT_FRESH_RELOAD_RACE_OK speed=106 device=1` after frame 100.
Both setup files were read successfully; a new race receives period 618250
and the saved controller device through the production configuration consumer.
Full in-memory configuration and profile structures match the pre-exit dumps
byte-for-byte (`expected.config`/`reloaded.config` and
`expected.profiles`/`reloaded.profiles`). The reloader checks the loaded device
configuration, not an actual pressed physical joystick; that input gate remains
open. It also does not claim playlist persistence or a saved race state.

Regressions rerun successfully: 2,600 setup-storage success/fault cases,
2,640 original Controllers navigation/device/default/exit comparisons,
10,240 key-capture comparisons and 786,432 original Speed comparisons.
Amiga build/runtime guard, shell syntax and diff checks pass.

This closes in-race controller/Speed edit persistence and a repeated-successful
pause gate. Remaining scope includes nested pause failures and visible initial
failure reporting, physical joystick input, original standings/shop and
inter-track vehicle options, and the full final player-setup completion audit.

### 2026-09-24: nested pause failures with allocation-free warnings

The Controllers open-failure path formerly allocated a generic message dialog
to report its failure. It now shares the existing static Help warning reserve
via `slicks_amiga_warning_open`; reporting an allocation failure makes no new
allocation. The warning says `CONTROLLERS UNAVAILABLE - PRESS A KEY` and uses
the same saved-background/font restoration. The existing Help entry point
remains a wrapper supplying its unchanged message. The reserve retains the
legacy `help_warning` owner flag, so the same input/close/destruction path
releases it; no two warnings can own it simultaneously.

`SLICKS_PAUSE_LIVE=3` / `LIVEMENUN`, muted A1200/68020/2 MiB port 25103,
passes `NATIVE_PAUSE_NESTED_FAILURES_RETRY_RESUME_OK` with
`amiga/diag_pause_nested_failure.gdb`. Thirteen ordinary input checkpoints
exercise Help allocation failure, dismiss, successful Help retry/close,
Controllers allocation failure, dismiss, failure loading its eighth icon,
dismiss, successful Controllers retry/close, and resume. Allocation cases
are target-side boundary injections, not physical memory exhaustion. The
resource case actually calls archive lookup with a missing name after seven
successful loads; it checks cleanup of partially prepared child ownership.

Every warning owns the static reserve, with no allocated generic message.
Frame count, game clock and status clock remain fixed throughout. Parent
frames match byte-for-byte after each warning and successful child close:
`parent1` equals `parent3`/`parent5`, and `parent6` equals
`parent8`/`parent10`/`parent12` in `.run/pause-nested-v1` (five comparisons).
Font colours are restored at each parent checkpoint. Full car arrays and
64,000 scene bytes match across the entire pause (two more comparisons).
The race subsequently reaches frame 150 with engine playback, released owner
and final system restoration 0x1f.

Original Controllers rendering regressions pass at both 100,80 and 45,65:
120 sequential full-frame/font redraw comparisons and 20 capture prompts
at each origin, with native close restoration. Build/runtime guard, shell
syntax and diff checks pass. Initial parent-open visible failure reporting,
late child rendering failures, physical joystick input, standings/shop and
the final full setup audit are still open; this is not a claim that all
resource failure modes have been exhaustively tested.

### 2026-09-24: visible initial pause-load failure notice

Initial archive/surface/parent-menu failures now display `MENU UNAVAILABLE -
PRESS A KEY` instead of silently returning to the race. The message uses the
already decoded `race->font.runtime` and the existing shared static warning
save-under. It requires no new menu owner, font load or allocation. The original
message renderer supplies the tint/text/save-under; this error text and its
platform recovery policy are not presented as an original DOS error screen.

The notice is converted into view 0 while the race stays untouched in view 1.
The race remains paused until an ordinary non-modifier key press. Dismissal
restores chunky pixels and font colour, releases the reserve, and reaches the
existing cleanup, vblank-baseline reset and engine restart. Both the menu-owned
and ownerless warning entry points reject an already active shared reserve.

The extended `amiga/diag_pause_failure.gdb` requires five visible-warning
checkpoints in addition to its five recoveries and final successful menu retry.
Muted A1200 runs on ports 25104 and 25105 pass
`NATIVE_PAUSE_FIVE_FAILURES_RETRY_RESUME_OK`. All 15 full-race/configuration/
chunky before-after comparisons still pass, including the font bytes in the
race object. The final run captures the actual palette pointer from the stored
warning painter: optimized debug parameter metadata initially yielded an
incorrect pointer and could not be used for rendering evidence.
`warning1.png` in `.run/pause-failure-v1`, rendered from the corrected target
dumps, was visually inspected: the complete notice is legible and centred on
the race with its dimmed save-under.

The emergency text callback includes the same post-call compiler barrier as
other C/assembly bridges to prevent an unsupported PC32 sibling relocation in
elf2hunk. The first link exposed this and was corrected before the successful
test runs. Build/runtime guard, shell syntax and diff checks pass. This closes
visible initial pause-load failure reporting; it does not close unaudited late
child errors, physical joystick verification or original standings/shop work.

### 2026-09-24: Options consumer audit and Collisions fix

Reading the original DS:0f1c labels identifies option 11 as COLLISIONS and
option 12 as CHANGE CAR. The former resolves into DS:3028. Original
22d2c..22d36 compares that word with zero and bypasses pair collision handling
when zero (jump at 22d33 to 231a3). The native setup previously resolved the
word but discarded it; its pair resolver always ran.

The resolved field is now named `car_collisions` with its DS address retained
in a comment. `prepare_race` stores its inverse zero-test in
`race->car_collisions_disabled`, and the pair resolver returns without mutation
when disabled. Zero-initialized legacy fixtures retain enabled collisions.
Terrain collision routines are unchanged; this is not a switch for walls or
off-track effects. Existing contact latches are deliberately not cleared by
the disabled pair gate, matching the original bypass.

`verify-race-options` now executes original CMP/JNE for all 65,536 option words,
including negative and high-byte-only nonzero values; all match. Its 66,816
full original option-resolver comparisons also pass. `verify-car-collision`
adds a full-state no-mutation assertion over all four drivers with collisions
disabled, then retains the enabled original unequal-weight/latch cases.
The 3,840 original setup-session transitions and driving/terrain regressions
pass. Amiga build/runtime guard and diff/shell checks pass.

`SLICKS_OPTIONS_MENU=14` / `OPTIONSD` selects Custom and Collisions Off through
ordinary native menu keys, closes Options, and selects GO. Muted A1200/68020/
2 MiB gate `amiga/diag_collisions_option.gdb`, port 25106, passes
`NATIVE_COLLISIONS_OFF_MENU_RACE_OK calls=412 terrain_contacts=0` at frame 200.
It checks the edited menu value, configuration/session handoff, actual disabled
gate on every observed pair-resolver call and zero pair collision count while
racing. There were no terrain contacts in this run: unchanged terrain behavior
is supported by the separate host regression, not claimed from this native run.

Two further audit findings remain open:

- WEAPONS (option 7 / DS:3020) is resolved but never assigned to the production
  runtime's `weapons_enabled`. Only the HUD diagnostic fixture sets it. The
  existing HUD/input consumers therefore do not honor the user setting yet;
  firing/shop mechanics also cannot be claimed complete from HUD tests.
- CHANGE CAR (option 12 / DS:3030) has an original read at 245bd, but the local
  result is immediately overwritten with constant 2 at 245c4. Do not invent a
  new gate from the label: reverse-engineer the surrounding intermission flow
  before deciding the correct native behavior. Standings/shop remains missing.

### 2026-09-24: Weapons option, capacities and initial selection

The previously discarded DS:3020 word is now named `weapons_enabled` in the
resolved setup and normalized by a full-word nonzero test into the live runtime.
`verify-race-options` executes original HUD gate 1daaf..1dab9 for all 65,536
words, in addition to its 66,816 option-resolver and collision-gate cases.
The original DS:10a4 capacity bytes are exported from the executable, checked
byte-for-byte and supplied to the live HUD. In particular, weapon capacities
are 100,50,50,10,80,10,100,10, not the diagnostic-only uniform 20.

Original startup 1fd72..1fdc6 first selects the next available weapon after -1.
For a positive (computer) role, it always consumes one shared RNG draw, forms
`floor(rand15*6/32768)`, and selects the next available weapon after that index.
It does this even with Weapons disabled or an empty inventory. Human/nonpositive
roles do not consume that draw. `slicks_initial_weapon` is compared against the
entire original block, including original RNG and arithmetic helpers, for all
256 availability masks, all four driver slots, five signed participation values
and six seeds: 30,720 selected-weapon/final-RNG/stack-boundary cases pass.

After native race initialization receives the session RNG, it now performs
this selection for each driver in order, before the status HUD draw. The
production race therefore no longer starts every driver's selection at -1
unconditionally, nor omits the computer draws. This deliberately changes the
subsequent RNG stream relative to the incomplete port; do not preserve old
captured trajectories by dropping the original draws. The whole original
race initializer is not claimed proven solely by this bounded block test.

`SLICKS_OPTIONS_MENU=15/16` / `OPTIONSE/F` select Custom and Weapons On/Off
through ordinary native input and enter GO. Separate muted A1200/68020/2 MiB
runs on ports 25107/25108 pass
`NATIVE_WEAPONS_OPTION_MENU_RACE_OK enabled=1/0`, using
`amiga/diag_weapons_on.gdb`, `diag_weapons_off.gdb` and the shared observer.
Before racing, both verify all eight weapon capacities, the exact expected
initial RNG and no selection for these empty inventories. At frame 200,
the running race and driver-device configuration still agree with the menu
setting. These are empty-inventory setup tests, not firing or shop tests.

The broader DOS HUD suite passes: 2,400 text command sequences, 24,576 inventory
initializations, 11,264 next-weapon selections, 8,640 weapon-HUD command/fault
cases and 32 composed full-pixel HUD transitions, plus track HUD/record tests.
The 3,840 setup-session transitions, Amiga build/runtime guard, diff and shell
checks pass. Remaining work includes weapon cycling input/firing, original shop
and intermission consumers, seed-sensitive end-to-end regression reruns after
this corrected startup, and the full configure/save/restart/race audit.

### 2026-09-24: saved-player regressions after weapon RNG correction

The current build was tested against four existing menu-created saved fixtures
in fresh muted A1200/68020/2 MiB processes. This is a reload/race regression,
not a claim to have repeated profile creation on the current build.

- Port 25109, `.run/random-vehicle-v1`: random-once human retains its chosen
  vehicle; both shared computer selections make the expected weighted
  random-per-race choices. The observer now also checks the two subsequent
  computer weapon-initialization draws against the original RNG recurrence
  before the race display is installed. The session selection seed remains
  unchanged while the race owns the advanced seed. Empty inventories retain
  selected weapon -1 for all slots. The native reload/race gate passes.
- Port 25110, `.run/setup-unique-v1`: ABC remains the unique human in slot 2,
  shared computers occupy slots 0/1, and slot 3 is inactive. Name, profile
  uniqueness, player order, participation, fixed vehicle and all active
  palette-ramp entries pass. The reloaded profile matches the original saved
  profile structure byte-for-byte.
- Port 25111, `.run/setup-mixed-v1`: the human in slot 0, shared computers in
  slots 1/3 and inactive slot 2 retain their vehicles, roles and source colours.
  The reloaded profile also matches its saved structure byte-for-byte.
- Port 25112, `.run/setup-shared-human-v1`: three slots sharing the built-in
  human profile retain their assignments/vehicles, active-driver ordering and
  distinct fallback colour ramps; slot 2 stays inactive. Saved/reloaded
  selection arrays and all 18 active-slot colour bytes compare byte-for-byte,
  and every active ramp's five RGB shades match the original interpolation
  formula. The six inactive-slot colour bytes differ: they retained previous
  menu values at save time and are zero in the freshly initialized session.
  Original selection skips colour assignment for profile zero, so these
  unused transient bytes are not a persisted per-driver colour requirement.

All four native gates pass. SHA-256 checks before and after confirm none of the
eight saved CFG/PLR files changed during these reload-only runs. The observers
perform no target-state writes. Final process inspection confirms no test
emulator/debugger remains. No production change was needed in this regression
turn; only the random-vehicle observer gained the post-start RNG check.

This protects the central human/computer/inactive/shared-profile handoff after
the startup correction. It does not replace a fresh full creation/edit/save
audit, physical pressed-joystick verification, or unfinished original
intermission/shop and weapon consumers.

### 2026-09-24: clean-start native creation, save and fresh-race audit

`.run/setup-clean-v1` did not exist before this test (the launch explicitly
required that). No CFG/PLR fixture was copied into it. Current-build muted
A1200/68020/2 MiB port 25113 runs ordinary `PLAYERSY` menu input: create ABC
through name entry, use the colour dialog/edit flow, move the unique profile
through all four driver slots, and exit/save. All six displacement checkpoints
and system restoration 0x1f pass with
`UNIQUE_PROFILE_ALL_FOUR_SLOTS_DISPLACEMENT_SAVE_OK`. The new disk contains
142-byte CFG and 61-byte PLR files.

A fresh process on port 25114 runs `SETUPR` on that new disk. It passes
`UNIQUE_PROFILE_FRESH_RELOAD_RACE_DRIVERS_VEHICLE_COLOURS_OK`: ABC remains a
unique human in slot 2, computers share profile 1 in slots 0/1, and slot 3 is
inactive. Name, profile flags, player ordering, participation, selected vehicles,
source colour endpoints and all five RGB shades per active car are checked
at the actual race startup boundaries. The saved/reloaded ABC profile structs
compare byte-for-byte.

The native configuration at `prepare_race` is also dumped. New read-only tool
`build/verify_native_configuration` converts only the m68k struct's byte order
and re-encodes every persisted field through the original-code-verified CFG
writer. Comparing with the saved file passes all 142 bytes, rather than only
checking selected fields. As a negative control, the independently saved
106%-Speed pause fixture is rejected against this 100% configuration at byte 6.
The configuration oracle was rerun: 256 startup defaults, 256 full save streams,
1,024 original load/state comparisons and 141 truncated-input cases pass.

The unique save/reload observers are now shared `_checks.gdb` scripts with a
caller-supplied dump directory. Existing entry points retain their old directory;
new `diag_setup_clean_save.gdb`/`diag_setup_clean_reload.gdb` isolate this run
without duplicating assertions or overwriting prior fixture artifacts. No
production change was needed. All diagnostics exited, and diff checks pass.

This proves the requested configure-through-native-menus/save/fresh-restart/
correct-race handoff for this representative mixed-player case on the current
build. It is not a claim that missing original standings/shop/intermission
screens, weapon consumers or physical joystick input have been completed.
