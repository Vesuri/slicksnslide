# Native race status

This is an accumulated evidence log, not the active TODO list. Sections below
record intermediate builds and may say that now-completed features are still
pending. For current status use [open-work.md](open-work.md),
[player-setup-completion.md](player-setup-completion.md) and
[audio-channel-plan.md](audio-channel-plan.md). Historical tests/hashes are
not automatically certification of the latest integrated build.

## Original font and HUD background connected to production

The native driver HUD now calls the translated 68020 string/glyph routines,
not the temporary opaque white/black renderer. Startup loads `alamenu.@I`
from `SLICKS.000` (2,542 bytes, decoded to 320x16 at y=184), and lap/finish
redraws restore each original saved-background region before transparent
indexed text. The 52x22 rounded VGA save-under is clipped at x=320/y=200:
those original off-display bytes must not wrap into another chunky row.
The racing/finished text colours follow original nearest-colour targets
(50,50,70)/(60,60,35). The runtime font is reconstructed in its original
padded layout; archive-derived bytes remain local runtime data.

`make verify-hud-background` executes original `2e51a`, substituting only
file/allocator/scheduler boundaries: all 5,120 decoded pixels match and every
truncated input fails the native decoder. `make verify-dos-hud` now also runs
original `1ddc0` through real background restore, palette changes, numeric
formatting, strings, glyphs and VGA writes. Across 12 sequential four-driver
lap/finish/time transitions, every visible pixel matches on both DOS pages.
That composed test excludes status/weapon calls; it is not proof of complete
HUD functionality. Host dirty tests and the 320-case GCC font bridge test pass.

An isolated muted A1200 `.run/original-hud` run, port 24944, completed at 700
updates with 24 translated font calls, race error 0, and no bitmap-audit
failure. Startup and final native chunky/palette dumps were inspected: the
real grey HUD strip, lap numbers and last-lap times are visible. The harness
exited 1 solely because its new assertion expected frame 200, while BITMAPAUDIT
configures 700 (confirmed in slicks_diag.c). The assertion/output order is now
corrected; it has not been rerun. The historical `frame200.chunky` artifact is
actually the frame-700 dump. No emulator remains live, and no normal launch
image has been promoted. This is not yet the complete HUD: the left-side track
information and weapon/selected-player states remain open.

## Setup storage transaction and Amiga adapter

`setup_storage.h` supplies a platform file transaction for the two original
encoded streams, not a replacement serialization format. It stages both
files, moves old destinations to backups, installs both replacements and
then removes backups. Reported I/O failure rolls back the pair; if rollback
itself fails, old bytes remain at the original or backup names for recovery.
Existing `.new`/`.bak` files block the operation without mutation. Failed
creation distinguishes no ownership from a partial file that we may remove.
Successful publication with failed backup cleanup has a distinct result.

`make verify-setup-storage` passes 2,600 simulated success/single/double I/O
failure cases across all four old-file-presence combinations and partial vs
pre-creation write failures, plus four untouched-leftover-artifact checks.
All original bytes remain recoverable and successful pairs contain exactly
the CFG/PLR encoder output. This is a transaction fault model, not yet a live
filesystem or fresh-process restart test.

`amiga_setup_storage.c` cross-builds with `out/SlicksSetupStorage`. Its DOS
adapter checks file types, distinguishes absence from other lookup errors,
handles short writes and flush/close errors, refuses rename-over-existing,
and retains the first error/path for caller reporting. It is not wired to
the live save trigger yet; no files have been saved by these tests.

The two-file publication is not claimed power-loss atomic. Startup must
detect leftover artifacts and present recovery instead of accepting an
ambiguous mixed generation. Remaining integration: explicit native CFG
signature policy, original dirty/save caller lifecycle, configuration loading,
failure/recovery UI and isolated target save/restart verification.

## Complete original colour-dialog integration

`colour_dialog.h` composes original 2f290..2f6d9: 80x30 save-under,
fixed 75-percent (15,15,40) tint, original caption colour with immediate font
restoration, RGB bar colours, local-copy controls/pulse and accepted-only
endpoint commit. Both editor endpoints now open the original dialog at
(160,145) and (160,160). Allocation/free occurs with AmigaOS available;
the existing BIOS-tick accumulator drives pulse state while hardware is owned.
Name and colour actions are no longer pending placeholders.

The full-pixel verifier now passes 104 colour-dialog screen/font-state cases
against the real DOS wrapper and native 68020 font renderer, including both
positions, RGB controls, repeated tick buckets, acceptance, cancellation and
restoration. Existing 138 name and 120 list/delete screen comparisons pass.

`out/SlicksColourDialog`, `PLAYERSC`, `diag_colour_dialog.gdb`, port 24981
passed a muted-host A1200 2 MiB/zero-fast run. Ordinary keys create ABC,
reopen Edit, accept changes to endpoint one, cancel endpoint two, reopen and
accept endpoint two. Fifteen editor and ten colour presentations were checked;
local changes do not leak before acceptance, cancellation leaves the endpoint
unchanged, accepted colours survive repeated editing, and font state restores.
Prompt and final menu frames match all 64,000 chunky/bitplane pixels.
Artifacts: `amiga/.run/colour-dialog-v1/`. No profile files or normal launch
binary were changed. Persistence and complete native options/help remain open.

## Live original name-entry integration

The editor now opens the original name dialog and processes its character
state rather than leaving a pending name action. The Amiga adapter snapshots
the installed keymap for shift/caps/alt combinations while AmigaOS is running;
hardware-owned input uses that table and handles press/release modifiers.
Name buffers are allocated/freed only outside takeover. The VBlank loop drives
the cursor's 200 ms bucket, and all painted bounds feed existing dirty-row C2P.

`out/SlicksNameDialog2`, `PLAYERSN`, `diag_name_dialog.gdb`, port 24980 passed
on a muted-host 2 MiB/zero-fast A1200. Ordinary input creates ABC, selects
the new record in Edit, backspaces/appends to commit ABX, reopens it again,
then cancels its name prompt and rejects empty-name F2. Count remains four
and the committed name remains ABX. Nine name presentations and six editor
presentations were checked. Typed and final menu frames each match all
64,000 authoritative chunky/bitplane pixels. Artifacts are in
`amiga/.run/name-dialog-v1/`; no profile files or default executable changed.

Test infrastructure finding: this FS-UAE GDB stub silently ignored attempted
target-memory writes, including input queue bytes (confirmed by immediate
readback). The initial runs stopped after opening Edit. Subsequent input is
now queued inside the diagnostic program as ordinary key events; GDB only
observes state. Do not use debugger assignments to target memory as evidence
that a fixture or input event was applied. All those earlier runs are closed.

Colour-picker integration and persistence remain open. The final setup goal
still requires a fresh-process save/restart and correct race setup.

## Original name-dialog rendering and editing lifecycle

`name_dialog.h` composes original wrapper 307b6, input 2f6da, the verified
character-state helper and cursor save-under/underline operations. It uses
the caller's working name, 20-character limit and flags 0x203, preserving
existing text on entry. The original grey tint and caption/value colours,
saved-rectangle rounding, uppercase/glyph filtering and font restoration
are retained. It uses caller-owned bounded buffers rather than preserving
the DOS wrapper's missing outer-buffer free.

`make verify-list-pixels` now passes 138 full-screen/full-font-state name
dialog comparisons against original DOS rendering and native 68020 fonts:
empty, short and 20-character initial names; append, backspace, space,
ignored input; visible/hidden cursor; Return/Escape and outer restoration.
The existing 120 list/delete comparisons still pass. The staged DOS runner
invalidates translated blocks before changing stop boundaries, avoiding
cached blocks that run through the next intended stopping point.

This verifies the dialog dependency, not its Amiga keyboard/clock integration.
Next: connect name requests in the active editor, translate platform text
input, drive cursor timing and test actual Add/name/accept/re-edit on A1200.
The focused user goal and final save/restart/race gate are recorded in
`player-setup-completion.md`.

## Native profile editor property controls

The verified original editor painter is now connected to Add and eligible
Edit list results. The live controls use the original row/key/limit helpers;
F2 and DONE accept, while Escape/F9/F10 cancel. Cancellation deliberately
does not roll back profile properties, only the deferred name commit. An
accepted empty name does not append a record. Font colour and the full
prepared menu background are restored before redrawing the player screen.
Name and colour subdialog requests are still explicit pending actions, not
implemented modal behaviour. No profile-file writes are enabled.

`out/SlicksProfileEditor`, `PLAYERSE`, `diag_profile_editor.gdb`, port 24976
passed a muted-host 2 MiB/zero-fast A1200 run. Ordinary keys open Add, change
type, percentage and vehicle, cancel, reopen and request empty-name F2.
The test checks seven editor presentations, retained cancelled properties,
the next new-profile initialization preserving vehicle but resetting type
and percentage, unchanged logical count and restored font state. Edited and
closed-menu frames each match all 64,000 chunky/bitplane pixels; artifacts
are under `amiga/.run/profile-editor-v1/`. The default executable is unchanged.

New-profile initialization now permits slots 1/2 after original-style Delete
reduces count below three. Its original-DOS full-data-segment/RNG comparisons
now cover 1,280 cases; the 4,419 deletion, 630 editor-tail, 6,108 key and 1,024
limit comparisons still pass. The next integration is the original name
prompt, whose caller uses flags 0x203 (preserve existing text), not just 3.

## Original profile editor redraw

`profile_editor_draw.h` translates 27bea..27f53, retaining the six saved-row
restores, selection bevel, working-name text, role/percentage, signed vehicle
branches, swatches and redraw-byte lifecycle. `profile_editor_renderer.h`
composes the existing original-backed background, bevel, palette, 68020 text
and icon painters; the destination addressing continues to use mult320.

The expanded setup verifier passes 864 complete original drawing-command,
string, order and redraw-state comparisons (including zero redraw, all rows,
negative/random/out-of-range vehicle bytes and empty/long names). The palette
verifier passes 108 sequential full-screen and full-font-state comparisons
against real DOS rendering, with real archive assets and native 68020 font
and icon routines. Existing setup and palette gates still pass. This is the
verified painter dependency, not a connected Add/Edit modal: name/colour
subdialogs, commit/cancel lifecycle and live integration remain next.

## Native profile Delete integration

The shared Edit/Delete action list now opens at original coordinates
(120,90)..(270,190), with initial action `4-row`, flags zero and the
original DS:13a0 captions. Delete results pass through the verified original
dispatch and DS:13b5 confirmation prompt. Y/K/J confirm; other keys cancel.
The original deletion helper updates profile storage and selected indices;
setup resources receive the new logical count before selection is rerun.
The full prepared menu background and prior font colour are restored on
modal return. Edit results remain explicitly pending: no editor is claimed.
Profile-file persistence is not connected by this change.

`out/SlicksProfileDelete`, `PLAYERSD`, `diag_profile_delete.gdb`, port 24975
passed a muted-host, 2 MiB/no-fast-RAM A1200 regression: six ordinary Down
events open Delete, N cancels the first prompt and Y confirms the second.
The profile structure before/after cancellation is byte-identical. Confirmed
deletion reduces count 3 to 2, refreshes setup count, frees the list and
restores font colour. Prompt and closed-menu bitplanes each match all 64,000
authoritative chunky pixels. Artifacts are in
`amiga/.run/profile-delete-v1/`. Original-DOS action comparisons (131,072)
and full-screen/font-state list/prompt comparisons (120) pass. This isolated
debug build does not replace the normal launch executable.

## Shared precomputed 320-byte row offsets

`src/graphics/sgfx_mult320.s` supplies one 256-entry, 32-bit `mult320` table
(1 KiB, assembler-generated, no startup multiplication). Existing particle
retirement, partial row/rectangle conversion, sparse-pixel writes, car and
particle drawing/restoration, and HUD pixel addressing now use it. Target C
references the same symbol through `row_offsets.h`; host tests have matching
constant entries. The sparse writer reuses the offset for 320-byte interleaved
bitplane rows; its generic bitmap-stride fallback remains intact.

Font comparisons, dirty/HUD tests and surface-effect tests pass. The real
68020 writer checks pass 1,394 rectangle/sparse cases, 48 retirement cases,
70 motion cases and 1,920 original-DOS lifecycle comparisons. No frame-time
gain is claimed without a fresh A1200 measurement.

HUD dependency correction from following the original call sites: the
`3aaf2` call at the start of `1ddc0` restores saved HUD background, not a car
icon. `250b9..250e4` first captures a 50x22 region at (90+60*i,186) with
`3abe5` into DS:4bd2; its planar width rounds to 52 pixels. A separate 16x8
weapon-area background is captured at (90+60*i,192) into DS:4be2. The original
capture includes rows beyond the visible 200-line display; the native port
must retain visible behavior without writing beyond its 64,000-byte surface.
This is an in-game save-under operation, not a DOS screenshot asset.

## Direct 68020 port of the original font drawing dependencies

Per the user's direction to port the original HUD code rather than recreate
its appearance, three drawing dependencies are now translated to 68020:

- `sui_font_glyph.s`: original `2fef2..3000e`, preserving padded glyph rows,
  transparent source zero, and indexed font-palette lookup. The hardware exit
  writes the authoritative chunky surface instead of selecting a VGA plane.
- `sui_font_measure.s`: `300e5..301aa`, including signed spacing, glyph-zero
  lookup behavior, tabs, and the original accumulated tab-position update.
- `sui_font_string.s`: `301ab..302b5` and the `3000f` character wrapper,
  preserving alignment, newlines, the 1,000-character bound and two-pass
  offset shadow/foreground drawing with palette restoration.

`make verify-font-glyph` executes the actual DOS routines through their VGA
plotter (only port/window hardware is modelled) and executes the assembled
68020 ports. It passes 1,236 full-frame glyph pixel comparisons, 4,176 width
comparisons, and 320 full-string pixel/return-value comparisons. The fixture
uses the real archive font in padded runtime layout, varied palettes including
colour zero, signed spacing and shadow offsets, tabs/newlines and all flags.
Register preservation and font-palette restoration are checked too.

These dependencies are now called by the production HUD, as described above.
The earlier opaque white/black command renderer has been removed. Remaining
HUD work includes left-side information, weapon states, inactive driver slots
and broader race-time sequencing/target verification.

## HUD priority: original driver text commands recovered

HUD work is now the user's immediate priority. `src/ui/race_hud.h` implements
the text-command portion of original `1ddc0..1e009`: 60-pixel driver panels,
lap modulo 100 right-aligned at (101+60*i,186), or finished place at
(99+60*i,186) followed by a dot at (100+60*i,186). Time is left-aligned at
(106+60*i,191), formatted `SS.hh` with a decimal point. It uses DS:3037
(last lap) for racing drivers and DS:4c06 (best lap) for finishers, not
the current native elapsed race time. Inactive driver slots emit nothing.

`make verify-dos-hud` executes the complete original ddc0 routine and real
aceb formatter/arithmetic helpers, capturing graphics call boundaries.
**1,600** sequences match both display pages across drivers, active state,
finish ranks, lap boundaries and time clamp boundaries. Numeric/text pixel
rasterization, saved backgrounds, palette selection and status-bar rendering
are stubbed in that command-only phase, explicitly not proven by it. The
separate composed phase described above now executes the real drawing path.
The native command helper
is now connected to an intermediate `draw_timers` implementation, with host
tests for positions, lap/finish transitions and dirty coverage. This is not
proof of original rasterization; the direct port described above supersedes
that integration as the intended implementation.

The pending physics check was closed with an unsigned velocity-factor fix:
bias -600 had made the signed native factor reverse velocity. All **2,880**
expanded composed integrator cases now match, and the damage suite passes.
Do not expand physics work ahead of HUD.

The earlier `.run/finished-car-driving` A1200 run (port 24943) completed at
the 7,200 observation limit without all drivers finishing. Indices 1,3,0
finish and keep moving; index 2 remains on lap 3 at (21141,6850), damage
267 and service state 2. Results are not reached. This run predates the
unsigned-factor change and any HUD integration; no emulator remains live.

## Finished entrants retain driving and collision participation

For the supported fixed-lap race with no end deadline yet set, original
`20373..20509` does not suppress a finished driver's controls. It caps
throttle scalar at 7000 after the normal maximum-speed and empty-fuel caps.
Native code no longer zeros controls merely because `finished` is set.
The collision eligibility block `22d36..22d63` checks active entrants and
layer, not finish rank; the native finished-car exclusion is removed.
Lap/time bookkeeping remains frozen for already finished entrants.

The extracted throttle helper also reproduces signed low-word tick products,
32-bit scalar addition and signed low-word maximum-speed product. **672**
original-code cases include the no-deadline caller gate, normal/finished and
empty-fuel states, positive/negative/extreme scalars, tick-product overflow
and maximum-product overflow. **48** original pair-gate cases verify both
finished/unfinished combinations on every distinct active same-layer pair.
Full native-update and unequal-weight collision fixtures confirm the new
paths are reached. Existing composed physics/damage regressions pass.

This is not timed-race shutdown or the original post-finish delay. Original
`98b6` identifies mode 5; that mode can arm an 1800-unit end deadline after
the winner. With no unfinished active entrants, `22ca0..22cd9` arms a shorter
230-unit deadline. Those end-deadline/input-suppression and results-return
semantics remain open; current native results still start on all finished.
The new finished-entrant changes require their own target regression.

## Car-update phase boundary corrected

The original motion loop `202eb..214e8` visits all four drivers before
the tail loop `221a7..23d94`. Native code formerly interleaved each car's
motion and complete tail, allowing collisions/effects/service changes to
affect later cars before their movement. Production now stores four control
bytes, executes all `prepare_car_motion` calls, then all `finish_car_update`
calls. This retains the existing within-tail order; it is not a claim that
every tail event or intervening original phase has been recovered.

An original-instruction oracle verifies all eight driver-loop continuation/
exit edges. A native approaching-car fixture distinguishes the old
interleaving from the required two-pass ordering and checks production uses
the latter. Isolated single-car tests retain an explicitly test-only wrapper.
The composed 720-case original integrator regression passes after the split.

Before this scheduling change, the silent A1200 `.run/collision-counter-alias`
run (port 24941) **finished naturally at update 4,069** with the boundary,
reverse-coast and collision-counter fixes. Finish order was indices 1,3,0,2;
all reached lap 5. Damage peaks were 112/45/244/0, repair frames 19/0/29/0,
and final fuel 0/0/-4/0. Results were drawn, all four HUD audits passed,
including two damage audits, with zero pixel failures. The strict 3,600
deadline still failed (index 2 then on lap 4). This does not establish
original trajectory equivalence and must not be attributed to the subsequent
two-pass change, which requires its own A1200 run.

That separate two-pass A1200 run `.run/two-pass-car-update` (port 24942)
completed its bounded observation and **failed to finish within 7,200**.
Indices 1,3,0 finished; index 2 stayed on lap 3 at (21152,6850), velocity
(78,6), fuel -2, damage 267, AI/service states 1/2 and watchdog 100.
The strict 3,600 checkpoint also failed. No collision/load error was reported;
results were not reached. This supersedes 4,069 as current integrated evidence.
Keep the independently verified two-pass ordering, not the earlier ordering
merely because its race finished. The newly corrected finished-entrant driving
and collision behavior was **not** included in this run.

## Composed pit motion: reverse coast and collision-counter alias

The production force/movement loop is extracted into `integrate_car_motion`
without introducing CPU state or an interpreter. The differential test runs
original `20dd1..214df` with real arithmetic helpers and the complete `cb02`
walker/response, using BASIC's decoded service-enabled masks. **720** cases
cover five upper-pit positions, drive scalars 3000/0/-3000, four headings,
three velocity pairs, active/coasting input and one/two elapsed ticks. They
compare position, velocity, scalar, pending damage and update contact.

This composed test exposed and corrected two production mismatches:

- Ordinary coasting decays negative drive scalars too, using the original
  wrapped Q15 arithmetic shift. Native code previously skipped negative
  scalars and used truncating division for positive ones. The first negative
  fixture kept -3000 natively while the original produced -2904.
- Both `2131b` walker output pointers alias **BP-3a, the integration loop
  counter**, not an otherwise unused scratch word. On contact the walker
  stores clear X then clear Y there. The subsequent increment and signed
  comparison use that Y, normally ending remaining physics substeps. Native
  code previously continued all substeps. The walker now returns clear Y to
  the actual loop counter, preserving the alias rather than assuming a
  universal early break. At (21290,6850), coasting velocity (861,-713), two
  ticks formerly produced (21271,6825) instead of original (21250,6850).

All 720 cases and the existing damage, driving and car-collision suites pass.
This covers the composed integrator, not input steering, AI, the later
per-driver effects/service tail, dynamic map mutations or whole trajectories.
An updated Amiga damage/fuel race is required before claiming a finish fix.

## Screen-boundary damping restored

Inspection of the composed movement loop found a production mismatch:
`2000:1357..14d1` clamps the car centre to x=300..31700 and y=300..17900,
but also damps **both** velocity components for each clamped axis. Native
code previously clamped position alone. `clamp_car_to_track` now applies
the original unsigned Q15 factor `word(0x7dd4 - drive_bias)`, initialized by
`aeb6..aedd`, using the existing low-dword multiply/arithmetic-shift helper.
An out-of-bounds corner applies the factor twice, in X-then-Y order; exact
boundary coordinates do not damp.

**7,168** original-instruction comparisons cover all four drivers, both
coordinate boundaries and signed extrema, positive/negative/extreme
velocities and biases including an unsigned factor above 0x7fff. A full
native-update fixture verifies damping occurs after movement and changes
the other velocity component too. The complete damage suite (including
15,840 real-pit walker comparisons), driving and car-collision gates pass.

The Amiga build and isolated muted A1200 run `.run/world-boundary-damping`
(port 24940) pass the 700-update BUMPS gate: 35 takeoffs/landings, 373 shadow
frames, peak 6916, 35 jump sounds, 15 wall and 31 car-contact sounds, no
unexpected weapon sounds or reported load/collision errors. This is not
proof of the unresolved BASIC fuel/pit trajectory or natural race finish.
The normal launch image was not promoted by this isolated debug run.

## Damage/fuel non-finisher isolated

The completed `.run/nonfinisher-all-cars` observation identifies car index 2
as the sole non-finisher at update 7,200 (lap 3); indices 1, 3 and 0 finish
in that order. Index 2 stays beside the upper pit, approximately (211,68),
with exhausted fuel and damage 214. This is not a missing results trigger:
one car genuinely has not completed its laps.

The subsequent silent `.run/service-stall-snapshot` A1200 run captured its
full state at update 3,600: position (21139,6850), heading 894, measured
speed 16, velocity (32,0), target (214,64), AI state 1, service state 2,
watchdog 100, recovery timer 398, fuel -4 and capacity 3312. Service state 2
holds the stationary watchdog at 100 rather than initiating recovery.

`verify_dos_ai` now replays this state through the **complete original
f09d AI routine**, including its real steering helpers, and compares native
controls and AI state/timers/target. All **800 calls** (400 each at tick
steps 1 and 2) match. Physical position, heading, velocity and contact age
are deliberately held fixed; this proves the response to the captured
stall, **not** that the original game follows the same trajectory into it.
The full existing AI suite also passes. Continue with the composed movement
and collision path into the pit; do not invent a watchdog override.

The moving-ray oracle now additionally decodes BASIC's actual archive masks
with service enabled and packs them into the original raw/upper-mask layout.
Across centres x=209..218, y=61..71, all nine immediate destination offsets,
four driver indices, both layers and both sampling states, **15,840** calls
to the complete original `cb02` walker match native hit/no-hit, position,
velocity and damage output. These use the captured velocity (32,0) and
measured speed 16. No fixture addresses unretained map bytes. The synthetic
walker, track-response and complete damage regression suite also pass.
This rules out a walker/response difference for those identical inputs;
both sides receive the same asset-decoded masks, so it does not independently
prove dynamic map state or the composed force/steering/collision trajectory.

A separate finish-fidelity issue remains: the native update suppresses
finished-car controls and excludes finished cars from pair collisions,
whereas the original contains a reduced-speed finished-car driving path.
Its complete caller/order behavior needs verification before changing it.

## Track-specific actor records retained

The production track loader now records the five actor-producing object
types (79/80/81/82/89) in source order, rather than only excluding their
static sprites. The records retain original kind, layer 1, zero initial
velocities and signed Q4 coordinates from `1b908..1b9ae`. Kinds 1/3 use
the signed -2 coordinate offset before the wrapping word shift. The
original count saturates at 99; record 99 is repeatedly overwritten as
overflow scratch and is not included in the subsequent actor-creation loop.

**2,200** original-code comparisons cover all 110 object types, counts
0/1/98/99 and boundary coordinates, comparing every field of all 100
records, including untouched entries. A real BASIC asset fixture verifies
its two records (kinds 0/2, Q4 positions 48,32 and 3440,992). All 195 tracks
load with service off and on; 14 contain actor records, maximum 32 in F1.
BASIC has two and BUMPS zero. Original AI, surface and dirty gates pass;
the A1200 build succeeds with 1,002 additional bytes of navigation metadata.

These are loader records, not an implementation of track-actor simulation
or rendering. Their later positions need not equal their loaded coordinates.
Shared-pool reservations can now be based on actual track contents, but the
allocator and dynamic object behavior still require integration.

The current integrated silent A1200 damage/fuel regression
`.run/track-actor-metadata` (port 24937) **does not complete within 7,200
updates**. Car 3 reaches lap 5 at update 2,847, but not all cars finish;
the earlier 3,600 checkpoint also remains incomplete. This supersedes the
older 4,085-update completion observation for the current build. There was
no reported load/collision error, but the results gate did not run and must
not be reported green. The emulator stopped at the unchanged observation
limit. The diagnostic now prints all four cars at the deadline and limit
for the next run; this failed run only logged car 3's sparse progress.
Do not attribute the failure to the new metadata without isolating it:
the older successful full race predates the collision-burst RNG changes.

## Live shared actor-pool inventory

The bounded `SLICKS_TRACE_POOL` reference hook records PRE/POST actor records
for the first 256 calls, without changing game state. Its reusable patch is
`tools/patches/dosbox-x-slicks-actor-pool.patch`, applied after the page-trace
patch. `tools/verify_actor_pool.py` rejects incomplete/misordered pairs,
missing records, unexpected capacity/high-water bounds, missing page toggles
and incorrect state/lifetime transitions. It checks lifecycle state, not
the correctness of rendered pixels or every allocation between calls.

The completed silent BASIC capture `tmp/pc-pool-reservations/layers-paced.log`
passes **256 complete calls and 38,299 slot transition comparisons**.
The pool capacity is 200 (handles 1..199); maximum observed high-water is 169.
Its actual reservations are not simply four cars plus four shadows:

- First PRE: cars 1..4; persistent track-object actors 5/6 (priority 6);
  temporary startup actor 7 (priority 10, lifetime 30).
- Second PRE: shadows occupy 8..11 (state 3, priority 1, lifetime 1);
  new point effects start at 12. The four shadow slots become -3 and stay
  reserved even when invisible.
- Startup slot 7 becomes -2 in POST call 31, free in POST call 32, then
  is reused by a permanent point in PRE call 33. A contiguous "reserve
  the first eight slots" approximation would therefore be wrong.

The trace agrees with the original track-object allocation loop
`24eec..25041` (DS:3124 objects) and shadow allocation at `1fc80..1fcf0`.
It also shows point emissions already present in the first outer update;
the native countdown's early-return behavior needs an original caller-gate
comparison before claiming startup emission/cadence parity.

Reproduction (all captures/byte-derived data remain local-only):

```sh
SLICKS_TRACE_POOL=1 SLICKS_TRACE_PAGES=1 make reference-layer-trace \
  REFERENCE_FIXED_ROOT=tmp/pc-pool-reservations REFERENCE_KEY_PACE=2
python3 tools/verify_actor_pool.py tmp/pc-pool-reservations/layers-paced.log
```

Next integration must carry track-specific reserved slots, the temporary
startup actor's release and persistent shadow handles into a shared native
slot model. No production executable was changed by this inventory step.

## Retained point retirement

The native compact pool now keeps expired points occupied through their
first retirement pass: temporary state 1 becomes -2, permanent state 5
becomes -6. They are excluded from drawing/restoration buckets and released
on the following actor pass, matching original `33fa7..34021`. Emission
precedes that next pass, so the retained entries still consume capacity
during intervening emission. This supersedes the immediate-removal limitation
recorded below. The previously spare byte at offset 23 holds state; target
compile-time assertions retain the 24-byte ABI.

`verify-planar-writes` now compares the production 68020 routine directly
with original x86 expiry, motion and retirement across **24 sequences of
80 updates (1,920 comparisons)**: states 1/5, both starting pages, lifetimes
0/1/2/3/35/50. It compares occupied count, lifetime, signed-word positions,
retirement state and draw-bucket membership. Original retirement redraw is
stubbed, so this is not a multi-page pixel/overlap proof. Adjacent original
slices have their Unicorn code cache cleared and their stop PCs checked;
without this, cached blocks can run past a changed slice endpoint.

The existing 48 real-68020 retirement/capacity cases now also verify the
following-pass release/compaction without duplicate dirty entries. Motion,
point raster, surface, dirty, damage, driving and car-collision gates pass.

Remaining shared-pool work is material: the original allocator capacity
word DS:16be is 200 in the unpacked initializer and the local BASIC/BUMPS
race-data captures. Valid handles are below capacity, with handle zero
reserved. The native 256-entry particle-only capacity is not equivalent.
Original race entry `1fc80..1fcf0` allocates persistent shadow actors for
enabled drivers and stores handles at BP-18; those and car/reserved actors
must share the eventual exact slot model. Retaining point states does not
yet establish lowest-free-slot identity, overlap order or exhaustion parity.

The rebuilt silent A1200 BUMPS run `.run/point-retirement` (port 24936)
passes at update 700: 40 takeoffs/landings, 429 shadow frames, peak 6384,
40 jump, 13 wall and 36 car sound requests, no weapon requests. The owned
emulator stopped afterward; the normal launch image was not promoted.

## Point raster parity and particle update phase

`make verify-dos-points` executes the real `33673` point drawing routine and
its real `3b55:000e` VGA plotter. The test models only sequencer plane select
and byte writes to the VGA window, converting those writes into a comparison
surface; it does not stub the game's drawing calls. **12,288** whole-surface
comparisons match native `draw_trail_particles`: all 256 raw foreground-mask
bytes, limits 0/15, both page-coordinate slots, all colour bytes, four plane
selections and interior/edge/out-of-bounds positions. This proves individual
point painting for the race's packed-map mode, not multi-actor retirement.

The production particle pass now follows all four car updates, matching
the original caller at `2000:3f51`: a newly emitted point moves and consumes
its first lifetime tick on the same actor update. The 68020 routine now
leaves zero lifetime unlimited and defers expiry on actor page zero, as the
original expiry oracle requires. Page-one expiry still removes the compact
native entry; exact original slot retention/reuse is **not** implemented by
this change. Permanent-mark overflow fallback uses the same page gate.
The 24-byte particle layout is unchanged; the assembly call gains a page
argument. Profiling keeps its named stages despite the execution reorder.

Real 68020 tests pass 48 lifetime/page/compaction/capacity cases and 70 motion
boundary cases. Surface tests explicitly reject early page-zero permanent
commits. Dirty tracking, DOS point/expiry, AI and damage regressions pass.
Original shared-pool slot order, negative-state retirement and composed
overlap sequences remain open fidelity work.

Silent strict A1200 BUMPS run `.run/point-phase` (port 24935) passes at
700 updates: 18 zones, 40 takeoffs/landings, 429 shadow frames, peak 6384,
40 jump, 13 wall and 36 car sound requests, no weapon requests. Emulator
stopped afterward. The subsequent edit only repositions profiling markers
to preserve their labels; that build is rebuilt, not another target run.
The normal launch image was not promoted.

## Collision point constructor and first movement

`verify-dos-particle-expiry` now runs the real point constructor (`332ad`),
allocator (`330ab`) and actor updater (`32ef2`) together, without stubbing
those boundaries. The actor pool is preallocated, as during a race; heap
initialization is not part of this test. **2,048 cases** cover all 256 colour
bytes (including signed-word arguments), both burst lifetimes, both layer
limits and available/full pools. They verify balanced far returns,
constructor fields, updated position/velocity/lifetime/layer/state/priority,
unchanged neighbouring slots and no record mutation on allocation failure.

The original creates these bursts as **state 1, priority 3**, not state 3.
Point save buffers are the `0000:0100` sentinel, source size is zero, and
initial page-X coordinates are 320. The follow-up updater preserves the
colour byte and selects frame zero. This supports the native temporary
point representation, but does not prove equivalence of the native compact
pool to the original shared actor pool under exhaustion or retirement.

For the 1,024 successful allocations the test also executes the original
first movement (`3394f..339d2`) with extreme signed velocities, checking
16-bit position wrapping and arithmetic-shift page coordinates on both
pages. It stops before animation/rendering: no pixel-output or composed
expiry/page-timing equivalence is claimed. All existing expiry, retirement,
allocation and reserved-shadow sequence checks still pass. No production
binary changed in this verification step.

## Collision burst emission

The native contact tail now emits the original `23b0f..23c94` point burst
after the verified contact sound. The same update-contact/previous-contact
gate suppresses repeats. The per-driver collision-partner scratch records
the scanned partner index (`23193..2319c`), including latched overlaps,
and is cleared when a burst is emitted. Index zero deliberately selects the
same branch as a zero scratch byte; it is not converted to a boolean hit.

- Nonzero scratch: 3 particles, lifetime 35, jitter radius 12, velocity
  divisor 60.
- Zero scratch: 7 particles, lifetime 50, jitter radius 7, signed velocity
  divisor **-126** (the original byte 82h is sign-extended).
- Both use priority 3, current centre coordinates, and layer-dependent
  occlusion 0/15. The point is the driver's palette slot `index*5+3` for
  one third of random draws; otherwise it uses the original nearest-colour
  match to RGB 55/55/10. These are temporary actors, not permanent marks.

RNG order is colour, Y jitter, X jitter. All draws are consumed even if the
native pool cannot allocate another particle. **960** cases execute the
original emitter with real RNG/multiply/divide helpers and capture its
constructor/update calls: all driver indices, partner bytes 0..3, layers,
signed velocity extremes, three seeds and empty/full native pools match
particle parameters, outgoing scratch and RNG state. Actor allocation is
stubbed in that oracle, so it does not prove original pool-exhaustion
allocation behavior. Palette coverage expands to **1,280** comparisons.
The AI, damage, driving, pair-collision, surface and dirty regressions pass.

Integration uses the existing chunky point-actor renderer and leaves its
24-byte assembly ABI unchanged. Full original race-entry scratch state and
composed actor ordering remain broader fidelity checks; the isolated
emitter proof does not establish complete collision-effect fidelity.

The silent strict A1200 BUMPS regression
`amiga/.run/collision-bursts` (port 24934) passes with the integrated code:
700 updates, 40 takeoffs/landings, 429 shadow frames, peak 6384, 40 jump
sound requests, 13 wall-contact and 36 car-contact requests, and no weapon
sample requests. This is a target integration regression, not a pixel-wise
DOS burst-rendering comparison or a performance certification. The emulator
was stopped afterward; the normal launch image was not promoted.

The Amiga race path is native 68020 code. It does not execute a translated
x86 CPU context and does not use captured DOS frames. Track graphics, cars,
fonts, start lights, palettes, sound samples, and music are decoded from the
original files at run time; surface trails are generated point actors just as
they are by the original race routine.

### Current correction: contact sound selection and timing

Original `23ada..23b07` emits a contact sound only when DS:536c is nonzero
and DS:5370 is zero. It indexes loaded sample handles at `4c4f` with the
car-contact latch `4dae`: sample **5 for walls, 6 for cars**, flags 2,
priority 14. Dispatch now occurs once at the per-car update tail, after
damage consumption and before jump sound, instead of in wall substeps or
the car-pair resolver. Those early paths retain collision bookkeeping but
no longer emit sound.

The previous heading-selected wall table was a misidentification:
`20808..20828` reads selected weapon `2fac`, then indexes `0196`. Samples
10..16 must not be emitted as ordinary wall sounds in this weapons-disabled
race. The unused `06c2` bookkeeping flag is cleared after actual contact
dispatch; it is not the condition selecting that call. This supersedes
the old heading-table claim in the historical overview below.

**2,048** original instruction comparisons verify current/previous contact,
car-contact versus stale last-wall state, all headings and drivers, signed
sample handles, flags, priority and balanced stack. Only the mixer entry is
stubbed. Separate integration fixtures prohibit early substep/pair events
and retain one tail event with suppression during persistent contact.
The full AI, damage, driving, car-collision, surface and dirty tests pass.

Silent strict A1200 BUMPS run `amiga/.run/contact-sound-jumps`, port 24933,
passes at update 700: 13 wall and 36 car sound requests; no requests for
weapon samples 10..16; 40 takeoffs, 40 landings, 429 shadow frames, peak
height 6384 and 40 jump-sound requests. Emulated audio/DMA remain enabled.
This is dispatch verification, not a listening test or proof of every
mixer behavior. The emulator was stopped and the normal launch image was
not promoted.

For the current completion requirements, use
[the current queue](open-work.md#current-completion-queue). The initial
service overview below and dated/append-only findings record development
history; later original-code corrections supersede earlier approximations
and historical target checksums are not blanket current-build evidence.

## Initial implemented race services (historical overview)

- All ten 34-byte `.omi` records and all forty directional car images load
  from `SLICKS.000`. The default DOS lineup is vehicles 5, 2, 0, and 0.
  Unproved bytes 4–6 remain explicitly numbered rather than carrying the old
  speculative top-speed/response/steering names; the live speed limit and
  steering recurrence use their separately traced per-driver values. Byte 32
  is now identified as the divisor in `1000:ed64`'s four-channel impact/damage
  update rather than the former speculative AI-speed field; byte 33 remains
  explicitly unlabelled.
- The start grid is derived from each track's recorded position and heading.
- Four cars use persistent fixed-point position, velocity, speed, and heading
  state. Throttle uses the traced `0xa0` increment. The normal tyre-force
  block uses the recovered signed 32-bit numerator, four-factor divisor,
  branch-specific Q15 velocity decay, and `velocity / 20` position step.
  Coasting first applies its recovered per-driver Q15 multiplier to the drive
  scalar. Ordinary braking applies the recovered Q15 factor 8 to both velocity
  components and clears that scalar before the active-force update. The seven
  force coefficients are produced by the original quarter-step interpolation
  table from the default 13-value driver setup rather than hard-coded outputs.
  Steering now preserves the original four-stage signed integer recurrence,
  including the traced human/AI input strengths and per-driver scales; the
  semantic trace proves 9,329 literal heading transitions.
- Saved-under restoration retains the full 0--319 car X coordinate.  This is
  covered by the strict BASIC A1200 checksum: the two cars initially beyond
  x=255 no longer restore their old pixels at the wrapped x=7 position.
- The normal computer-control path follows the original ten-byte navigation
  records. Its 16-sector vector quantizer, steering thresholds, aligned-
  velocity throttle restoration, ordinary coast, greater-than-five-sector
  braking, centre checkpoint comparisons, vehicle-specific opponent
  look-ahead distances, opponent-triggered brake suppression, 700-tick
  initial grace, 150-tick stationary watch, 40-tick accelerating
  escape turn, randomized turn side, and 100-tick post-escape watch are
  recovered from `e204` and `f09d`. The
  `analyze_ai_controls.py` replay reconstructs each pre-turn heading and
  matches the recovered normal drive decision on 98.81% of 12,287 traced AI
  samples; the remaining trace rows include the special destination states
  still listed below.
- Car contacts use the recovered `.omi` extent and weight ratios. For each
  updated car, `2000:2d27..31bd` projects a point ten fixed units along its
  velocity using `(abs(vx) + abs(vy)) / 2`, tests that point against the other
  car's centre using the current car's byte-2 extent times 50, and applies the
  original integer-percent component transfer. It does not separate positions.
  Each car has the original persistent contact latch, which suppresses another
  impulse until that car completes a scan with no overlap. A forced DOS overlap
  gives current velocity `(-885,172)` and other velocity `(-863,889)` for
  weights 18 and 20; `make verify-car-collision` fixes that oracle independently
  of the full race. The same oracle covers the per-car secondary impact values
  81 and 65 recovered from `2000:30b0..317f`; the maximum is exposed as the
  current frame's collision-effect strength. Track
  contacts walk every integer centre pixel from the old to proposed position,
  snap to the last clear pixel at the original 100-unit scale, apply the
  four-neighbour `c63e` velocity transform, and then use the original fixed
  centre clamps. Class 2 takes the original non-contact dispatch. A complete
  scan of all 195 supplied tracks proves that none persists animated classes
  22–26 in its static material map; those classes must be introduced by the
  still-unrecovered runtime actor path rather than by a missing track fixture.
- The immutable mode-zero `b089` material map is reconstructed independently
  of the visible track pixels. Its four classes match the captured DOS
  `BASIC.SS` map at all 60,800 pixels; the strict A1200 gate fixes its checksum
  at `80f1987a`. A second independently layered mode-one map now also matches
  all 60,800 captured pixels for both `BASIC.SS` (`aa8bc219`) and
  `BASICTRK.SS` (`e472d3a7`). It retains the packed upper surface bits which
  cannot be recovered from the visible low-three-bit pixels alone.
- Wheel effects use the original per-car/per-heading wheel offsets and the
  recovered speed scalar `(abs(vx) + abs(vy)) / 2`. The values 61--72 in the
  live dispatch are palette indices, not archive sprite handles: the native
  path now creates the same one-pixel actors instead of incorrectly expanding
  every family into a 4x4 `savu` image. Classes 3/4 select colours 67--69;
  5/6/9/10/13/14 select 61--63; and 11/12 select 64--66 with their recovered
  30-through-49-tick first-actor lifetime. Above speed 200 the shared helper
  scatters one component around each wheel, and above 250 it adds a moving
  component with the original random 15-through-24-tick lifetime and
  -11-through-11 fixed velocity. Classes 7/8 use colour 55 above speed 300,
  lifetime 20, and -10-through-9 velocity. Sampling suppresses classes 2, 15,
  and 22 through 26. The active-particle count remains 16-bit at the 256-entry
  capacity, so filling the pool cannot make all smoke vanish.
- Each wheel on road-like material classes 0, 1, 17, 19, and 31 uses the DOS
  acceleration/brake gate. Acceleration emits below the driver's DS:4ee0
  threshold times ten; braking emits below twice that threshold. An emission
  creates moving colour 218 for 5--19 ticks plus a three-tick colour 70--72
  point. Above speed 100 it then selects sample block 2--4 with flag 2 and
  priority 10. The native calls preserve this observed random-number order.
- The live 32-entry material jump table is recovered and active. Classes 0,
  1, 17, 19, and 31 use the road-point path; 3 and 4 use colours 67--69;
  5, 6, 9, 10, 13, and 14 use 61--63; 7 and 8 use the separate moving point
  path; and 11 and 12 use 64--66 through the long-lived helper. Classes 2,
  15, 16, 18, and 20--30 have no actor dispatch.
- Four original-font HUD rows show race time and lap/finishing position.
  Checkpoint wrap records current, previous, and best lap times. A race ends
  when all four cars finish and draws an ordered results panel.
- The title menu selects the player car, every `.SS` file discovered in the
  `TRACKS` directory, and one through nine laps. The bundled run/debug setup
  exposes all 195 supplied tracks. Their navigation tables contain 7 through
  46 regions; the native decoder accepts and renders every one, with
  `make verify-native-tracks` providing an exhaustive host gate. Escape
  returns from a race and Return returns after results.
- Title-menu redraws retain the ordinary and selected colours produced by the
  recovered palette matcher. The C/assembly bridge no longer inherits
  undefined `d0`/`d1` values from its caller, so unrelated code layout cannot
  change menu colours.
- Paula channel 0 loops the engine sample selected by the original ten-entry
  vehicle-to-sample table (blocks 17, 17, 21, 22, 19, 18, 20, 18, 23, and
  24) at DOS priority 100. Its frequency uses the original per-vehicle base
  and slope tables applied to `(abs(vx) + abs(vy)) / 2`; the result is
  converted to a PAL Paula period. The full 26-block
  bank is indexed in one pass. Channels 1 and 2 play natural-length effects:
  a new car contact dispatches block 6, while track impacts select blocks
  10--16 from the current 16-way heading through the original DS:0196 table.
  Both use DOS flag-2 duplicate suppression and priority 14. Ordinary lap
  wraps use block 25 at priority 18, entering the final lap uses block 8 at
  priority 19, and the first finisher uses block 9 with flag 2 at priority 30.
  Paula reloads forever while DMA remains enabled, so each effect and the
  channel-3 `intermed.wav` results cue repoint its reload registers to a
  two-byte chip-RAM silent word after the initial body has been latched. The
  handoff is deferred for two audio updates: a fixed two-raster-line delay is
  shorter than the first DMA-request interval at the samples' period and can
  accidentally leave the sample itself armed as the reload body. Effects and
  the results cue therefore play once; only the engine intentionally loops.
  Chip allocations and DMA are released before AmigaOS is restored.
- Live painters merge changed areas into a fixed rectangle list. Horizontal
  bounds are rounded to 32 pixels for Kalms, packed into the already-allocated
  title workspace, and converted at their native screen position; unchanged
  areas are skipped. One-pixel particle changes and changed timer-glyph pixels
  are synchronized directly into the interleaved planes, avoiding tiny C2P
  calls. The live race
  has one authoritative 64,000-byte linear painting surface after setup; the
  four-bank VGA store is retained for translated setup code and synchronized
  only when a diagnostic logical checksum is requested. A direct four-bank
  source was tested on the strict 68020 target: pixel painting was within one
  millisecond of the linear path, but gathering banked rows before an
  unmodified Kalms transpose added roughly ten milliseconds at the busy
  700-frame checkpoint. A fused banked-source transpose was subsequently
  proved pixel-exact, but its per-word bank permutation made C2P alone take
  624 raster lines on the 68020, so the linear surface remains substantially
  faster. The ordinary 200-frame checkpoint now updates only changed HUD
  glyph cells and uses pointer-stepped car save/restore and rotation on that
  surface. A 68020 particle pass advances, expires, compacts, and rebuilds
  priority buckets in one traversal. The ordinary 200-frame checkpoint now
  takes 173 raster lines (about 11.1 ms), down from 474 (about 30.4 ms). The
  particle-heavy 700-frame checkpoint takes 293 lines (about 18.8 ms) with 98
  live particles. Both measurements include diagnostic phase probes, and both
  strict A1200 gates enforce the 312-line PAL-frame ceiling.
- The strict FS-UAE target is a stock 2 MiB A1200 (`fast_memory=0`). The race
  runtime—including its embedded particle array—and both CPU-side surfaces
  use `MEMF_ANY`, so they necessarily reside in Chip RAM on this target; only
  display bitmaps, copper lists, and Paula sample bodies explicitly require
  `MEMF_CHIP`.
- The archive directory is read once per open. A BASIC session now reads about
  269 KiB in total instead of performing thousands of 19-byte directory reads.

## Evidence and remaining fidelity work

Skid-mark retirement correction (2026-09-23): the DOS road point at
`2000:26d6` and short stationary surface point at `1000:e934` set actor
state `+1a=5`, with countdown `+3e=3`. Expiry negates the state at
`3000:3945`; states <= -5 bypass saved-under restoration at `3e36`.
The retirement pass redraws states -6/-7 through `3673`, then frees -7.
Consequently these pixels persist after their actors are freed; the native
three-tick unconditional erase was incorrect. Native retirement now commits
these marks to the authoritative chunky surface after all saved-under layers
are unwound. The 30..49-tick stationary variant and moving particles remain
transient. No additional framebuffer or permanent-particle pool is needed.
The surface-effect test covers expiry and an overlapping transient's later
restoration. The Amiga build and collision/physics/dirty-list tests pass.
The silent strict A1200/2-MiB run reached frame 700 with unchanged car
positions, logical checksum `3d08d41f`, display checksum `30ad3861`, and zero
audio-blanking spills. An independent decode of the captured live bitplanes
matches all 64,000 authoritative chunky pixels. Retirement currently raises
the busy checkpoint to 315 raster lines (about 20.2 ms), versus the required
maximum of 312. Avoiding duplicate dirty entries preserves both checksums
but still measures 315. Integrating retirement into the existing assembly
particle pass reduces the measured checkpoint to 311 lines (about 19.9 ms),
with both checksums and all car positions unchanged, and no audio-blanking
spills. The timing gate stays at 312; the lap gate's image expectations now
include persistent marks. This is a checkpoint measurement, not a maximum
over the entire race. The production 68020 routine also passes 16 Unicorn
expiry/compaction/dirty-capacity cases; its structure offsets are checked by
the target compiler. Only the dirty-list capacity fallback retains a separate
retirement scan. Visual confirmation and broader regression runs remain
pending. This does not yet prove exact DOS page-parity
retirement timing or all actor masking cases.

Native framebuffer inspection at frames 700 and 900 confirms the earlier
trails remain while cars move on. Both diagnostic images were rendered from
the target's chunky surface and live palette, not used as game assets.
The full 768-byte live palette equals the existing DOS
`tmp/pc-fixed/basic-race-palette.bin` capture, including grey indices 70..72;
their light appearance is not evidence of a palette-upload mismatch.
Broader frame-time profiling is deferred at the user's request; the opt-in
MEASURE diagnostic is available but has not been run.

Particle coordinate fidelity: `3000:3982/398d` add velocity as a 16-bit
word; `39af/39ca` use arithmetic right shift by six to obtain screen
coordinates. Native advance now wraps the word and sign-extends it, and
drawing rounds negative fractions down instead of leaking them onto row or
column zero. Verified with 70 production-68020 signed-boundary cases and
126 host-side negative-fraction clipping cases. The Amiga build, dirty-list
tests, 16 retirement cases and 1,394 display-writer cases pass.

Next actor fidelity dependency: `2000:5129..5145` installs DS:05b8 as a
linear actor occlusion map. The point renderer at `3000:3843..386f` compares
its raw byte with actor `+3b`; zero `+3b` bypasses masking. Wheel emitters
derive that threshold from DS:5388[driver] * 15. This is NOT simply the
decoded five-bit material class: `1000:b1ab..b1cd` packs class << 3 with
three low bits, while `b089` mode zero shifts the stored byte right by three.
Neither decoded class alone can be substituted for this comparison.
The raw byte is exactly `(material_map[pixel] << 3) |
(surface_map[pixel] & 7)`: freshly decoding BASIC.SS and comparing with the
existing DOS `slicks-track-chunky.bin` capture gives zero mismatches across
all 60,800 playfield pixels. Reproduce with `make build/verify_actor_mask`
and `build/verify_actor_mask ref/SLICKS.DAT ref/TRACKS/BASIC.SS
tmp/pc-fixed/slicks-track-chunky.bin`. No extra mask allocation is needed.
Driver layer transitions still require recovery before adding foreground
masking to cars/particles or permanent-mark retirement; the BASIC map match
does not prove those transitions or map fidelity for every track.

The strict 2 MiB A1200 gates cover title startup, 200 live race frames, a full
lap, the 25-region alternate track, the object-heavy `ICE.SS`, the maximum
46-region `HEIKKI30.SS`, all four finishers, the displayed results frame,
engine audio, results music, and clean system restoration. Host-side
differential tests cover the native graphics primitives and exhaustively
decode all 195 supplied tracks.

The implementation is playable and complete as a race loop, but these details
still require instruction-level recovery before calling the simulation
bit-exact:

- exposing non-default values for the recovered driver setup, the
  contact/reverse brake branches, the trigger for the special car-state path,
  and the remaining
  `.omi` property semantics;
- the special destination AI modes beyond the recovered normal path,
  opponent probe, and stationary recovery cadence;
- animated boundary classes 22–26 and rendering the recovered collision-effect
  strength through the original actor system;
- the remaining exact sound-event selection, priority, pitch, and duration
  cases beyond the recovered engine, lap, finish, collision, impact, and
  road-slip paths.

`tools/patches/dosbox-x-slicks-race-state.patch` provides the semantic DOS
trace used to compare controls, navigation state, positions, headings, and raw
54-byte per-car state without introducing an emulated CPU into the Amiga build.
Its paired integrator hooks also preserve the velocity entering and leaving
each update together with the seven drive coefficients and ten Q15 factors;
the force hooks now also preserve both signed division operands and results.
Across 27,030 traced X/Y updates, `tools/analyze_race_velocity.py` proves the
closed recurrence with no mismatches: the force numerator is the low 32 bits
of `direction * drive_scalar * 200`; its divisor is the low 32 bits of
`drive3 * drive0 * (state20 / 70 + 10) * (23 or 38)`; and the previous
velocity is decayed by branch Q15 factor 4 or 3 over `32768 + drive1` before
the force is added. The coast branch first applies Q15 factor 5 to the drive
scalar. Ordinary braking applies Q15 factor 8 once per elapsed simulation
quantum before clearing the drive scalar. The opt-in
`dosbox-x-slicks-special-state.patch` fixture additionally proves that positive
car-state word `+29h` bypasses the normal integrator and multiplies both
velocity components by the zero-extended Q15 factor 7. Its countdown/target
maintenance and the event that enters this state are mapped, but the entering
event still needs a natural DOS capture. The native runtime now uses the
recovered normal-state block, including two-quantum throttle and braking at
the native 50 Hz cadence; dynamic setup choices, contact/reverse braking, and
that special-state trigger remain to recover.

The actor-page investigation exposed and corrected a scheduling gap: native
`update_car` used `timestep=2` for throttle/braking but performed just one
force/position integration. DOS `0dd1..14dc` repeats that integration for
each elapsed BP-2 tick before the once-per-outer-update surface/layer/actor
work. Native force, position, track contact, centre clamping and normal
coasting decay now run twice per fixed two-tick update. Controls, steering,
surface emission, car collisions, lap bookkeeping and timer advancement
remain once per batch. Full-update host fixtures cover both the positive
special branch and ordinary coast, including intermediate positions and
the once-per-batch timer. The Amiga build and focused host tests pass.
Track-contact latch persistence across substeps is also implemented and
covered for all four two-tick hit/clear combinations. This does not prove
whole-update equivalence: live elapsed-tick tracing and complete DOS phase
ordering still need verification. See `actor-layers.md`; no arbitrary speed
scale was added.

The first silent A1200 run with this correction reaches a complete, drawn
results screen at frame **445**, all four finished, positions **4/3/2/1**,
logical checksum `dee9b678`, display `4a907ebf` (`.run/two-tick-physics`).
The old strict results gate correctly fails against its frame-618/order
baseline. Its expectations have not been rewritten, and this experimental
build has not replaced the normal launch-directory executable. Validate
the new timing and remaining substep contact semantics before promoting
the new checkpoint; reaching results alone is not a fidelity proof.

The next build replaces the fixed-two-tick assumption with a rational PIT
accumulator at the captured 100% speed / nominal 50 Hz configuration (see
`actor-layers.md`). It passes a million-update phase/total test and explicit
one/two-tick force/position fixtures. The silent A1200 run
`.run/rational-physics` reaches complete results at frame **486**, all four
finished, positions **4/2/1/3**, logical `00c2ad54`, display `7694f9c7`.
This supersedes the experimental fixed-two-tick frame-445 checkpoint, not
the normal-launch binary or old strict frame-618 gate. The latter still
fails deliberately; no new baseline has been accepted as DOS-equivalent.

Steering now also applies the elapsed-tick multiplier present at DOS
`2000:0cd6` and `0d4c`, after the final signed `/50`. Previously the native
update rounded the steering amount correctly but applied only one tick's
turn per batch. The focused verifier covers all batches 1..45 with left,
right, both and neither input (180 cases), including heading wrap and a
fractional division result that distinguishes multiply-before-rounding
from the original multiply-after-rounding order. This covers human-input
steering fixtures, not complete AI or whole-race DOS equivalence. The
focused physics, surface, car-collision and dirty-list tests and Amiga
build pass; normal-launch promotion and the old results gate remain
unchanged pending broader timing verification.

The separate silent A1200 `.run/steering-ticks` run reaches complete, drawn
results at update **416**, all four finished, positions **2/1/4/3**, logical
checksum `4010e0a0`, display `7d9a3a49`. The old frame-618 gate fails as
expected. This is an execution check, not proof of whole-race equivalence.

`make verify-dos-steering` now executes the original left-steering arithmetic
instructions (`2000:0c79..0cd6`) from `disasm/runtime.bin` under Unicorn,
without substituted arithmetic services. It checks 576 combinations of
human/AI input magnitude, scale, penalty, property and elapsed ticks.
Native steering matches all cases; the previous wide-intermediate formula
fails 211. The fix preserves the low signed word before each divide, as
required by the original `IMUL; ...; CWD; IDIV` sequences. Extreme property
and scale fixtures exercise machine arithmetic, not claims about supported
menu values. This test checks the delta and balanced stack, not the full
control/damage path, AI choice, or full race.

The `2000:0d58..0da2` heading adjustment is now implemented:
it uses signed byte DS:3057, word DS:3053, elapsed ticks, and a signed
32-bit speed comparison against 30. DS:3057 is initialized to zero at
`1000:c14c` and assigned a random sign only when previously zero at
`1000:edfd..ee29`; the same routine increments the four words beginning
DS:304f with an upper clamp of 999. Its producer and car-impact caller
are now connected as described below; other damage consumers still
need implementation.

`make verify-dos-damage` executes the complete original `1000:ed64..ee7f`
routine, including the real 32-bit multiply/divide and RNG helpers, and
compares native `apply_damage` in 576 cases. Coverage includes both
enable gates, nonpositive impulses, signed dword multiplication wrap,
separate resistance/coefficient/100/2 arithmetic, four signed-word
channel additions and upper clamps, RNG advancement only on the first
eligible call, and the far-return stack balance. Ten native tail fixtures
cover the `(impact-300)/4` threshold and consumption exactly once for the
current driver without consuming the other driver's pending impact.

The native car-pair resolver records pending impacts separately from
diagnostic magnitudes, and the per-driver tail consumes them following
`2000:399f..39d6`. Steering now reads damage channel 3 rather than an
independent always-zero penalty. Defaults remain damage scale/master
gate zero, matching DS:3026 and DS:36a6 in the captured
`tmp/pc-layers/slicks-race-data.bin`; no menu option is falsely claimed
implemented. Other producers (`1000:f082`, `2000:205f`), damage-induced
yaw timing in full DOS race traces, other damage consumers, repairs and
menu setup remain open. The yaw
speed operand is the previous measured `(abs(vx)+abs(vy))/2` at DS:684e
(written at `2000:21fc..2243`), not the longitudinal throttle scalar.

The silent A1200 `.run/damage-producer` regression reproduces update 416,
positions 2/1/4/3 and both `4010e0a0`/`7d9a3a49` checksums exactly with
damage disabled. All four cars finish and results are drawn. This confirms
no change to that experimental checkpoint, not damage-enabled A1200
equivalence; the old frame-618 gate still fails and is not rewritten.

Native damage yaw now applies channel 2 times the persistent signed
direction and elapsed ticks, retaining low signed-word multiplication
before division by 70. It runs after input steering and before heading
normalization/integration. A separate `measured_speed` field retains the
prior update's velocity magnitude until refreshed after integration and
before car-pair collisions; it is not the throttle scalar. The steering
oracle executes the actual `2000:0d58..0da2` block in 1,080 cases covering
both signs/zero, the 30/31 threshold, signed dword speed comparisons,
word overflow, elapsed ticks and nonzero driver indexing. Four full native
two-update fixtures verify old-versus-new speed timing and heading wrap.
These pass with damage, steering, collision, surface and dirty-list tests;
the Amiga build succeeds. This last yaw change has not yet had a
damage-enabled A1200 run or full-race DOS comparison.

The former always-zero `tyre_load` field was actually another disconnected
copy of damage channel 0. Native force now reads `damage[0] / 70 + 10`, as
the four DOS X/Y powered/coast branches do at `0ed3`, `0fc2`, `1084` and
`1173`. The duplicate field is removed. Eight one-tick full-update fixtures
cover damage 0/69/70/999 for powered/coast force: X contributions are
53/53/48/22 and 84/84/76/35 respectively with the fixture's coefficients;
Y remains zero. These fixtures check this recovered connection, not every
force operand or damage-enabled race equivalence. Brake/reverse state,
the fully-damaged reverse-drive gate at `067a..06a5`, and other damage
consumers remain open.

Silent A1200 `.run/damage-force` (including the new yaw path) again reaches
complete results at update 416, positions 2/1/4/3 and unchanged logical/
display checksums `4010e0a0`/`7d9a3a49` with damage disabled. The historical
618-update gate remains intentionally unchanged and fails; this is not
a damage-enabled A1200 validation or a new accepted fidelity baseline.

The ordinary no-contact brake/reverse branch `2000:05bc..06a5` now has
its recovered DS:5374 forward-drive latch. Throttle sets it to 1; braking
then applies unsigned Q15 factor 8 per elapsed tick and clears the drive
scalar. Measured speed below 100 plus nonzero DS:4ee4 clears the latch,
arming reverse on the following update. With the latch clear, damage
channel 0 below 999 permits the signed-word `-maximum_speed * 33` reverse
scalar; at 999 the existing scalar is left unchanged. DS:4ee4 is the
vehicle byte also used for engine sound (.omi byte 25), not a newly
invented reverse option. Race initialization clears the latch as DOS does.

The original-code damage verifier also executes this whole brake branch
and its real multiply/shift helpers: 432 cases cover both latch states,
speed -1/99/100/65536, zero/positive/negative vehicle property, damage
998/999, three biases (including an unsigned factor above 7fffh), and
1/2/45 ticks. Native velocity, scalar, latch and stack behavior match.
A native multi-update fixture covers brake, reverse force integration,
then throttle rearming. The surrounding contact and special-state entry
guards, reverse-specific rendering, and dynamic maximum-speed refresh
remain open; this branch test does not prove those are implemented.

Silent A1200 `.run/brake-reverse` reaches the same complete update-416
results (2/1/4/3, `4010e0a0`/`7d9a3a49`). No new baseline or normal-launch
promotion was made; the historical results gate continues to fail.

Special-state control guarding now follows `2000:03f2..03ff`: every
nonzero state skips throttle, braking and input steering, but damage yaw
still runs. Positive states retain Q15 factor-seven velocity damping;
negative states follow `0e5e..0e9e`, decaying the drive scalar with the
unsigned coast factor and arithmetic shift, then bypassing normal force
and velocity damping. Both continue into position/contact integration.
Thirty-two two-tick full-update fixtures cover every input combination
for positive/negative states, with damage yaw still active. A negative
drive-scalar fixture verifies arithmetic-shift rounding. The original-code
verifier additionally executes `0dd9..1230` and real arithmetic helpers
in eight signed-state/scalar/bias cases, matching native drive scalar and
both velocity components. Focused regressions and the Amiga build pass.
The state producer/lifetime and surrounding race-state guards remain
open; these forced fixtures do not demonstrate naturally triggered
special states or full-race equivalence on A1200.

The special-state lifecycle `2000:3cb1..3d3e` is now implemented at the
per-driver tail after damage and before contact clearing. DS:305a is a
separate signed target. With target zero, positive state decays by the
low signed word of `266*ticks`, clamped to zero if it goes negative.
State below target increases by that step without an upward clamp; once
state reaches/exceeds target the target is cleared. The native sequence
for target 700 and one tick is 266,532,798,532,266,0. The original-instruction
oracle matches 150 state/target/tick combinations, including signed-word
overflow; the multi-update sequence fixture and existing physics, damage,
steering, effects and collision tests pass, as does the Amiga build.
Initialization resets both words. The target producer remains untraced:
the current reachable listings expose initialization and consumption but
no nonzero direct assignment. No guessed trigger has been introduced,
and naturally triggered special-state A1200 verification remains open.

The missing target producer is now recovered from bytes omitted by the
reachable listings. Scanning the full image for the DS:305a operand found
stores at image offsets 13687/136e3 (instructions at physical 23787/237e3).
The relocated application CS is 1987h; its CS:a8c4 switch table dispatches
effective surfaces 13 and 14 to physical 23737 and 23793. Above measured
speed 350, they set a signed-word target to `(speed/20)*omi[3]` and
`(speed/30)*omi[3]` respectively. Surface 14 also divides both velocities
by two with signed truncation. The producer runs after pair collisions
but uses the earlier measured speed, matching the original phase order.

Native layer selection now retains DS:5378 as `effective_surface`, forcing
it to zero during nonzero special state. The recovered surface producer
is connected before the per-driver damage/lifecycle tail. The oracle
executes the real switch table plus complete case bodies in 36 cases,
checking thresholds, zero/extreme unsigned vehicle property, target word
wrap, signed velocity division and the original sound-request flag.
Two full native multi-update fixtures trigger the state from actual map
selection and verify velocity change, progression and retrigger suppression.
The sound request is not yet submitted natively: DS:4c51 mapping/dispatch
and the airborne actor-drawing path still need recovery. These are not
claimed complete by the physics tests. A1200 coverage of these surfaces
also remains open.

Jump sound dispatch is now connected: DS:4c51 is slot 7 of the loaded
sample-handle table at DS:4c4a (the sound tracer uses this same mapping).
Native takeoff queues block 7, flags 2, priority 12 after damage consumption
and before special-state advancement, matching `2000:3c94..3cae`. The
original-code verifier executes this dispatch in four pending/handle cases,
checking sign extension, flag clearing, arguments and balanced stack; only
the mixer is replaced with RETF. It clears the reused emulator translation
covering the next block so the earlier lifetime test's compiled block cannot
run past this test's stop boundary. Native multi-update surface fixtures
verify one queued event at takeoff and no repeat while airborne. Tests and
Amiga build pass; audible playback and surface-specific A1200 validation
remain open, as does airborne actor drawing.

Airborne drawing recovery: `2000:3ebf..3f45` does not raise the main car
sprite. For signed state >500 it updates a separate persistent actor with
X equal to DS:53b6 and Y equal to DS:53be + state/500. Its arguments are
zero motion, animation delay 33, sprite frame 0, lifetime 3, no occlusion mask,
state 3 and initial priority 3; the caller immediately sets priority 0.
The actor was allocated at physical `1fc80..1fc9d` from the driver's
DS:4f44 sprite source and its handle stored in BP-18[driver]. Initialization
positions it offscreen. Normal car submission remains at DS:53b6/53be.

The original-code verifier now covers six signed-state/threshold cases
through the full dispatch block and checks all 13 arguments, the priority
override and stack balance. Only the actor update/render service is stubbed;
this proves the request, not its rendered pixels. Native shadow rendering
is still unimplemented. A faithful implementation must account for the
state-3 lifetime/retirement path, priority-0 ordering before later allocated
particles, and saved-under restoration; simply drawing a second car while
state>500 would miss the original actor lifetime after the last refresh.

Shadow retirement is now checked across successive passes, not merely
single-state fixtures. `verify-dos-particle-expiry` runs the original expiry
and retirement fragments over two 12-pass sequences (both initial page
phases), including reactivation of the same slot after retirement. A
state-3 shadow expires to -3; retirement decrements it to -4 then restores
-3, so its slot is retained, not freed. Refreshing the reserved slot to
state 3/lifetime 3 reactivates it. Expiry at `3000:3918` precedes drawing
and the page toggle at `3e0a`; the page-sensitive delay must use that old
page phase, not the phase after the toggle. These tests do not render
pixels or prove saved-under restoration. Native drawing still needs that
integration; no approximate shadow has been introduced.

The shadow source is now recovered and executed, not inferred from the
actor-call arguments. `1000:c3fc..c554` constructs DS:4f44 from the first
two OMI bytes (DS:4e84/4e88). With `n = radius_x + radius_y`, its stored
width is `ceil(n/4)*4` and height is `n`. The nontransparent square is
`radius_y <= x,y < n`; alternating pixels satisfy
`((x ^ y ^ radius_y) & 1) == 0` and contain palette index **37**, with all
other visible pixels zero. It is not a silhouette copied from the car.
`verify-dos-damage` now executes the complete original constructor and
its actual planar pixel writer at `2000:fbca`, checking every visible
pixel, padding, header and return stack for 225 positive-radius pairs
(1..15 each). This verifies generated source pixels, not final screen
clipping, actor ordering or saved-under behavior.

The previous “colour 33” interpretation of actor argument 7 was incorrect
and is corrected above. For sprite actors, +23 is the signed animation
delay: `3000:39df..3a83` compares the +3c counter with it, advances +24
and selects the next source pointer through +26; `3a8f..3a96` increments
the counter when the delay is nonzero. The unmasked sprite renderer at
`3673..36dc` passes only position, source and destination to `aa7c`—no
colour override. Native shadow integration remains open; the newly
verified pattern must be used instead of a solid colour-33 car mask.

Native shadow integration now uses that recovered pattern in the
authoritative chunky surface. Four reserved shadow records retain their
position and page-sensitive lifetime after the last refresh; expiry to -3
does not discard/reallocate a particle slot. They restore in reverse driver
order after priority-zero particles and draw in forward order before those
particles and cars. Both paint and restore mark dirty rectangles, so C2P
sees the same source pixels. The original car sprite is not raised.
All ten shipped OMI records have radius_x 5 or 7 and radius_y 1; saved-under
storage also supports sides up to 16. Screen edges are safely clipped.

Verification now compares 450 native whole-screen outputs against pixels
generated by the real DOS constructor, across 225 positive-radius pairs
and both logical/chunky-authoritative paths. It also compares native expiry
against two 12-pass original-code sequences, including reactivation, and
checks four overlapping shadows, five interior/edge placements, exact
background restoration and dirty coverage. Physics, surface-effect,
collision and dirty-tracking regressions pass; the 68020 build succeeds.

The diagnostic-only `JUMP` argument (debug.sh `SLICKS_SHADOW_TEST=1`) forces
a stationary airborne car, then removes its airborne state. It leaves
normal startup unchanged. `diag_shadow.gdb` checks the program-side draw,
expiry and restore assertions on an A1200 with 2 MiB chip RAM and no fast
RAM, with host sound muted and emulated audio intact. Debugger-side state
assignments did not take effect in the initial attempt, so the fixture is
injected by the program itself instead. This is a controlled renderer test,
not proof of natural jump trajectories or whole-race DOS equivalence.
The normal-launch `.run/dh1/SlicksDiag` is not replaced by these isolated runs.

The completed A1200 fixture reports
`SLICKS_SHADOW_OK FRAME=5 CHECK=2 AUDIT=1`. In addition to chunky-state
assertions, it reconstructs the sampled display pixel from all eight real
bitplanes after C2P on frames 1 and 5, confirming colour 37 on draw and the
saved background on restore. The per-frame bitmap audit finds no writes
outside declared dirty regions. Natural surface-13/14 jump-track replay and
exact general DOS actor-slot/page behavior remain open.

### Natural BUMPS jump regression and map corrections

The full 195-track scan now reports jump classes on both decoded layers.
No supplied track has static class 13; 33 contain upper-layer class 14.
BUMPS has 2,104 such pixels. A diagnostic-only `UPPERJUMP` startup selects
the real `BUMPS.SS`, keeping the normal starting grid, countdown, AI,
properties and physics. Unlike `JUMP`, it injects no car position, speed,
special state or track-map values. The completed silent 2-MiB/no-fast-RAM
A1200 run reports:

```
SLICKS_JUMP_TRACK FRAME=700 ZONES=18 TAKEOFFS=51 LANDINGS=51 SHADOW_FRAMES=511 PEAK=6384 SOUNDS=51
SLICKS_JUMP_TRACK_OK
```

`SLICKS_JUMP_TRACK=1 ./debug.sh "$KICKSTART" diag_jump_track.gdb` reproduces
the invariant gate. Shadow frames count individual cars, not unique display
frames. Sound counts are queued sample-7 requests, not an audible-playback
comparison. The normal launch binary was not promoted.

An independent original-DOS trace uses the same unmodified track bytes
(SHA-256 `ba9e7ed6220bb363bf6619f635ecdd80e5ce95e0df9e7fd3d1634f92d41e7782`),
with special-state injection explicitly disabled. The reference Makefile
now accepts `REFERENCE_TRACK`, preserving BASIC as its default. Capture:

```
env -u SLICKS_TRACE_SPECIAL SLICKS_TRACE_PAGES=1 make reference-layer-trace \
  REFERENCE_FIXED_ROOT=tmp/pc-bumps-jumps \
  REFERENCE_TRACK=ref/TRACKS/BUMPS.SS REFERENCE_KEY_PACE=2
```

The stronger map comparison initially found 3,810 raw collision-mask
differences despite the natural-jump smoke test passing. Recorded original
construction writes exposed missing mode-zero bridge classes for types
39/40/65 (source palette 216→0, 217→1, 218→31, 26/219→2), a type-13 road
overlay clearing bridge material, missing surface types 21 and 26, and
surface-priority handling. Original `1000:b34e..b35f` makes category-one
surfaces preserve pre-existing classes 17 and 2; observed category-two
edges can replace the tree surface. These corrections are now in the
decoder. `analyze_track_map_writes.py` joins captured writes to unambiguous
source pixels for diagnosis and excludes ambiguous overlapping placements;
it does not supply assets or runtime map data to the native game.

After correction, all 60,800 BUMPS raw-mask bytes AND all 60,800 full
five-bit upper-surface values match DOS. Both BASIC maps still match;
BASICTRK's available upper-surface fixture also remains exact. The layer
replay now checks effective surface as well as layer. Its optional DAT/SS
arguments decode the maps instead of substituting logged DOS inputs:

```
build/verify_actor_mask ref/SLICKS.DAT ref/TRACKS/BUMPS.SS \
  tmp/pc-bumps-jumps/slicks-track-chunky.bin \
  tmp/pc-bumps-jumps/slicks-track-surface.bin
build/verify_actor_layers tmp/pc-bumps-jumps/layers-paced.log \
  ref/SLICKS.DAT ref/TRACKS/BUMPS.SS
```

All 6,695 captured transitions and both map inputs at every captured position
match, including 635 special-state samples, 71 entries, 67 exits and 46
contact samples. No out-of-bounds samples are skipped in this trace. The
corrected A1200 build repeats the natural-jump result above; host driving,
surface-effect, collision and dirty-tracking checks pass. This establishes
these map and transition contracts, not identical DOS/native trajectories,
AI decisions or wall-clock timing. Other object/category combinations and
dynamic track changes remain subject to broader recovery and verification.

### Original-instruction route-controller comparison

`make verify-dos-ai` now executes the complete DOS `1000:e204..e471`
routine and its real division/direction helpers. It compares the native
ordinary route decisions from the same target, heading, saved measured
speed and velocity, and checks the original return stack. Before correction
the first mismatch was target east, heading 1, speed 0: DOS accelerated
without steering (mask 1), while native also steered left (mask 5).

The native controller now follows `e31b..e34a`: divide heading by 1200
before subtracting the target direction. Sub-sector heading differences
must not produce steering. The original +4 direction convention is retained
until the signed difference is wrapped, preserving the exact +/-8 turn
choice for opposite directions. It also uses saved DS:684e measured speed
rather than recomputing it from velocities after collisions. `e2a2..e302`
first computes `speed/10`, then divides each velocity by that rounded value;
`velocity*10/(speed+1)` was not equivalent and is removed.

All 20,480 complete-routine cases now match: eight route targets, all 16
heading sectors at offsets 0/1/600/1199, speeds 0/700/701/709/1200, and
eight velocity cases including independent saved-speed values and signed
direction-threshold cases. At speed 709, velocity (910,1330) normalizes to
(13,19), not (12,18); this crosses a direction threshold. Driving, surface,
collision and dirty tracking regressions pass, and the 68020 build succeeds.

Scope: ordinary routing with initially cleared control bytes, no active
recovery or predicted contact. This is not a claim that the surrounding
`f09d` state machine, stuck/recovery timers or alternate-target path are
complete. The watchdog/escape corrections below cover two additional
blocks, not the entire caller. The old percentage-based
`analyze_ai_controls.py` heuristic is not the authority for these decisions.

The route-correction A1200 runs (before the recovery change below) passed
the natural BUMPS jump gate at frame 700: 41 takeoffs, 41 landings,
418 shadow/car frames, peak 5320 and 41 sample-7 requests. The isolated
results run reached all four finished cars and drew results at frame 440,
positions 4/2/3/1, checksum `4fd4c2eb`, display `07215263`. It **failed**
the retained older exact frame/order/checksum gate; no golden was changed.

### Signed AI watchdog and escape timing

The native watchdog now implements `1000:f0d8..f162`: subtract the signed
low byte of elapsed DOS ticks, trigger only below zero, enter state 2 with
timer 40 and watchdog 50, reload 150 after integer-position movement, and
hold 100 only for service-state >=2 (not for all escaping cars). Nonzero
service-state becomes -1 on timeout. Signed 16-bit wrap is retained.

The escape output now implements `f71c..f7dd`: use the previous signed
direction to steer this update, then choose a new direction only when its
separate timer is <=0. Selection is `rand()*2/32768`, not `rand()&1`;
reload that timer to 50 and subtract elapsed ticks from both timers. A
negative state timer returns to state 0, reloads 300 and clears the turn
timer, but still emits this update's accelerating-turn controls. No random
number is consumed merely by entering escape. The actual physics tick
count is now passed into AI instead of decrementing once per native frame.

`make verify-dos-ai` executes unmodified original instruction slices with
real RNG/division helpers: 6,720 watchdog and 16,800 escape cases match
native state, controls and RNG. Coverage includes all four driver indices,
subpixel/whole-pixel motion, signed service/direction states, signed timer
wrap and exact-zero boundaries, and elapsed byte values 0/1/2/3/127/128/255.
All 20,480 ordinary-route cases, driving physics, surface effects,
car-collision and dirty tracking checks also pass; the 68020 build succeeds.

Silent isolated A1200 verification (`.run/jump-ai-recovery`, 2 MB chip,
zero fast, PAL) passed `SLICKS_JUMP_TRACK_OK`: frame 700, 18 zones,
41 takeoffs/landings, 418 shadow/car frames, peak 5320, 41 sample-7
requests. This natural-driving run does not prove the escape state was
entered; the instruction-slice oracle supplies the recovery coverage.
The normal launch directory was not overwritten.

This does not establish the full `f09d` controller: alternate-target
selection, route-probe transitions and service/damage requests still need
recovery. Control-byte persistence also needs caller-level verification:
`99cf` skips human-input writes for AI drivers and `9caf` records input
edges without clearing the controls. The ordinary-route oracle currently
starts with cleared controls and must be extended to actual multi-update
state before claiming complete route-chain fidelity.

### Persistent route steering

The input-latch gap above is now corrected for ordinary routing and escape
outputs. `e3ae..e3de` overwrites both steering bytes only when the signed
sector error is nonzero; error zero preserves both previous bytes, even
if both were set. Accelerator/brake are recomputed on every route call.
The native car retains these inputs separately from the temporary control
mask suppressed during jumps/finished-car processing. Escape outputs also
update the latch, so returning to the route controller preserves their
steering until the next nonzero sector correction.

`verify-dos-ai` now exercises all 16 initial control masks for every route
case, and executes original `99cf` AI input bookkeeping before the complete
`e204` routine. All 327,680 cases match native output and saved latch; the
23,520 watchdog/escape cases still pass. This verifies the input-bookkeeping
and route contracts, not the still-incomplete intervening `f09d` probe and
alternate-target state machine. Physics, surface emission, car collision
and dirty tracking regressions pass and the A1200 build succeeds.

Silent A1200 run `.run/jump-ai-latch` passed the unchanged natural BUMPS
gate at frame 700: 37 takeoffs, 37 landings, 364 shadow/car frames, peak
5320 and 37 sample-7 requests. The changed counts reflect changed steering;
the test asserts jump/render/sound invariants, not DOS trajectory identity.
Normal launch files were not overwritten.

### Route-region coordinate correction and separate lap checkpoints

The route-region predicate now uses unsigned 16-bit sprite-origin
coordinates (`x/100-3`, `y/100-3`), with inclusive bounds, matching
`1000:f1fb..f277`. It previously used signed centre coordinates. The
original-instruction oracle stops at the actual inside/outside branch
destinations and compares 2,352 combinations: all four cars, ordinary,
zero-edge and inverted regions, negative/large coordinates, both edges
and nearby subpixel positions. All pass, along with the route/recovery,
physics, surface, collision and dirty-tracking regressions and target build.

Important remaining architecture correction: DS:6902 is the AI route
index, NOT the lap checkpoint index. `2000:27b6..2840` separately compares
sprite origin +4 against DS:6366/642e/64f6/65be checkpoint rectangles and
increments DS:305c. `2a61..2a8e` then requires surface 17 (selected or
upper surface) AND completed checkpoint count DS:6364 before recording a
lap and resetting DS:305c. Native `advance_waypoint` still conflates the
route wrap with lap completion and runs after movement, whereas original
`f09d` route advancement occurs before physics. The origin predicate fix
does not resolve these larger flow differences. Recover checkpoint-table
construction from track loading, then split lap/checkpoint accounting
from AI routing; do not adjust golden finish times to conceal this gap.

Silent isolated A1200 `.run/jump-ai-origin` passed the existing BUMPS
700-frame invariant gate: 34 takeoffs, 34 landings, 383 shadow/car frames,
peak 6916 and 34 sample-7 requests. This is not a lap-fidelity proof.

### Independent checkpoint decoding and lap completion

Recovered loader `live-listing.txt` `1000:ba19..ba89` identifies the
six-byte records between placed objects and AI regions as lap checkpoints:
BE16 x0, byte y0, BE16 x1, byte y1. These are now decoded into a separate
100-entry bounded array (the original DS tables have 100 words each),
instead of skipped. Every supplied track still decodes successfully.

AI route advancement now happens inside ordinary AI control processing,
before physics, and only changes the waypoint index. It no longer changes
lap/time/results state. Human driving does not need to visit AI regions.
The independent per-car checkpoint index advances at most once per update
using unsigned inclusive sprite-origin +4 coordinates. Completing those
checkpoints only enables the finish-line test; it does not award a lap.

Correction to the earlier layer description: `2981` reads mode ONE into
DS:5380, and `29ab` reads mode ZERO into DS:5384. The actual `2a61..2a8e`
finish gate accepts selected DS:537c **or raw mode-zero DS:5384** equal to
17, provided DS:305c >= checkpoint count. Native keeps the selected value
before special-state suppression and checks the independent material map.
Passing the gate resets checkpoint progress and invokes the existing lap,
sound and results bookkeeping. Exact DOS timer conversion remains open.

Verification: 256 original-instruction finish-gate cases match native
lap eligibility and checkpoint reset across all four driver slots, four
checkpoint progress values and 16 selected/raw surface pairs. Native
sequence tests prove route wraps do not award laps, premature line crossings
do not award laps, checkpoint edges are inclusive, completion alone is
insufficient, and staying on the line cannot repeat a lap with checkpoints
still outstanding. Existing 327,680 route, 23,520 recovery, 2,352 route-bound
cases and physics/surface/collision/dirty-tracking checks pass.

Silent A1200 `.run/results-checkpoints` reached results at frame 451:
all cars finished, laps 2/2/2/2, positions 3/2/4/1, AI waypoints 0/0/0/1,
checksum `e4bf7c8f`, display `a741fe3d`. The old strict results golden
**fails**, as expected from changed flow; it has not been replaced. This
run demonstrates completion through the new checkpoint path, not exact
DOS finish order, timing or complete AI state-machine fidelity. Normal
launch files were not overwritten.

### Checkpoint oracle and race-clock recovery

`verify-dos-ai` additionally executes original `2000:27b6..2840`, stopping
at the real advance/no-advance branch destinations. All 2,352 cases match
the production `advance_checkpoint` helper: four driver slots, three
checkpoint indices, inclusive and inverted bounds, negative/large input
coordinates and subpixel boundaries. The complete route/recovery/lap-gate
suite and host physics/surface/collision/dirty tests pass; target rebuild
succeeds. This helper extraction does not change the preceding A1200
race-flow behavior, but this turn did not repeat that emulator run.

Clock evidence for the next implementation: `1000:fe6d..fea3` accumulates
elapsed timer ticks in the 32-bit DS:685e counter (elapsed batches capped
at 45). `2000:2a94..2acc` records lap duration as twice the difference
between that counter and the previous lap timestamp. `2000:aceb..ad5c`
formats the unsigned LOW WORD after clamping it to 0x464f (17999), then
computes `floor(raw*5/9)` and formats two seconds digits, a period and two
fraction digits. Thus stored duration units are not centiseconds, and
the current native +2-per-update counters are not the original clock.
`3000:7a8b` reads the IRQ counter at DS:74bc; `7b41` increments it.
Preserve the distinction between raw 32-bit elapsed/lap state and display
conversion when replacing the native counters; do not simply scale each
frame independently, which would lose fractional timing.

### Raw-duration clock and exhaustive display conversion

Native cars now retain 32-bit elapsed/current/last/best raw duration units.
Each racing update adds twice its physics tick count, not a fixed two
centiseconds. Display values are derived from the accumulated raw value:
take the low 16 bits, clamp to 17999, multiply by five and divide by nine.
Lap completion preserves raw duration, resets the current raw accumulator
and compares signed raw best times; startup uses DOS's 30000 sentinel.
Existing centisecond fields are display/diagnostic views, not accumulators.

All 65,536 possible low-word inputs match execution of original
`2000:aceb..ad29` with its real arithmetic helpers, including clamp and
low-word wrap. A 100,000-update variable-tick test verifies raw accumulation
without per-update rounding. Route/checkpoint/lap/recovery oracles pass.
The old physics assertion expecting two centiseconds from one tick was
replaced with the verified result: two raw units display as one centisecond.
Physics, surface emission, collision and dirty tests pass; target builds.

Remaining clock scope: the native accumulator currently starts with the
native racing phase. Exact DOS race/countdown epoch, HUD mode selection,
full 32-bit timestamp storage in results and formatter punctuation/layout
still need integration verification. This is not a claim of complete HUD
or race-time fidelity. The old strict emulator goldens remain unchanged.

Silent A1200 `.run/results-dos-clock` reaches all four finished cars and
drawn results at frame 451, positions 3/2/4/1, as before the clock change.
Updated timer pixels produce checksum `726d0125`, display `be9dce13`.
The retained old strict results golden still fails; no assertion was
relaxed. The isolated session was stopped and normal launch files left alone.

### Countdown epoch and lap timestamp arithmetic

The clock now accumulates during the stationary start-light sequence too.
Evidence: countdown state is initialized at `fc41`, per-car lap timestamp
DS:303b is zeroed at `fdd1..fdd7`, global clock DS:685e is zeroed at
`fe3c..fe42`, and elapsed ticks are added at `fe9f` before countdown handling.
Thus the first lap includes grid time. The native `slicks_race_step` test
executes 20 real countdown updates and verifies all four raw elapsed/lap
counters against the PIT schedule while positions and countdown stage
remain unchanged. Its host-only particle symbol is an aborting guard:
entering gameplay would fail rather than silently substitute a particle loop.

Production `record_lap_clock` now has 512 original-instruction comparisons
against `2000:2a94..2b17`, including timestamp subtraction/doubling wrap,
zero and equal durations, signed high-word best-time comparisons and lap
increment. Display fields are regenerated from the saved raw duration,
not trusted as input to lap recording. All 65,536 display conversions,
100,000 accumulation updates, route/recovery/checkpoint tests and host
physics/surface/collision/dirty regressions pass. The target rebuild passes;
this turn did not repeat FS-UAE. Countdown release cadence, full HUD modes,
results timestamps and remaining AI transitions still require full-run
DOS/native comparison; the clock changes do not prove those complete.

### Route-loader expansion

`1000:bb56..bb96` expands loaded AI rectangles: subtract two from each
lower bound only if its signed word value is >1; add two to both upper
bounds with word wrap. Native loading now performs these operations once,
leaving target coordinates, the trailing byte and lap checkpoints alone.
Route Y coordinates are now words, matching DOS's widened byte reads;
file value 255 becomes upper bound 257 rather than wrapping to one.

The helper is compared with the original loader's arithmetic slice in
2,916 cases, covering lower bounds 0/1/2, signed-word extremes, upper-word
overflow, Y values 254/255 and unchanged driving targets. All cases pass,
all supplied tracks decode, host regressions and the A1200 rebuild pass.
The prior route predicate oracle supplied already-loaded bounds, so did
not detect this missing loader transformation; loader and consumer now
have separate original-code checks.

The skipped four-byte alternate-target records are also located precisely:
`bba5..bc04` reads count+1 into DS:6686, stores records at indices 1..count
of DS:6688/6750 (BE16 x, byte y, one discarded byte). `f3e1..f4be` chooses
the nearest candidate using the original distance helper, with slot zero
as fallback. Decoding/selection and the surrounding state-one behavior
remain open; do not silently treat these as extra lap checkpoints.

The pre-expansion clock-epoch build was verified in silent A1200 run
`.run/results-clock-epoch`: all four finish/results at frame 451, positions
3/2/4/1, checksum `59b94694`, display `2232da6d`. The retained historical
strict results golden fails; no expectations were weakened.

With route expansion, silent `.run/results-route-expansion` reaches all
four finishers and draws results at frame 450, positions 4/2/3/1,
waypoints 0/1/1/1, checksum `b24d0b09`, display `f97c7f1a`. This also fails
the unchanged old strict golden; reaching results is not proof of exact
DOS trajectories. Both isolated emulator sessions were stopped.

### Alternate-target decoding and selection

Track loading now retains the four-byte alternate records, with the original
count+1 and reserved slot-zero convention. `select_ai_alternate` implements
`f3bc..f4d3`: initial best distance 30000, candidates starting at index one,
absolute component distances from `(car/100 +4)`, signed low-word sum,
strict improvement (first candidate wins ties), and fallback index zero.
Zero runtime count switches to escape state 2 with timer 400. Loaded empty
lists have count one and therefore select slot zero instead. The native
state-one path selects an unset target and steers toward DS:690a/6912
equivalents, rather than following the ordinary route target.

2,940 original-instruction selector cases pass, including all four drivers,
empty/reserved-only lists, ties, signed coordinates and word-sum wrap.
The complete e204 oracle now covers both ordinary route and alternate
target input paths: all 655,360 control cases pass using the real ES-based
alternate coordinate reads. Existing lap/clock/checkpoint/recovery and
host regressions pass; the A1200 build succeeds. No emulator run this turn.

The first bounded decoder rejected CURVE6: it contains exactly 100 file
records, loaded by DOS at indices 1..100. Native storage now allows all
101 slots. DOS x[100] aliases y[0] because 6688+200=6750; this fallback Y
effect is reproduced explicitly, not through an out-of-bounds write.
A real-asset regression verifies 468 objects, count 101, last point (60,80)
and fallback (0,60). All supplied tracks decode again. DOS's last Y write
also reaches address 6818; any effect outside target selection remains
to be audited rather than replicated through unsafe memory corruption.

Scope: this does not yet make state-one recovery naturally reachable from
every DOS transition. Next recover `f162..f1de` state entry and route/service
timers together with the recent-contact counter. `2000:3d3e..3d7b` updates
DS:53c6 for AI drivers: increment by elapsed ticks while below 60000 and
not touching a boundary, or clear after contact unless DS:4dae suppresses
that reset. It is not a distance/probe metric. That counter drives the
unsigned threshold comparisons used to enter alternate/escape states.

### Contact-driven AI recovery and alternate arrival

Recovered `f162..f1de` state entry now consumes the signed recovery timer,
unsigned recent-contact age/threshold and route-seen flag. Ordinary state
enters alternate recovery (250 ticks) or escape (350 ticks), and an expired
alternate state enters escape (400 ticks), with the original 32000 age reset.
Startup uses age 60000, threshold 350 and the original route/timer defaults.
Route hits set the route-seen flag and apply the recovered speed-dependent
timer update. Non-service alternate arrival (`f679..f706`) uses inclusive,
unsigned word bounds around the sprite origin, clears the target/state/seen
flag on arrival, and applies signed-byte elapsed ticks below speed 7000.

`3d3e..3d7b` now updates recent-contact age after the current car's physics:
test age below 60000 before adding full elapsed ticks, without saturation;
clear on boundary contact unless the car-contact latch is set. The suppression
source is the existing `touching_car` (DS:4dae), not a separate flag:
`3183..318e` sets both participants and `31af..31b8` clears the current car
when no overlap remains. Existing collision tests cover that latch lifetime.

Original-instruction differential verification passes 6,144 transition cases,
512 contact-age cases and 84,672 alternate-arrival cases. The latter includes
all four car slots, unsigned edge bounds, signed speed comparisons, signed-byte
elapsed time and timer wrap. Unicorn's cached translated blocks initially
ran past the alternate slice boundary; explicitly clearing that cache before
the test resolves it. The original instruction bytes are unchanged. All
655,360 route-control cases and driving/collision/surface/dirty regressions
still pass, and the final A1200 executable builds successfully.

The first silent A1200 run `.run/results-contact-recovery` (before connecting
the car-contact latch) reaches all four finishers/results at frame 450,
positions 4/2/3/1, checksum `b24d0b09`, display `f97c7f1a`, unchanged from
the previous route-expansion run. The retained strict historical results
golden still fails; it was not weakened. That emulator session was stopped.
The final build with the recovered latch connection was separately run in
silent `.run/results-contact-latch`; it produces the same frame, positions
and checksums and the same historical-golden failure. That session was also
stopped. Neither isolated run replaces the user's normal runnable binary.

Remaining AI fidelity work includes service-entry/arrival paths
`f290..f36b` and `f504..f676`, the predictive-contact/weapon gate, and complete
caller ordering verification. Native track-boundary response is still an
approximation, so exact local state tests do not establish exact trajectories
or complete AI recovery in every race situation.

### Service approach controls

`ai_service_approach` now implements `f504..f5e5` and is called after normal
steering when alternate state 1 has a positive service state. It sets service
state 2 below distance 10. Below distance 80 it clears brake and chooses
throttle from the original nested thresholds: speed >700, distance <20 and
speed >200, or distance <9 and speed >80 suppress throttle. Steering remains
unchanged; at distance >=80 all ordinary controls remain unchanged.

The DOS distance is not signed Manhattan distance. Each word subtraction is
passed to the real long absolute-value helper with a zero high word, making
the subtraction's low word nonnegative at the helper boundary. The low-word
sum is then compared as signed. Native code retains that wrap behavior.
100,352 original-instruction cases pass, including all four drivers, all
16 initial control combinations, negative coordinate differences, word wrap,
and all three speed/distance boundary pairs. Tests execute the actual DOS
helper, not a stubbed absolute-value approximation.

Full service flow remains open. `b103..b1a4` generates up to 30 route/pit
pairs, iterating route regions then pit destinations and calling visibility
routine `cb02`; it records pairs only when that routine returns zero. The
inputs are DS:3641 pit count and DS:367e/3692 pit coordinates. These are not
the alternate-target records already decoded from the track. Pit-coordinate
production and this visibility dependency must be recovered before enabling
service entry from route hits (`f290..f36b`). Completion `f5e5..f676` tests
damage and optionally the original fuel quantities; service requests at
`f7dd..f849` also depend on fuel/lap state. None of these remaining branches
is replaced with an invented shortcut, and a normal BASIC run does not
exercise this new positive-service-state path yet.

### Pit destinations from original track objects

The live loader listing fills the previously missing producer: `b8cb..b908`
recognizes object types 68/69, sets DS:36a6, and appends up to ten destinations
to DS:367e/3692 in object order. X is the object's unadjusted word; Y is the
object's byte plus five. Rotation is not consulted. Native track navigation
now retains these destinations and availability, resets their count on each
load, and connects availability to the race's existing damage master gate.
The independent damage-scale option remains unchanged (zero in normal setup).

12,100 comparisons with the original loader slice pass: every supported
object type, counts zero through ten, both existing availability states, and
coordinate word-wrap boundaries. Actual asset regressions verify CURVE6's
three destinations (115,33), (85,33), (103,33), then reload BASIC and verify
exactly two: (114,121), (214,64). This also checks that prior pit counts do not
leak into a subsequent track. Full AI, driving, car-collision, surface-effect
and dirty-region host suites pass; the native A1200 build succeeds.

Next dependency: port the pit-routing use of `cb02`. It samples a line from
each route region's first corner to each pit, using the original `c5a0`
collision predicate, and records up to 30 unobstructed region/pit pairs in
`b103..b1a4`. Preserve the word-truncated multiply followed by signed division
in the line sampler; replacing it with a generic Bresenham test is not an
instruction-equivalent implementation. Pit destinations alone do not enable
automatic pit entry, repairs or refuelling.

Silent A1200 `.run/results-pit-loader` reaches all four finishers and results
at frame 450, positions 4/2/3/1, checksum `b24d0b09`, display `f97c7f1a`.
The unchanged historical strict golden still fails. The isolated session was
stopped and the normal runnable binary was not replaced.

### Native pit visibility and route-table generation

Track loading now generates the DS:3640/3642/3660 equivalents: at most 30
unobstructed route-region/pit pairs, in region-first then pit order. The
`cb02` pit-query specialization uses car=-1, layer zero and no actor test.
It retains inclusive endpoint sampling, ignores a blocked starting sample,
returns clear for a zero-length segment, and explicitly truncates each
interpolation product to a signed word before division. Native reads wrap
their linear offset to 16 bits as `b089` does. Unsupported offsets outside
the retained 60800-byte material map fail loading rather than inventing RAM
contents. All supplied tracks load successfully with this check.

The oracle executes the entire original `b103` routine and its real `cb02`,
`c5a0`, `b089` and negative-car `c63e` helpers. 768 cases match, covering all
32 materials, boundary levels 0..5, uniform/wall/start-point/sparse patterns,
both major line axes, word-product overflow, zero-length paths and the
30-entry limit and ordering. Existing full AI and host regressions pass;
the A1200 build succeeds. Loader generation currently uses boundary level
5 (new-race level); timing of reloading tracks after animated boundary
changes remains to be checked against the complete DOS setup chain.

Important correction from the full oracle: material 2 is BLOCKING in this
query, via the direct `c61c` return. Materials 22..26 compare material-22
against signed DS:4c6c-1. The previous moving-car helper's comment incorrectly
claimed material 2 was nonblocking. That comment is corrected, but changing
moving-car behavior requires recovering its actor/layer/sample gates, not
just inserting class 2 into the approximate raw-map predicate. This remains
an explicit track-collision fidelity gap.

Next: consume the generated route table in `f290..f36b` and preserve
same-update control ordering when an ordinary route hit enters service.
Repair/refuelling, service requests/completion and the full AI caller
sequence remain open; generating pit routes alone does not complete them.

Silent A1200 `.run/results-pit-visibility` reaches all four finishers/results
at frame 450, positions 4/2/3/1, checksum `b24d0b09`, display `f97c7f1a`.
The retained historical golden still fails unchanged. The isolated emulator
was stopped; the user's normal runnable binary was not replaced.

### Service entry and same-update steering

Route hits now consume the generated pit table through `f290..f36b`.
Positive service state is cleared and the contact threshold reset to 350;
a pending negative request changes that threshold to 50. The first matching
route/pit pair supplies the target and enters service/alternate state one.
An empty route table uses pit zero only when the master availability flag
allows it, matching the original fallback. 960 original-instruction cases
cover all four drivers, signed service-state extremes, absent/unmatched and
duplicate route entries, fallback enable/disable and target selection.

AI control flow now follows sequential original state checks. Ordinary
steering runs before the route hit; if that hit enters alternate/service
state, target selection and a second steering call happen in the SAME update.
Returning from an alternate destination does not re-run the earlier route
test. The reusable steering helper retains its prior 655,360-case oracle.
65,536 additional comparisons execute original `f1de..f5e5`, including both
real `e204` calls and service approach, with DS equal to the original ES-based
alternate target segment. They cover four cars, all headings and initial
control masks, eight target directions and eight speed thresholds, comparing
controls, targets, state/seen flags, contact threshold, route index and timer.

The damage-only request at `f7dd..f849` is also active: with no existing
service state, signed damage[0] >500 requests service (-1). It runs after
ordinary/alternate/escape controls, as in DOS. 168 original cases cover the
fuel-disabled branch, all cars, signed state extremes and the damage boundary.
Fuel-dependent requests remain unimplemented; so do repair/refuelling and
service completion. Current normal setup still has damage_scale=0, so an
unmodified BASIC race is not a natural end-to-end pit-stop test.

All new comparisons, the previous AI suite and driving/collision/surface/
dirty-region regressions pass, as does the A1200 build. Silent isolated run
`.run/results-service-entry` reaches all four finishers/results at frame 450,
positions 4/2/3/1, checksum `b24d0b09`, display `f97c7f1a`. The retained
historical strict golden fails unchanged; the emulator was stopped, and the
normal runnable binary was not replaced.

### Pit repair and fuel-disabled service completion

Direct decoding of the missing `231fb..232f3` branch identifies material 30
as the pit service surface. `repair_car_at_pit` now runs after car collisions,
using the saved measured speed and effective (special-state-suppressed)
surface. Speed must be signed <300. It adds full elapsed ticks to one shared
race-local signed-word accumulator (original BP-64, initialized at `1fc4b`),
not a separate per-car timer. At >=5 it subtracts five ONCE and subtracts
eight from all four damage channels, with word wrap and negative clamping;
damage-scale zero clears each channel. The shared accumulator persists
between car updates and frames. The original HUD refresh is not yet ported
as a complete damage/status HUD.

17,280 original-slice cases cover all cars, speed boundaries, signed timer
wrap, elapsed-tick extremes, damage subtraction wrap, and negative/zero/
positive damage scale. The oracle stops before the real HUD/refuel branches,
without replacing either with a fake successful implementation. The native
fuel-disabled completion branch `f5e5..f676` now clears service/alternate state,
route-seen and target X and sets timer 400 when signed damage[0] <10.
128 original cases verify that branch and preservation of target Y.
Same-frame service-order tests set damage to 100 to keep completion false
past their earlier stop boundary; they continue to verify both steering calls.

Refuelling remains explicit open work. `2327e..232f3` clears bit zero of
DS:305e, adds sign-extended low-word(ticks*60) to signed 32-bit DS:305f,
and clamps values >=DS:3063 to capacity-1. `20306` consumes ticks*2 before
controls and `20427` consumes another ticks*2 when accelerating. Capacity
and initial quantity are derived at `24c62..24cca` from setup/upgrade inputs,
not a guessed constant. Fuel-enabled service completion compares wrapped
32-bit fuel*10 against capacity*9. Recover these producers/consumers together
before enabling the fuel option; the current native setup remains fuel-off.

The full AI oracle, host gameplay regressions and A1200 build pass. Silent
`.run/results-pit-repair` again reaches all four finishers/results at frame
450, positions 4/2/3/1, checksum `b24d0b09`, display `f97c7f1a`; the retained
historical strict golden fails unchanged. This normal damage-disabled race
is not evidence of a natural complete repair stop. The isolated emulator
was stopped and the normal runnable binary was not replaced.

### Fuel quantities, consumption, refuelling and AI requests

Native cars now retain DOS's 32-bit fuel/capacity bits, the signed stock
fuel-upgrade word and DS:305e service flags; the race retains DS:3024's fuel
option. Setup `24c49..24cca` uses .omi byte 33 (DS:4efc), divides its product
with the fuel option by ten, multiplies by upgrade+46 and 72, then divides
by 100. Intervening products wrap to 32 bits; divisions are signed and
truncate separately. Initial quantity is capacity-1. The current stock
setup has upgrade zero; menu selection/upgrades are not yet exposed.

Idle consumption precedes AI/input: subtract sign-extended low-word(ticks*2),
clamp negative quantity to zero, and set flag bit zero only with fuel and
service master enabled. Throttle consumes another ticks*2 without a second
clamp; a latched empty-fuel flag caps the drive scalar at 3000 after its
normal cap. Accelerator road-cloud emission respects that flag. At effective
pit material 30 and saved speed <300, refuelling clears just that bit, adds
sign-extended low-word(ticks*60), and signed-compares/clamps at capacity-1.
These quantities continue updating with the fuel option disabled, as in DOS.

Service completion now includes the original signed comparison of wrapped
fuel*10 and capacity*9, rather than a rounded percentage. A fixed-lap AI
requests service below signed capacity/4 only before the final lap; damage
>500 still requests it independently. The oracle executes the actual `991f`
lap-limit helper in fixed-lap mode. Its timed-race dynamic-limit behavior
is still outside the native setup/finish flow and is not claimed complete.

Original-code comparisons pass: capacity 1,176; idle fuel/flags 2,688;
refuel/clamp/flags 4,704; service completion 16,384; fixed-lap service request
6,912. Cases include zero/negative inputs, word/long overflow, signed boundary
values and preservation of unrelated flag bits. All prior AI and host
driving/collision/surface/dirty regressions pass, and the A1200 build succeeds.
The normal runner remains fuel-off. A fuel-enabled natural A1200 pit-stop
regression and full HUD/menu integration are still required.

Silent A1200 `.run/results-fuel-runtime` preserves the normal fuel-off race:
all four finishers/results at frame 450, positions 4/2/3/1, checksum
`b24d0b09`, display `f97c7f1a`. The historical strict golden fails unchanged.
The isolated emulator was stopped and the normal runnable binary was not
replaced. This run does not verify a naturally occurring refuelling stop.

### Natural fuel-enabled A1200 gate: missing pit material

`SLICKS_FUEL_RACE=1 amiga/debug.sh <ROM> diag_fuel.gdb` selects fuel option
10 before stock fuel initialization, otherwise retaining the real BASIC
assets, four-lap grid and AI. It injects no car positions, quantities or AI
states. Per-car observations record requests, actual fuel increases on
effective material 30, and service-completion departures. The gate fails
unless at least one car performs all three; it is not a full race-fidelity
or performance assertion. Normal fuel-off setup and audible `run.sh` are
unchanged.

Two silent A1200 runs (`.run/fuel-natural`, `.run/fuel-pit-surface`, ports
24911/24912) fail at frame 2400: all four cars requested service once and
remain in state 2 on lap 2, with zero refuel/departure/pit-surface frames.
Capacities are 3345/4305/3312/3312, from actual .omi bytes 101/130/100/100.
Final positions are (12073,12528), (12079,12367), (11939,12383),
(11835,12538), in hundredths of a pixel. All target (114,121), detect layer
1 material **3**, and have measured speeds 118/60/140/88, below the refuel
gate's 300. Thus the immediate failure is missing material 30, not failure
to request or reach the pit, nor excessive speed at the final observation.
Both isolated emulators were stopped; the normal runnable binary was not
replaced.

`SLICKS_INSPECT_PITS=1 build/scan_track_materials ref/SLICKS.DAT
ref/TRACKS/BASIC.SS` now reports both maps around recorded pit destinations.
BASIC contains **zero material-30 pixels in either map**. `draw_sprite`
explicitly preserves underlying classes for type 68, an assumption from
the fuel/damage-off reference. Original `b9b0..b9f9` omits visual objects
68/69 only when both DS:3026 and DS:3024 are zero, clearing DS:36a6;
`bdbd..bde6` then omits their material pass only when that flag is clear.
Enabled service therefore needs the real mask pass, not a painted rectangle
or a refuel rule keyed to proximity.

Next: recover the original `/masks` image section loaded at `c080..c105`
and the category/class tables at DS:070b/0731 consumed by `b225`. The native
DAT decoder currently stops after 110 visible sprites at byte 34552;
remaining DAT content includes additional sections (the original loader
also scans for 12 34 00, present at byte 41022). These facts identify the
asset-decoding gap, not yet the mask format or an implemented fix. Keep the
natural fuel gate failing until actual service surfaces and full stop
behavior are verified against original instructions/assets.

All existing `verify-dos-ai`, driving, car-collision, surface-effects and
dirty-tracking host gates pass; the diagnostic A1200 build succeeds. Their
passing isolated cases do not establish the failing natural service flow.

### Real archive masks restore natural refuelling

The previous section's DAT-tail lead was not the mask source. Tracing
`7079` through the resource-opening routine establishes that `/masks`
selects the `masks` entry in **SLICKS.000**: 19,966 bytes, 110 ordinary
compressed images followed by a terminator. The new native material pass
decodes that resource, rotates each placed mask, and applies original
`b2c5..b426` category/compositing semantics in object order. It handles
both independent five-bit layers, protected surfaces, bridge-flag behavior,
the y=185 crop, and option-dependent omission of objects 68/69. It does
not synthesize pit rectangles from their target coordinates.

The Amiga loader now always replaces the legacy DAT-derived material maps
with this real mask pass, reusing the temporary DAT allocation and sprite
arena. Pit visibility routes are rebuilt from the resulting lower map.
Service availability now requires a pit plus fuel or damage enabled;
normal fuel/damage-off loading no longer enables it merely because the
track contains a pit. The old DAT-only builder still supplies the visible
background and legacy host-test compatibility; its inferred material
rules should be removed as callers migrate. Visible pit graphics still
need the original option gate (the old visual builder always draws them).

`verify-dos-ai` executes the real compositor and packed-map read/write
helpers for **77,824** combinations: mask values 0..37, all 32 lower and
upper values, and both bridge modes. It checks outputs and preservation
of neighboring packed pixels. All cases and prior AI tests pass. The
expanded actor-mask verifier reads the original archive directly when a
sixth argument supplies SLICKS.000. BASIC and BUMPS each match every one
of 60,800 raw-mask bytes and 60,800 upper-surface values in existing DOS
captures; BASICTRK matches all 60,800 available upper values. The first
trial mistakenly supplied decoded `slicks-track-material.bin` as a raw
fixture; using the documented `slicks-track-chunky.bin` resolves that
fixture-type error without changing code or expected data.

`verify-native-tracks` now exercises all 195 supplied tracks with archive
masks, with service both disabled and enabled. Both passes and driving,
car-collision, surface-effect and dirty-tracking regressions pass. The
optional `SLICKS_MASK_ARCHIVE` / `SLICKS_SERVICE` inspection reports BASIC
has 358 upper and two lower material-30 pixels when service is enabled.

Silent A1200 `.run/fuel-real-masks` (port 24913) passes the natural stop
gate at frame 2400: all four cars request, refuel and depart; counts are
2/1/2/2 completed departures and 64/43/62/61 fuel-increase frames. Laps
are 4/5/5/5 (three finishers, first car still racing), so this proves the
natural stop flow, not a completed four-car fuel-enabled race. No car,
fuel or AI state was injected. The emulator was stopped and the normal
runnable binary was not replaced.

Silent `.run/results-real-masks` (port 24914) retains the normal fuel-off
results at frame 450, all four finishers, positions 4/2/3/1, checksum
`b24d0b09`, display `f97c7f1a`. The unchanged historical strict golden
still fails. The isolated emulator was stopped. The final rebuild also
makes service availability recompute from option and recorded pit count
on each mask load, including an off-to-on reload.

### Complete natural fuel race and setup-aware pit graphics

The fuel gate now requires all four cars to have requested service, gained
fuel on the pit surface and departed, followed by four completed four-lap
races, unique positions 1..4, race completion and drawn results. It allows
up to frame 3600 rather than declaring success at the earlier stop-only
checkpoint. Silent A1200 `.run/fuel-complete-race` (port 24915) passes at
**frame 2632**. Departures remain 2/1/2/2; all cars are on lap 5, first
car finishes with fuel 446. This is an end-to-end native invariant gate,
not evidence of identical full-race DOS trajectories or finishing times.
The isolated emulator was stopped; the normal runnable binary was not
replaced.

`slicks_build_track_scene_options` now uses the real fuel/damage option
words before drawing. Objects 68/69 are omitted only when both are zero,
as at original `b9b0..b9f9`; their target records remain loaded. The Amiga
loader uses this setup-aware entry point and then applies the archive
masks with the same service option. Fuel selection is made before both
passes. Existing DAT-only host callers keep their old explicit legacy
wrapper until migrated; the all-track scanner now exercises the new
visual entry point in both option states.

The original-code oracle passes **18,432** visual-gate cases: all byte
object types, six signed values of each option, and both previous service
flags. The real BASIC asset test additionally verifies that disabling
service produces exactly the same VGA bytes as omitting the two pit
objects, retains the pit targets, and that either fuel or damage enables
identical pit graphics. All existing AI and mask-compositor comparisons
pass. Fuel/damage HUD bars and menu selection remain open; runtime options
are not yet a complete user-visible setup implementation.

All 195 tracks load with setup-aware visuals and archive masks with service
off and on; driving, car-collision, surface-effect and dirty-tracking gates
pass. Silent `.run/results-pit-visual-gate` (port 24916) still reaches all
four fuel-off finishers/results at frame 450, positions 4/2/3/1. Correctly
omitting pit graphics changes the checksums to `ba6de86a` / `898da844`.
The historical strict golden remains untouched and fails, as expected
from both the previously documented race changes and this visual change.
The isolated emulator was stopped. Build and whitespace checks pass.

### Recovered fuel/damage HUD command generator

`slicks_race_status_rects` implements the fuel/damage subset of original
`d9b6..ddbf` for the four active entrants with the weapon option disabled.
It emits half-open rectangles with semantic colour slots, not guessed
palette indices. Per car the left edge is `106 + 60*car`: background
20 pixels wide over y=187..189, fuel row y=188, damage row y=189.
Fuel width is signed `(wrapped32(fuel*40)/signed(capacity) + 1)/2`, with
the increment also wrapped, followed by a word-width endpoint addition.
It is deliberately not `fuel*20/capacity`. Exhausted flag bit zero selects
width 0 or 20 from timer bit zero. Damage draws only for positive channel
zero and uses `min(damage/40,20)`. A native invalid signed fuel division
returns -1 instead of inventing a display value.

The DOS oracle executes the complete `d9b6` routine, including real long
arithmetic helpers, and intercepts only the final rectangle drawing boundary.
It checks both original display-page calls, their exact coordinates, semantic
colour correspondence, page arguments, call count and return stack against
the native descriptors. **24,576 cases pass**, covering all four cars,
fuel/damage option combinations, signed/overflow fuel inputs, six capacities,
eight damage values, exhausted flag and both timer phases. Palette-cache
values are seeded in this test: it verifies drawing commands, not palette
resolution or rendered pixels. All preceding DOS AI/mask/service tests
still pass, and the A1200 build succeeds.

This API is not yet called by the production HUD renderer. The existing
timer/lap HUD is unchanged, so no new visible bars or FS-UAE HUD verification
are claimed. Next integrate palette selection (`d9d3..da34`, nearest colours
15/15/25 background, 50/50/15 fuel, 60/20/5 damage), authoritative chunky
rectangle writes plus dirty tracking, and the real blink cadence. The
runtime's DS:1716 pointer contains 0040:006c (BIOS tick count), not the
physics-tick counter or frame parity. Weapon/status icons, inactive-player
handling and full original HUD arrangement remain outside this subset.

### Live chunky status bars, palette and BIOS-rate clock

The Amiga loader now resolves background/fuel/damage colours from the
prepared race palette using the original nearest-colour rule (entries
1..255, strict improvement, initial distance 300). The oracle executes
the original `36fae` matcher for **960** comparisons, including uniform
palettes/ties and randomized valid six-bit DAC palettes; all match.

`slicks_race_draw_status` now consumes the verified rectangles during
initial setup and after each live race update. It writes the authoritative
chunky buffer, mirrors the VGA store only when required, and marks changed
pixels through the existing dirty tracker. It never writes bitplanes
directly. All four cars' calculations are validated before any drawing;
zero-capacity and signed-division-overflow faults cause diagnostic error
9 rather than partially painting or inventing values. The fuel/damage-off
path makes no display changes.

`SlicksStatusClock` accumulates PIT input /65536 from elapsed PAL vblanks,
independently of physics updates. The platform retains the clock across
race setup, with an application-local epoch rather than claiming the same
absolute phase as a captured PC BIOS clock. Vblanks while native menus
are displayed count; time with hardware returned to AmigaOS during disk
loading is not represented in this counter. A 100,000-step host test with
variable frame gaps checks tick/remainder arithmetic and low-word wrap
against a 64-bit reference.

Host raster tests cover 16 fuel/damage/exhaustion/phase combinations,
independent expected pixels across the full screen, agreement of VGA and
chunky representations, preservation outside the bars, sparse dirty-list
saturation, and no partial writes on division faults. All pass together
with the prior command oracle, AI, physics, collisions and surface tests.
The A1200 build succeeds. The fuel diagnostic now additionally compares
all 240 status pixels in chunky memory and actual interleaved bitplanes
against the verified descriptors at both clock phases and at results.
Menu exposure of fuel/damage and the remaining full HUD layout are still
open; the normal setup keeps both options disabled.

Silent A1200 `.run/fuel-status-hud` (port 24917) passes at frame 2632:
all four natural service/finish sequences remain intact, and all three
240-pixel chunky/bitplane audits report **zero failures**. Departure counts
remain 2/1/2/2 and final fuel values 446/0/0/0. This run enables fuel only;
damage-bar raster semantics are covered by the host/original-code tests,
not a damage-enabled natural target race yet. The isolated emulator was
stopped and the normal runnable binary was not replaced.

### Pre-start vehicle and service setup

The native menu previously selected the player's vehicle after race start,
which left fuel capacity initialized from the previous/default vehicle.
Vehicle defaults now belong to initialization, and the selected vehicle is
applied before track loading and race start. Start validates vehicle indices
and retains the selection. Vehicle and service setters reject changes once
the race has started, preventing a partially reconfigured live car.

The loader accepts fuel/damage settings before deciding whether to draw pits
and build their material maps. The custom-race fuel conversion matches the
original physical `2be0b..2be47`: signed values up to 5 disable fuel, larger
values are retained; damage is copied unchanged. All 65,536 signed inputs
match execution of that original instruction slice. Synthetic startup tests
exercise all ten vehicle selections and their resulting fuel capacities;
they test setup order, not asset rendering. The existing physics, collision,
surface-effect and dirty/HUD regressions also pass, as does the A1200 build.

This is setup plumbing, not completion of the original configuration menus:
interactive fuel/damage controls and non-custom game modes remain open.

Silent A1200 `.run/fuel-setup-order` (port 24918) passes at frame 2632.
All four cars finish four laps after natural pit service, with departure
counts 2/1/2/2 and fuel capacities 3345/4305/3312/3312. All three status
audits pass with zero chunky/bitplane pixel failures. This target run uses
the default vehicle selection; the other nine selections are startup-tested
on the host, not claimed as target race regressions. The isolated emulator
was stopped; the normal runnable binary was not replaced.

### Native fuel/damage options and interactive startup

The Options title entry now opens a native fuel/damage submenu. Up/down
select a setting or Back; left/right adjust it; Return toggles a setting
between its extrema, or activates Back; Escape returns without discarding
the selected values. Mouse activation follows the same selected-item path.
The existing left/right lap selection on the title's Options entry remains.
This is a native menu layout using the existing title text renderer, not a
claim that the original full options screen has been reproduced.

The original eight-byte records at DS:00da and 00e2 have minimum zero,
maximum 300, and increments 5 and 20 respectively. Executing the original
`294d1..29659` input path matches the native helper for **917,504** cases:
every signed-word starting value, both settings, left/right, Home/Page Up,
End/Page Down and Return. The helper includes original word wrapping before
signed clamping. Native navigation tests separately cover selection wrapping,
adjustments, Back/Escape and ignored keys. Extrema scan codes are tested at
the helper boundary; the Amiga UI currently maps arrows and Return only.

The first target menu test exposed a pre-existing entry-ABI bug: the shared
support `_start` invokes `main()` with no arguments, while Slicks declared
`main(int, char **)`. With an empty CLI string, the stack's saved register
was interpreted as argc=4096, causing an automatic race before the menu.
The program now declares `main(void)` and obtains/whitespace-trims the CLI
string through AmigaDOS `GetArgStr()`. Diagnostic mode selection no longer
depends on incidental caller stack contents.

The first debugger-driven input attempt also proved that this FS-UAE stub
silently ignores target-memory writes: the keyboard queue read back as zero
immediately after writes. The apparent vblank stall was therefore an idle
menu with no input; the VBI was active. `CONFIG` now queues only the 13 raw
key events in the application, and `diag_service_menu.gdb` checks their
effects read-only. It does not write selected settings or race state.

With working input, the test exposed three further existing GCC bridge
offset errors in `native_bridge.s`: narrow integer arguments occupy
four-byte stack slots, with their low word two bytes into the slot. Key
dispatch read offset 4 instead of 6, so all real keys became zero; menu
selection read 56 instead of 58; text x/y read 56/60 instead of 58/62.
These offsets are corrected. `make verify-title-bridge` executes the actual
production wrappers on a 68020 emulator with GCC-sized stack arguments:
65,536 dispatch cases and 512 text/selection boundary-register cases pass.
Drawing is intercepted at the register-ABI boundary in this test; this is
not a pixel oracle. The gate is also included in `verify-native-graphics`.

Silent A1200 `.run/service-menu` (port 24919) now passes. Its five submenu
draw calls report fuel/damage 0/0, 5/0, 10/0, 10/0, 10/20 in sequence.
The loader receives fuel 10 and damage 20, builds an enabled service map,
and starts the race with car-zero capacity 3345. The final assertion reads
the actual race object (the fuel-only diagnostic counters are intentionally
inactive in this test). The isolated emulator was stopped. This verifies
keyboard-to-setup integration, not complete original menu visuals, mouse
input, or a full damage-enabled race; those remain separate gates.

The separate silent `EXIT` command regression in
`.run/service-menu-restore` (port 24920) also passes, reporting restoration
status `1f`. It confirms the corrected CLI path still selects the restore
diagnostic and returns the tracked hardware/system state. Both runs use
strict PAL A1200, 2 MiB chip RAM and no fast RAM. No normal runnable binary
was promoted or existing gameplay checksum baseline rewritten.

### Damage-enabled menu race gate

`diag_damage_race.gdb` extends the CONFIG input-only scenario to the real
four-lap BASIC race (fuel 10, damage 20). Diagnostic-only observations record
each car's peak channel-zero damage and frames in which that channel falls
while on pit material 30 below speed 300. The status audit also runs when a
damage bar first becomes nonempty (damage at least 40), comparing all 240
status pixels in authoritative chunky memory and the displayed interleaved
bitplanes with the original-code-verified rectangle descriptors. These
observations do not alter damage, fuel, AI or collision state.

The gate requires observed damage, observed pit repair, all four four-lap
finishes/results and at least one visible damage-bar audit without pixel
failures. It fails on a 3600-update timeout; merely reaching the race does
not pass. Before the target run, `verify-dos-damage` passes its 576 full
original-routine arithmetic/gating/RNG cases and ten native delayed-damage
consumption fixtures, plus its existing airborne/shadow tests.

The first target run `.run/damage-menu-race` (port 24921) finished at update
2632 but **failed the damage gate**: all four damage peaks and repair-frame
counts were zero, so no visible damage-bar audit occurred. The three ordinary
status audits had zero pixel errors. This is not damage-path coverage merely
because the race completes. `CONFIGD` now selects the original maximum
damage value 300 using the submenu's Return action, retaining fuel 10; its
gate additionally verifies the actual enabled setup and prints collision
counts/maximum impact. No collision or damage state is injected to make the
test pass.

The maximum-damage `.run/damage-max-menu-race` (port 24922) also finishes
at update 2632 with zero damage/repair observations: 21 car contacts and
zero track contacts. Setup is verified as damage 300 with its master gate
enabled. The initial printed "MAX_IMPACT=0" was only the last update's
impact (the runtime clears it each update), not the race maximum. A separate
diagnostic now retains the peak across updates to distinguish subthreshold
contacts from a missing damage application. Both failed runs are retained
as evidence; the damage gate has not been weakened to accept them.

The corrected peak-observation run `.run/damage-peak-menu-race` (port 24923)
repeats the 2632-update finish and measures race-wide car-impact peak **256**.
The original delayed-damage expression `(impact-300)/4` does not become
positive until 304, so zero damage is correct for these 21 contacts even
at scale 300. BASIC is insufficient as a natural damage/repair fixture;
the strict damage gate remains failing/unproven rather than treating the
successful finish as coverage. All isolated emulators were stopped.

### Missing track-contact damage handoff

Inspection while investigating the natural gate found an independent
omission: original `c9fe..ca21` stores signed previous measured speed /3 in
the driver's pending-damage dword (DS:304b). Native track contact previously
only set contact state and sound. `record_track_contact` now also writes
that pending impact on every collision, including latched contacts; a
subsequent non-contact substep does not erase it. The existing update-tail
damage consumer remains responsible for thresholding/application/reset, and
a later car-pair impact retains the original ability to overwrite it.

The new oracle executes that original write and real long-division helper
for **120** speed/driver/latch cases, including negative and signed-dword
extremes; all match. Existing damage, driving, car-collision, surface-effect
and dirty/HUD tests pass, and the A1200 build succeeds. This verifies the
missing handoff, not the still-approximate moving-car material/layer/sampling
geometry or coordinate snap. The three natural BASIC runs above precede
this handoff fix and had zero track contacts; they cannot verify it on the
target. A genuine damaging track-contact run is still required.

### Original moving-car material predicate

`slicks_track_car_sample` now translates the complete positive-car `c5a0`
predicate: DS:3058 nonzero or DS:4daa zero suppresses map sampling; the
driver's DS:5388 layer overrides the caller's layer argument; material 2
blocks unconditionally; 22..26 block only when material-22 is no greater
than the signed, word-wrapped DS:4c6c-1. It uses the decoded lower/upper
material maps, not visible palette indices. Unsupported retained-map
addresses return -1, not invented empty terrain. Upper-plane coordinates
must be within 320x190 because signed x/4 outside that rectangle can alias
different packed bytes than the decoded map retains.

The oracle executes original `c5a0` and the real packed `b089` reader for
**344,064** combinations: four drivers/x alignments, every lower/upper
material pair, two layers, three signed special states, both sampling
latches and seven signed boundary levels including -32768/32767. It also
checks far return/stack balance and deliberately supplies an opposite layer
argument to prove the per-car override. All match. Suppressed null-map and
unsupported-address behavior has separate native checks.

Pit visibility now shares this predicate with explicit lower-layer,
sampling-enabled/non-special arguments (the original negative-car mode).
The existing 768 full original pit-route comparisons and all 195 real
tracks in service-off/service-on passes still pass. The full AI suite and
A1200 compilation also pass. This is not yet the moving-car integration:
`material_blocks_car`/the walker remain approximate. DS:4daa's lifecycle
must be translated with it: initialization clears it, special-state layer
handling at `22a4f..22a61` clears it, and `238eb..23961` conditionally
re-enables it only at an unobstructed position. The surface transition at
`232f9..2334f` also clears it and conditionally halves velocity. Those
state/response contracts are next; no changed car trajectories or completed
track-collision fidelity are claimed by the predicate tests.

### Moving-car sampling lifecycle integration

The moving-car walker and four-neighbour response now call the verified
`slicks_track_car_sample` predicate instead of the approximate material
classifier. Race initialization sets boundary level 5 and clears the per-car
sampling latch. Special-state layer handling clears that latch as in the DOS
update. Unsupported material-map reads stop the Amiga diagnostic with error
10 rather than being treated as empty terrain.

The `238eb..2399f` rearming path is translated separately: current contact
(`536c`), previous contact (`5370`) and special state each prevent rearming.
Otherwise a disabled latch is tentatively enabled and cleared again if the
car centre is blocked. The safe-position word/byte are updated on this path.
The per-update tail copies current contact into previous contact before
clearing current contact; previous contact is not a jump-sound flag.

The real material-27 dispatch (`232f6..23357`) halves signed velocities only
while sampling is enabled, disables sampling, and sets previous contact.
It runs before rearming, damage consumption and special-state advancement.

Original-code differential verification passes **43,008** lifecycle cases
and **56** material-27 dispatch cases, including signed velocity extremes.
The full AI/damage suites and drive-physics, car-collision, surface-effect
and dirty-tracking host regressions pass after integration. The A1200 build
also succeeds. These tests do not prove the moving ray walk or its response
geometry: those remain approximate, as does animated boundary-level state.
In particular the original walker discards the high product word during
interpolation and only ignores a blocked step zero; the native walk still
needs those contracts and collision-position writes checked against the
complete original routine.

The silent strict A1200/2 MiB CONFIGD run at
`amiga/.run/collision-lifecycle` (port 24924) reaches 3,600 updates without
unsupported-map errors. Unlike the preceding zero-track-contact run it
records 453 track contacts and 40 car contacts, with peak impact 567.
Per-car damage peaks are 157/208/167/149, repair-frame counts 44/22/0/27,
and all four fuel values end at -4. Three status audits, including the
first visible damage bar, have zero pixel failures. This demonstrates
natural damage and repair activity but **does not pass the strict race
gate**: laps end at 4/3/2/4 and no car has finished. The completion/repair
requirements remain unchanged. Exact moving-ray and response translation
is still needed before attributing these trajectories to the original.
The damage and BUMPS GDB gates now retain a separate collision-error trap
after disabling their startup breakpoint.

The companion real-BUMPS A1200 run
`amiga/.run/collision-lifecycle-jumps` (port 24925) **passes** its existing
700-update gate: 18 navigation zones, 41 takeoffs and 41 landings, 440 shadow
frames, peak special-state height 6384, and 41 jump-sound requests. No car
state was injected. Both target runs used silent host audio while retaining
emulated sound/DMA. Neither run changes the normal `run.sh` launch image.

### Original four-neighbour track response

Full `c63e` execution exposed errors that the earlier `c9fe..ca21` slice
could not detect. Opposed left/right samples keep the decayed X component
and reflect Y; opposed up/down samples reflect X and keep Y. The two
diagonal pairs respectively negate both swapped components or neither.
The old native implementation had incorrect signs in all four branches.

The velocity transform now preserves the original wrapped 32-bit product
before signed division by 16, including negation before division. Single
vertical/horizontal contacts write half the absolute reflected component
as pending damage. Other branches write previous measured speed divided
by three. `record_track_contact` no longer overwrites that branch-specific
result. The response also writes the original signed-word pixel-centre
coordinates (`x*100+50`, `y*100+50`).

The replacement oracle executes **12,800 complete original `c63e` calls**
with the real `c5a0`, packed `b089`, multiply/divide and absolute-value
helpers: four drivers, all 16 neighbour patterns, both layers, and 100
velocity pairs including signed extremes and overflowing products. Native
velocity, position and pending damage match; far return and stack balance
are checked. Subsequent first, repeated and cleared contact bookkeeping
must preserve the selected damage. This supersedes the earlier 120-case
slice test and its incorrect conclusion that all wall impacts use speed/3.

The full damage suite, existing drive/collision/surface/dirty host tests,
and A1200 build pass. The original `06c2` sound-request latch is not part of
this state comparison: native impact-sound scheduling remains a separate
fidelity item. The approximate moving ray and optional actor probing still
prevent a claim of complete track-collision equivalence.

The next walker oracle can use the actual car-movement call at `2131b`:
it supplies actor-probe flag zero, scale one, the current driver, and layer
argument zero (overridden by `c5a0`). Both output pointers alias the same
caller scratch word. Current and proposed fixed-point positions are
divided by 100 for the ray endpoints. A zero return copies the proposed
full-precision positions; a nonzero return keeps the response's position
and sets current contact. This distinguishes the normal movement path
from the other `cb02` use at `21bce` and avoids inventing an actor-probing
dependency for normal car movement.

Silent A1200 CONFIGD regression `amiga/.run/exact-track-response`, port
24926: after 3,600 updates cars 0/1/2 have finished at lap 5; car 3 remains
on lap 3. Damage peaks are 48/237/88/280, repair frames 16/30/19/65, fuel
768/0/0/2935. There are 31 car contacts, 73 track contacts and peak impact
517. All three HUD audits pass with zero pixel failures, including visible
damage, and no unsupported-map error occurs. The unchanged strict gate
still **fails** because the fourth car has not finished. This improves on
the preceding 453-contact/no-finish run but does not prove full collision
or AI fidelity. The normal launch image has not been promoted.

### Whole moving-car ray comparison

The car-movement specialization of `cb02` now ignores only a blocked
sample at step zero, not an arbitrary blocked prefix. An entirely blocked
one-pixel path was the first counterexample in the new original-code
oracle: DOS returned a contact while the native walker returned clear.
Zero-length rays now return without a material sample, matching the
actor-probe-disabled caller. Interpolation truncates each signed product
to its low word before division, as the original IMUL/CWD/IDIV sequence
does. The verified response owns the collision-position write.

The oracle executes the full original walker and real material/response
helpers. **1,100** calls match contact return, fixed-point final position,
both velocity components and pending damage, covering four drivers,
fourteen stationary/axial/diagonal/long paths, five obstacle patterns,
both layers and both sampling-latch states. No-hit position handling
matches the normal caller's proposed full-precision position write.
Another **20** long-ray cases fail loudly on unretained map data: a memory
read hook on the original independently confirms those calls access bytes
beyond the retained 60,800 lower / 15,200 packed-upper map bytes. They are
not counted as equivalent supported calls. All original calls must return
with a balanced far-call stack, including rejected-map cases.

The 12,800-case response oracle and existing damage, drive, car-collision,
surface-effect and dirty-tracking tests pass, and the A1200 build succeeds.
This covers the ordinary movement caller, not every `cb02` actor-probing
mode or unretained memory alias, and does not resolve the sound-request
latch, animated boundary state or remaining AI approximations.

The silent A1200 CONFIGD run `amiga/.run/exact-moving-ray` (port 24927)
ends with the same reported counters as the response-only run: three cars
finished, the fourth on lap 3 at update 3,600, 73 track contacts, 31 car
contacts, three HUD audits with zero pixel failures, and no unsupported
map error. Thus the corrected ray edge cases do not fix this remaining
race-completion failure. The strict gate remains failing and unchanged;
the normal launch image was not promoted. The full AI and damage oracle
suites also pass after the final walker test changes.

Next confirmed AI discrepancy: `f09d..f0d8` only calls `ebbb` when driver
byte `6936` is zero and global word `3020` (weapons) is nonzero; otherwise
it explicitly clears `2fb0`. Native `ai_controls` currently calls its
approximate predictor unconditionally and uses the result to suppress
braking, including the weapons-disabled normal race. That caller gate and
the predictor's actual consumers need original-code verification next.

### Weapons-disabled AI correction

Tracing `2fb0` shows it is a weapon-action request, not a general collision
avoidance result. `206a5` checks request 1 and selected weapon `2fac` before
weapon processing; `20c09` handles request 2 via `eb49` selection. The
`ebbb` lookup at `0166` is indexed by the selected weapon, not vehicle.
The old native approximation indexed it by vehicle, ran it with weapons
disabled, and suppressed braking on a predicted hit. That function, table
and unused counter are removed from the current weapons-disabled race
path. This is not an implementation of weapons-enabled gameplay, which
remains open rather than retaining a misleading approximation.

The original `f0a3..f0d8` gate passes **6,144** comparisons (four drivers,
all 256 service-state byte values and six probe counters including 10/11
and 255): with `3020=0`, it clears the weapon request and leaves the probe
counter untouched. An execution hook makes entering `ebbb` a test failure.
A separate native-tail fixture keeps all four cars close together for
32 calls at each heading and driver and verifies the retained brake is
not cleared. This uses state 3 solely to isolate the common tail; actual
steering branches remain covered by the existing original-code tests.
The full AI/damage suites, drive/collision/surface/dirty regressions and
A1200 build pass after removal.

Silent A1200 run `amiga/.run/no-spurious-ai-probe`, port 24928, has the
same reported 3,600-update outcome as the preceding ray-walker run: cars
0/1/2 finish, car 3 remains on lap 3; damage peaks 48/237/88/280, repair
frames 16/30/19/65, fuel 768/0/0/2935, 31 car contacts and 73 track contacts.
All three HUD audits pass. The strict race-completion gate still fails;
this correction does not explain the remaining non-finisher. The normal
launch image was not promoted. The diagnostic script now additionally
reports final per-car position, velocity, checkpoint/waypoint, AI/service
state, target, watchdog/recovery, controls and surface for the next run.

The isolated-tail fixture is also checked against **2,048 full original
`f09d` calls**, verifying controls, watchdog and balanced return stack for
all four drivers, sixteen headings and thirty-two consecutive updates.
Those comparisons and the complete AI suite pass. The oracle clears
Unicorn's translated-block cache afterward because later tests stop at
instruction boundaries inside that full routine; without clearing, a
cached whole-routine block can bypass a slice's requested stop address.

### Update-wide contact age and non-finisher state

The expanded baseline run `amiga/.run/nonfinisher-state`, port 24929,
confirms car 3 is **moving**, not stationary in a pit, at update 3,600:
position 22651/13395, velocity 158/1797, waypoint 2, checkpoint 1, AI state
0, service state 0, target -1/64, watchdog 150, recovery 398, controls 8,
surface 0 and fuel 2935. It remains on lap 3 while the other cars finished.
This single snapshot does not prove why it is slower or whether a longer
run would finish; the strict 3,600-update gate remains unchanged.

An independent source audit found `ai_update_contact_age` was testing
`touching_solid` (last movement-substep wall result), while original
`23d4a` tests DS:536c (`actor_contact`, latched across the update). A clear
later substep therefore hid an earlier contact from the native timer.
The previous test mapped its one boolean to both concepts and could not
expose the error. Varying both flags independently first reproduced the
mismatch; switching the production predicate to `actor_contact` makes
all **1,024** timer cases match the original. The complete AI/damage and
drive/collision/surface/dirty host suites pass with the correction.

The same audit identified a separate remaining extra condition in
`update_actor_layer`: native bridge entry additionally tests
`!touching_solid`, whereas original `229d2..229f9` only tests current layer,
lower material, special state and update contact `536c`. The replay test
currently initializes `touching_solid` to zero, so it does not prove that
extra condition harmless. Keep this separate from the contact-age target
run and add independently varied flags before changing it.

The corrected timer's silent A1200 run `amiga/.run/update-contact-age`,
port 24930, completes the unchanged 3,600-update diagnostic with exactly
the same reported positions/AI states and race counters as the expanded
baseline above. Three cars finish and car 3 is still moving on lap 3;
the strict completion gate fails, while all three HUD audits pass without
pixel or retained-map errors. Thus this verified timer correction is not
the cause of the remaining completion failure in this fixture. The normal
launch image has not been promoted.

### Independent bridge-entry contact flags

The bridge-entry test now executes original `229d2..22a61` for **196,608**
combinations: four drivers, every lower/upper material pair, both initial
layers, three signed special states, both sampling-latch states and
independently varied update-contact/last-wall flags. Before correction,
the first counterexample was both materials zero, lower layer, no special
state or update contact, but a stale last-wall flag: DOS enters the upper
layer and native did not. Removing the extra `touching_solid` check makes
layer, selected/effective surface and sampling latch match every case.
This tests those four outputs, not other side effects outside the native
layer helper. Existing AI/sampling comparisons continue to pass.

The CONFIG diagnostic now exposes sparse car-3 progress checkpoints at
lap/service transitions and every 600 updates, without per-frame debugger
stops or target-state injection. Collision-failure breakpoints use a named
diagnostic hook instead of a fragile source line number in both the damage
race and BUMPS scripts. Normal runs do not enable the progress checkpoints.

The old surface-effect fixture expected the stale last-wall flag to block
entry too. Its expectation is corrected using the independent original
oracle above, not a new native screenshot or checksum. The surface-effect
and dirty-tracking suites then pass, alongside AI, damage, drive and
car-collision regressions; the A1200 build also passes.

The silent strict A1200 run `amiga/.run/layer-and-progress` (port 24931)
still ends at update 3,600 with the same three finishers and car 3 on lap
3, with identical reported terminal state/counters to the preceding run.
HUD audits pass and no collision-map error occurs. New progress checkpoints
show car 3 starts lap 2 at frame 404 and lap 3 at frame 2,858. Its fuel
service request-to-departure intervals are 463..831, 1216..1618,
1981..2540 and 2914..3538: 1,953 of the first 3,600 updates occur with a
service request active. It repeatedly refuels/repairs and returns to route
following; at frame 3,600 it is moving with fuel 2935 and damage 0.

This supports repeated service delays rather than a permanently stuck
car, but does not establish original-game trajectory equivalence or prove
eventual completion. Next use a separately labelled, bounded extended
observation to find whether it finishes naturally; retain the failing
3,600-update gate unchanged instead of tuning gameplay to its deadline.
The normal launch image has not been promoted.

### Separate bounded natural-finish observation

`amiga/diag_damage_extended.gdb` uses the same CONFIGD menu path and
unchanged game binary as the strict damage race. It reports the existing
3,600-update checkpoint but continues, with a separate 7,200-update limit.
Success requires four lap-5 finishers with unique places 1..4, race-complete
and results-drawn state, observed damage/repair activity, visible damage
HUD checks and zero HUD pixel failures. Collision/load errors fail
immediately. It uses existing sparse progress callbacks and no writes to
target memory. It does not replace or relax `diag_damage_race.gdb`.

Run through the usual isolated debug environment with
`SLICKS_DAMAGE_RACE=1 ./debug.sh "$KICKSTART" diag_damage_extended.gdb`.
Host audio remains muted while emulated audio and DMA stay enabled.

The strict A1200/2 MiB run `amiga/.run/natural-damage-finish`, port 24932,
**passes the separate natural-finish observation**. Car 3 starts lap 4 at
update 3,824 and finishes at **4,085**, triggering the native results screen.
All four finish at lap 5 with places **3/2/1/4**. Damage peaks are
48/237/88/280; repair-frame counts 16/30/19/65. Four HUD audits, including
the results transition and a visible damage bar, report zero pixel failures.
No collision-map or load error occurs. Final fuel is 0/0/0/-4.

No production gameplay change or target state injection was needed for
that completion. The previously observed fourth car was delayed by service
trips, not permanently stuck. The original strict 3,600-update gate remains
failing and unchanged; it must not be cited as passing. This target result
proves natural completion and the tested native results transition for this
scenario, not original-DOS trajectory or complete results-layout fidelity.
The emulator was stopped after the observation and the normal launch image
was not promoted.

### Original track-information HUD commands

The native command generator now translates `2adbe..2aeb5`: an eight-character
filename stem at (5,186), followed by the second stored track-record time at
(10,193) only when DS:696b is signed-positive. Time formatting shares the
original unsigned clamp to 17999 and truncating 5/9 conversion with driver
timers. Text/run buffers include room for all eight filename characters and
the terminator.

`verify-dos-hud` executes the original routine and filename-table lookup for
40 cases, including nonidentity track selections, eight-character names,
zero/negative signed times and clamp boundaries. All text commands match;
palette selection and rasterization are stubbed in this particular check.
The existing 1600 driver-command cases and 12 composed four-driver pixel
transitions also pass.

The follow-up implements `1a62e..1a7e9` record loading, including big-endian
stream checksums around little-endian payload words, name termination,
signed time normalization, cumulative positive-record gating of checksum
errors, and the final checksum/trailer. The native loader additionally
rejects truncated buffers explicitly. A 364-case differential test executes
the original loader with file I/O substituted: the real BASIC header and a
one-byte mutation at each of its 363 positions. Loaded bytes, return values,
trailer and fatal paths match. Every shorter BASIC prefix is rejected.

Production race loading now supplies the filename stem and second stored
record to the left-side HUD, with original nearest-palette RGB (55,55,65).
The composed HUD oracle includes the real original track-name/record renderer
and matches the complete visible surface through 12 four-driver transitions.
For comparison, its page-0-only name is mirrored onto page 1; palette search
is fixed to the same colour on both sides. Status/weapon calls remain outside
this test. Host dirty/HUD regressions pass.

The fresh muted A1200/2 MiB run `.run/track-hud`, port 24945, completed
700 updates with `SLICKS_HUD_TARGET_OK FRAME=700 FONT_CALLS=26 ERROR=0`.
Startup reported ten translated-font calls, track BASIC and record 822.
Both native framebuffer dumps were inspected: BASIC/04.56 remain visible
at startup and update 700. The per-update bitmap audit reported no failure.
The emulator stopped on completion; the normal launch image was not
promoted. This verifies the integrated track/driver HUD, not full-HUD or
complete original-game fidelity.

### Remaining weapon HUD dependencies (original-code trace)

`1d9b6` exits for inactive DS:4bc6 slots. It clears the status rectangle
(106+60*i,187)..(126+60*i,190) when any of weapons/fuel/damage is enabled.
The weapon branch requires DS:3020 nonzero and signed DS:2fac[i] nonnegative.
Its icon index is the selected weapon byte plus five, with byte wrapping.
The ammunition bar uses the signed low-word product of inventory
DS:6a7a[13*i+index] and 20, divided by signed capacity byte DS:10a4[index].
This is not an unsigned or saturating percentage calculation.

The icon update first restores saved 16x8 background DS:4be2[i] at
(90+60*i,192) on both pages via `3aaf2`, then calls transparent `3aa7c`
at (91+60*i,192) with the far icon pointer DS:6b1a[index]. The ammunition
colour is nearest RGB (33,33,70), distinct from the three currently native
fuel/damage/background colours.

`slicks_hud_weapon` now translates this command layout and signed low-word
arithmetic. 8,640 original-routine cases cover all four drivers, inactive
and disabled states, selection -1 and all eight weapons, signed capacity
boundaries, product wrapping, divide-by-zero and quotient overflow. The
oracle executes the complete `1d9b6` with graphics boundaries captured,
checks both-page bar/restore/icon command sequences, and handles original
INT 0 faults. Each case restores a clean Unicorn CPU context: otherwise its
pending exception state contaminates later fault cases. Real icon loading,
composed weapon pixels and live inventory/menu integration remain pending;
the command helper does not enable weapons in production yet.

### Original weapon selection state

`weapon_state.h` translates complete `1eb49..1ebba`. It searches weapon
inventory slots 5..12 after the current selection, then wraps through the
current slot inclusively. A signed inventory count must exceed one; zero,
one and negative counts are not selectable. No available weapon returns -1.
Original current selections >=8 normalize to zero before searching. Invalid
negative selections below -1 fail explicitly rather than reading outside
the inventory. This invalid-state guard is not claimed as DOS equivalence.

11,264 original-machine-code comparisons pass: all 256 availability masks,
four drivers, selections -1..8 and 127, available counts 2/32767, unavailable
counts -32768/-1/0/1, plus wraparound and stack return checks. This is the
selection transition used at `20c1d` after a selection/depletion request;
the surrounding firing/depletion state machine remains unported.

The setup trace at `26394..263de` clears all thirteen inventory slots and,
when DS:3022 is zero, sets slots whose DS:106f flags include bit 0 to four.
This is conditional setup behavior, not permission to seed every weapon
with ammunition. Saved setup loading, item flags/capacities and runtime
inventory producers still need integration before enabling weapons.

### Original HUD icon assets

The startup `2ea5d` group at `1a03a` loads thirteen image resources: vir1,
vir2, vir3, vir4, vir_fuel, then vir5 through vir12. The last eight are the
weapon HUD icons selected by index+5. Production now loads these eight
members directly from SLICKS.000 into bounded native icon buffers, retaining
their individual dimensions. No extracted or captured images are used.

The generalized HUD image decoder is checked against complete original
`2e51a` execution for the background and all thirteen icons. Every visible
pixel matches, and every truncated member is rejected. The original padded
VGA representation stores a trailing byte equal to rounded width minus real
width. Additionally vir7 and vir10 encode one excess final RLE pixel. The
original decoder writes it into the next VGA bank before its signed remaining
count terminates decoding. The native decoder reproduces bounded planar-bank
spill effects instead of discarding that write or overrunning the chunky
buffer. All eight production icon loads and existing HUD regressions pass.
The prior 700-update A1200 audit predates this asset-loading change; a new
target run is still required. Icon drawing/live weapon-state integration
remain pending.

### Direct chunky transparent icon blitter

`sgfx_chunky_transparent_blit.s` lowers the original `3aa7c` transparent
stores into the authoritative chunky surface. It reads decoded row-major
pixels, skips zero, uses the shared mult320 table, and preserves all data
and address registers. The caller must validate the complete on-screen
rectangle; this is not a clipping or emulated-VGA fallback routine.

`verify-hud-background` now executes both original `3aa7c` on the actual
original-decoded planar assets and the native 68020 routine on decoded
chunky assets. Across thirteen icons, six positions (including all four
HUD slots and the right edge) and three backgrounds, 234 full-surface
comparisons pass, with register and stack preservation checked. This checks
transparent pixels as well as painted pixels. The assembly object is in
the Amiga build; live weapon-state/renderer integration is still pending.

### Weapon HUD connected to native state

The race runtime now owns four 13-slot inventories, signed selections and
capacities, and an explicit weapons-enabled flag. Initialization selects -1
and leaves weapons disabled; it does not fabricate ammunition or enable
unimplemented firing. `slicks_race_draw_status` consumes those state fields,
validates weapon divisions/assets before painting, clears the original bar
region, draws the ammunition bar, restores the original 16x8 icon background,
and calls the direct 68020 transparent blitter through its GCC bridge.
Palette selection includes the original ammunition RGB (33,33,70).

Icon changes compare against the expected real asset/background pixels, not
a framebuffer shadow. Only changed icon regions are restored, drawn and
marked dirty; optional logical VGA mirroring follows the chunky write.
The supported original weapon assets are at most 15x8, fitting the one-pixel
inset within their 16x8 restore rectangles.

The composed DOS HUD test no longer stubs `1d9b6`: all original bar, restore
and transparent-icon drawing executes alongside driver/track text. Twelve
four-driver transitions cover all eight icons, varying ammunition and a
no-selection state; all visible pixels match on both pages (page-0-only
track name mirrored). Fuel/damage are disabled in this composed fixture;
their separate regression tests remain applicable, not proof of every
combined weapon/fuel/damage state. The blitter also passes 234 additional
full-frame comparisons through the production GCC bridge, with callee-saved
registers and stack checked. A fresh target run is still needed after this
integration. Setup loading and gameplay inventory/firing producers remain
open; the normal game still runs with weapons disabled.

### Combined status options and cache invalidation

The composed oracle now covers 32 four-driver transitions with every
weapon/fuel/damage enable combination, both refuelling blink phases,
varying fuel/damage/ammunition, and selection changes. Every visible pixel
matches complete original HUD execution. This exposed a production cache
bug: once formatted lap times stopped changing, disabling weapons could
leave the previous icon behind. The original `ddc0` restored the panel,
whereas native text-only cache matching skipped that restore. Driver HUD
cache keys now also include status options and effective weapon selection.
The expanded oracle and dirty/HUD regressions pass after this fix.

The separate muted A1200 renderer fixture `.run/weapon-hud`, port 24946,
was started using the preceding integrated build before this cache fix.
It explicitly supplies four weapon selections/counts through GDB; it is
not evidence of gameplay firing or inventory producers. Its 700-update
dirty-region audit is still pending at this checkpoint. The executable
and symbols must not be rebuilt while that debugger is running. On
completion the fixture dumps both chunky and actual interleaved bitplanes
for a full pixel comparison. A target rebuild including the cache fix
remains required after that session ends.

### Target weapon fixture validation

The first weapon-HUD fixture reached update 700 with error zero but failed
its required icon-call count (zero calls), so it is **not a passing weapon
test**. A second run stopped early: immediate readback after GDB writes
still showed weapons disabled and selections -1, and the status-renderer
entry confirmed that state. These debugger writes did not take effect;
do not use either run as renderer coverage or assume injected state applied.

The replacement explicit `WEAPONHUD` diagnostic mode initializes its state
in native code, with no change to normal launch behavior. It tests icons
0..3 initially, 4..7 after update 350, disables weapons after 500, and
re-enables the first group after 550. GDB now observes only. The cache fix
is included in this rebuilt target. The muted `.run/weapon-hud-v3` run,
port 24948, is in progress; do not rebuild its ELF while it runs.

`tools/verify_amiga_frame.py` checks all 64,000 target pixels from the
actual interleaved eight-plane dump against the authoritative chunky dump.
Its zero-frame and deliberately flipped last-pixel/last-plane positive
control pass. This is separate from the per-update dirty audit, which
detects writes outside declared dirty regions rather than proving every
pixel inside a changed region is correct. Final target dumps are still
pending at this checkpoint.

### Weapon HUD target verification completed

The native `.run/weapon-hud-v3` fixture completed with
`SLICKS_WEAPON_HUD_OK FRAME=700 ICON_CALLS=20 ERROR=0`. Entry-state readback
confirmed enabled=1, selections 0/1/2/3 and loaded icons. No dirty-region
audit failure occurred through its 700 updates and scheduled icon/off/on
transitions. The final actual interleaved bitplane dump matches all 64,000
chunky pixels using `verify_amiga_frame.py`; the resulting image was also
inspected. FS-UAE stopped on completion. Normal launch remains unchanged,
with weapons disabled; this fixture is renderer evidence, not gameplay
firing/depletion or setup fidelity.

The original `2bb70` setup writes participation values 0, 1 and 255. Both
`1ddc0` and `1d9b6` gate on zero/nonzero rather than requiring value 1.
The driver-command oracle now covers all three values: 2,400 complete
command sequences pass. Native inactive-player setup/gameplay handling is
still pending and must preserve the distinction between the signed states.

### Original new-game inventory producer

`slicks_new_game_inventory` lowers `26372..263e6`: all four cash words
receive DS:302a; all 13 inventory words per player are cleared, then receive
4 only when DS:3022 is zero and the corresponding DS:106f flag has bit zero
set. This is a new-game operation, not a per-race reset. It must not overwrite
saved-game inventory or grants made between races.

`make verify-dos-hud` now executes that complete original loop for all 8,192
item-flag masks with modes 0, 1 and -32768 (24,576 cases). Upper flag bits and
starting cash vary; every cash and inventory word is compared. All previous
HUD/selection/record comparisons also pass. This producer is verified but
not yet wired into production: the native new-game/setup owner, item-table
loading and profile interpretation remain open. No production ammunition
has been invented or enabled by this change.

Setup research separates three distinct persistence paths:

- `2b486..2b709` reads SLICKS.CFG, including selected profile indices at
  DS:044c, not the 13-word race inventory. It validates version 0x0f and a
  byte derived by `35ee0` from the BIOS date at F000:FFF5. An Amiga loader
  will need an explicit policy for this machine-dependent signature.
- `2b997` reads SLICKS.PLR, initially reserving three built-in profiles.
- `1d20c..1d583` is the separate saved-game loader and restores inventories;
  it must not be mistaken for ordinary configuration loading.

The CFG reader also unconditionally copies DS:172c to DS:172e after closing
the stream, overriding the latter's loaded byte. Preserve this post-load
behavior when translating the full loader, rather than exposing raw fields
as final setup state.

### Complete selected-profile setup translation

`src/game/profile_setup.h` now translates the entire `2bb70..2bdd7` routine,
with explicit callbacks for the override-mode query (`198b6`), vehicle
chooser (`19967`) and vehicle asset/property loader (`1ccfc`). These callback
implementations are not claimed by this port. `make verify-profile-setup`
executes the original routine and compares all four selected indices,
participation bytes, vehicle bytes, six-byte control maps, the four-entry
ordering array, count, and callback arguments/order: 46,080 cases pass.

Coverage includes every profile flag byte, independent suppression on/off,
negative/zero/out-of-range selections, per-slot override results, signed
chooser modes, high-byte vehicle choices, retained negative vehicle bytes,
and zero/negative/normal/maximum signed vehicle counts. Flag 6 alone selects
successive fallback control maps; merely having bit 1 set is insufficient.
The count increases for a positive selected profile even if its participation
was suppressed. Ordering puts negative participation first; the original's
remaining-slot fill behavior is retained, not replaced with a sort.

This verified setup translation is not yet connected to the production race:
PLR/built-in profile loading, the three callback bodies, and propagation of
participation into simulation/rendering still need integration. In particular,
passing this test does not prove the `1ccfc` asset/property loader itself.

### Original profile override and weighted vehicle choice

`slicks_profiles_override` now implements `198b6` (game type exactly 5).
`slicks_choose_profile_vehicle` implements `19967..199ce`, including one
Borland RNG advance for empty/zero-weight tables, signed wrapping 16-bit
weight sums and signed scaled-draw comparisons. The test executes both
original routines without replacing their RNG or arithmetic helpers:
all 65,536 game-type words and 7,680 weighted-choice cases match, including
the post-call RNG state. Weight tests include negative/zero counts, zero
weights, sparse tables, signed sum overflow and indices beyond 255.

The existing 46,080 complete profile-setup comparisons also pass. Their
callback-order tests run in a fresh Unicorn engine after the real-code
tests: reusing already-translated callee blocks after adding hooks caused
the first attempt to bypass the new boundaries. The two helpers are native
and independently proved, but connecting their state to production setup
and translating/integrating `1ccfc` remain unfinished. No new A1200 run or
full composed production-setup claim is made for these host comparisons.

### Vehicle-selected bias and road threshold connected

Auditing `1ccfc` found two remaining driver-slot constants in production.
The property loader now decodes `.omi` byte 23 as `(byte - 100) * 2`, exactly
as `1d01a..1d029`, and race startup takes drive bias from the selected vehicle
instead of `{0,-8,0,0}`. Road-cloud emission now uses the selected vehicle's
already-loaded byte 24 (`effect_profile`, DOS DS:4ee0) instead of the fixed
four-driver threshold table. This fixes these two consumers when changing
vehicle assignments; it does not claim the remaining wheel geometry tables
or complete setup/asset-loading path have been generalized.

`make verify-vehicle-properties` executes original `1cdba..1d142` with only
file-byte reads/close substituted. It compares the bias word and road
threshold byte for all four destinations, ten actual archived `.omi` assets,
and all 256 values of each affected input byte: 10,280 comparisons pass.
It intentionally does not claim coverage of subsequent fatal checks,
sprite generation, or every other property field. Surface-effect, physics
and dirty-region regressions pass, including a new driver-1/vehicle-9
threshold regression. The A1200 executable rebuilds; no new emulator timing
or live-race result is claimed in this step.

### A1200 regression after selected-vehicle property fixes

Fresh `.run/weapon-hud-v4` on debugger port 24949 completed on the configured
PAL A1200, 2 MiB Chip RAM, no Fast RAM, host audio muted (emulated audio/DMA
still enabled): `SLICKS_WEAPON_HUD_OK FRAME=700 ICON_CALLS=19 ERROR=0`.
The bitmap-audit failure breakpoint did not fire. The final actual
interleaved bitplanes match every one of the 64,000 authoritative chunky
pixels with `tools/verify_amiga_frame.py`. FS-UAE was stopped by the run's
exit trap; the normal launch image was not promoted. This remains an
explicit weapon-renderer fixture, not proof of firing/depletion or full
setup fidelity, and its instrumented runtime is not a speed measurement.

Next vehicle-selection integration gap: `emit_wheel_surface` still indexes
the four captured wheel-coordinate tables by driver slot. Original `1ccfc`
finishes by calling `1c3fc` after constructing/remapping the sixteen vehicle
sprites. Recover that geometry producer before claiming tyre-position parity
for arbitrary selected cars; do not just substitute a vehicle index into a
four-entry driver table when ten vehicle models are available.

### Wheel marker extraction recovered

Correction to the prior investigation lead: `1c3fc` creates the shadow
bitmap; `1c2e9..1c3fb` extracts wheel coordinates from each rotated car sprite
before driver-colour remapping. It scans columns before rows, records 255
markers (remapping to 25), and accepts 254 markers only until two wheels
have been found (remapping those to 184). Missing wheels set X=-1 but leave
their Y untouched. The gameplay consumer at `22329..22332` skips negative X.
Original assets include zero-, one- and two-marker vehicles, so retaining
two fixed sample points for every selected model is not equivalent.

`src/game/wheel_geometry.h` translates this scan. A third accepted 255 marker
returns an explicit native error rather than overwriting unrelated memory
as DOS would. `make verify-wheel-geometry` passes 672 original-code
comparisons: ten vehicles, sixteen supplied orientations, four destinations,
plus synthetic missing/mixed/saturated-marker cases. Both coordinates and
the complete modified planar sprite allocation match; the original pixel
read/write helpers execute, not mocks. This verifies extraction from the
same supplied pixels, not original rotation generation. Production still
uses the old tables until `3595b` rotation/padding equivalence is verified
and per-vehicle geometry plus marker colour remapping are integrated.

The test exposed a host-only archive reader mismatch: zero-length `car9`
marker entries stopped lookup before the actual duplicate-name payload.
`host_archive.h` now skips such markers like the existing target loader;
all ten models are included rather than omitting vehicle 9.

### Per-vehicle wheels integrated into production

The wheel oracle now executes original `3595b` rotations as well: 640
full-allocation comparisons include header, planar pixels, padding, real-width
trailer and untouched allocation tail. Current native rotation matches for
every supplied vehicle/base sprite/quarter-turn. The subsequent 672
extraction comparisons still pass. Another 640 comparisons now exercise
`slicks_race_add_car_sprite` itself, matching its wheel coordinates and
remapped visible pixels to original rotation/extraction output.

Production no longer uses the captured four-driver wheel tables. Each base
sprite stores two coordinates for each quarter-turn; the emitter selects by
actual vehicle and heading, skipping negative-X missing-wheel entries like
original `22329..22332`. The loader replaces accepted 255/254 markers with
25/184 before rendering. All supplied models have at most two markers;
assets with more are explicitly rejected because supporting them can require
different marker remaps per orientation. Two extra surface regression cases
verify one/no-wheel behavior and that missing wheels do not advance RNG.

Wheel, surface-effect and dirty-region tests pass; the A1200 build succeeds.
These production changes alter particle counts/RNG relative to the old
incorrect two-wheel tables, particularly the one-marker bike and zero-marker
vehicle. Do not preserve old trajectory/particle-count expectations merely
to keep fixture hashes stable. A fresh live A1200 run for this geometry
change remains pending (the previous v4 run predates it).

### Saved player-profile decoding

`src/game/player_profiles.h` translates `2b997..2bb6f` into a bounded PLR
decoder. Version 0x97 is followed by a big-endian count of profiles appended
to the caller-supplied three built-ins. Each 58-byte record contains 21 raw
name bytes, nine big-endian statistics, one discarded byte, one setting byte,
nine coefficients decoded by subtracting 40 with byte wrap, vehicle/flags,
and six control bytes. Postprocessing converts settings below 10 to 100
for every active profile, including built-ins on unrecognized/absent input.
Names are not implicitly null-terminated; UI consumers must retain bounds.

`make verify-player-profiles` runs the entire original reader with only
open/get-byte/close replaced by a stream, retaining the original BE-word
reader. 784 comparisons pass across every supported extra count 0..97,
varied byte payloads and invalid headers. Names, all statistics, coefficients,
settings, flags, vehicle choices, controls, count and stream consumption are
compared, including untouched profiles beyond the loaded count. Native
truncation checks cover every nonempty prefix of a maximum-sized file and
prove no partial state mutation; capacity overflow and absent-file handling
are checked too. These guards intentionally avoid DOS's unchecked EOF and
out-of-bounds behavior.

The decoder is not yet called by the Amiga setup path. Built-in profile data,
CFG selections and production ownership/lifetime of the profile collection
still need integration; passing the decoder oracle alone does not complete
native menu-driven setup or inactive-player gameplay propagation.

### Wheel-geometry A1200 regression passed

Fresh `.run/weapon-hud-v5`, port 24950, completed the 700-update renderer
fixture with `ICON_CALLS=19 ERROR=0` and no bitmap-audit failure. It uses the
new per-vehicle wheel extraction and marker colour remaps on PAL A1200,
2 MiB Chip RAM/no Fast RAM. All 64,000 final displayed pixels match the
authoritative chunky image. Debug host audio was muted without disabling
emulated audio; FS-UAE stopped on completion. The normal launch image is
unchanged. This closes the pending geometry-build renderer regression,
not the remaining setup, full-race fidelity or performance requirements.

### Original configuration decoder and postprocessing

`src/game/configuration.h` translates `2b486..2b709` state handling for the
142-byte CFG file. It retains the explicit version-15/machine-signature gate,
15 option words, four selected-profile indices and input bytes, key/binding
arrays and the remaining settings. It reproduces unconditional date-code
initialization and default 30 at DS:172c, plus the post-load copy to DS:172e
that discards the separately serialized value. Unresolved settings retain
address-based names rather than inferred semantics. Volume application is
represented separately by the original low-byte/clamp-to-100 calculation;
native audio hookup is not claimed.

`make verify-configuration` executes the complete original loader including
the real BIOS-date signature calculation and final volume-setting routine
(audio lookup-table rebuilding disabled). Only open/get-byte/close are
substituted. 1,024 cases compare the entire 64-KiB data segment, stream
consumption and return state across varied payload bytes/date inputs and
valid, bad-version, bad-signature and absent files. All pass. Every one of
the 141 nonempty truncated valid-header prefixes is rejected atomically by
the native guard rather than reproducing DOS unchecked EOF behavior.

The native caller must explicitly supply the expected machine-signature
byte; no permissive import policy is silently substituted. Built-in defaults,
profile collection ownership, loading this configuration in Amiga startup,
and propagating selections/participation into the race remain open. This
decoder is not yet production-connected, so these host results do not claim
working complete menu setup.

### Built-in profile initializer and startup ordering

`slicks_set_builtin_profiles` now translates `2ac53..2acea`. It copies the
three caller-supplied labels including terminators without clearing name
tails; sets flags 7,7,6; sets profile 1's vehicle byte to count+1 and profile
2's to count (byte wrap); and sets settings 1/2 to 100. It does not reset
profile count, controls, statistics, coefficients, profile-0 vehicle/setting
or unused records. Bounded label validation is a native safety guard.

768 original-initializer comparisons pass, including all low-byte vehicle
counts, varied upper bytes, empty/nonempty labels, and preserved state.
Original string-copy code executes. The existing 784 complete PLR-loader
comparisons still pass. Labels remain supplied by the caller, not embedded
as extracted binary resources.

The original startup reads PLR at `25bfb`, optionally CFG at `25c0c`, then
later initializes built-in labels/flags at `2614b` and selects profiles at
`26154` with suppress=0, choose-mode=1. Do not initialize built-ins before
PLR and assume these mutations commute: loader postprocessing also touches
built-in settings. Initial selected profiles from original data are 2,1,1,1;
full startup default/control data still needs a deliberate native source
and production integration. The current Amiga title menu still uses its
older selected-vehicle/track/lap variables, not the recovered profile chain.

### Configuration startup defaults

`slicks_configuration_defaults` now decodes typed configuration fields from
the original initialized DS data, with no dependency on an in-game snapshot
or the unpacked file's uninitialized storage. It applies the startup BSS clear,
the main routine's 40 defaults at 172c/172e, and the 20-key copy from 06ac.
`verify-configuration` compares all 64 KiB of DS after executing the corresponding
original entry/clear/default/key-copy instruction slices: 256 input variants,
including the supplied initialized data and poisoned BSS, pass. Short inputs
are rejected without changing the destination. The existing 1,024 full CFG
loader comparisons and 141 truncation checks still pass.

The intervening original routine at 2c03e obtains local time and writes the
day/month/year inputs at 4db4/4db6/4db8 (day becomes zero for years before 1997);
it does not overwrite the configuration fields decoded here. These slice tests
do not cover DOS environment setup or command-line overrides. Production setup
ownership, delivery of the initialized data resource, and menu/profile wiring
remain pending; the defaults helper is not yet called by the Amiga startup.

`make setup-defaults` now builds `src/gen/setup_defaults.h` from the supplied,
independently unpacked executable using `tools/export_setup_defaults.c` and
the verified decoder. The output contains only a typed configuration constant,
not a DS dump or a runtime CPU context. It is ignored by Git along with all
other generated original-derived data. Source/tool changes invalidate it;
missing or stale unpacked input is rebuilt through the existing unpacker.
`verify-configuration` compiles this generated header and compares every
configuration field against the original-data decode before its existing
original-instruction tests. Those comparisons pass. No production setting is
changed yet: the raw custom-option fuel/damage defaults must pass through the
original game-mode selection logic rather than being applied to every race.

### Original game-mode option resolution

`src/game/race_options.h` ports the complete `2bdd8..2bf38` resolver. Custom
mode 4 copies nine option fields, with signed fuel values <=5 disabled. Other
modes use the supplied original DS:1157 mode flag byte: inventory mode is bit
0, 3020 retains bit 1 as 0/2, fuel is disabled, and damage is bit 3 times 12
(0/96, not 0/100). The remaining preset values follow the original routine.
Unproved field meanings retain address-based names. The local-only generated
setup resource now includes the six original mode flag bytes.

`make verify-race-options` executes the complete original routine without
stubbing any calls and compares the entire 64 KiB DS state: 66,816 cases pass,
covering every signed custom fuel word and every flag byte in all five preset
mode slots. Other custom words vary through signed/wrapped values. Generated
mode flags are checked against the supplied executable. This resolves the
setup-to-race option mapping; production menu/state wiring remains unfinished.

### Configuration-backed production fuel/damage setup

Amiga startup now owns a typed configuration initialized from the generated
original data. Race preparation calls the original mode resolver and applies
its fuel/damage outputs. The existing native service-options screen edits
configuration options 9/10 and selects Custom mode on entry, through both
keyboard and mouse paths. Normal mode 0 retains fuel/damage disabled, despite
nonzero stored Custom values. This native screen is still not the complete
original menu; preset selection, profile ownership, inventories and other
resolved fields remain to be connected. CFG file reading is not wired yet.

The Amiga build generates/rebuilds the ignored typed resource as needed.
CONFIG/CONFIGD explicitly seed a zero diagnostic baseline; FUEL explicitly
overrides mode/fuel/damage in a temporary configuration. They do not change
normal defaults. On PAL A1200 with 2 MiB chip/no Fast RAM, muted debug runs
passed `SERVICE_MENU_OK FUEL=10 DAMAGE=20 CAPACITY=3345 DRAWS=5`
(`.run/setup-options-v1`, port 24951) and
`SETUP_DEFAULTS_OK FUEL=0 DAMAGE=0 ERROR=0`
(`.run/setup-defaults-v1`, port 24952). Both sessions stopped afterward.
Resolver/configuration and HUD/dirty-region host regressions also pass.

### Native player-profile startup ownership

Amiga startup now owns `g_slicks_profiles`, a typed native profile collection
that survives races. Before hardware takeover it initializes the original
100 six-byte control records, clears the other fields as C startup does,
loads an optional `SLICKS.PLR` from the working directory, and applies the
original built-in initializer after the file loader. The temporary file buffer
is freed immediately; malformed recognized files fail startup. Profile controls
and the three built-in labels are generated from initialized original data
under ignored `src/gen/`, not captured runtime state or committed asset bytes.

The profile verifier executes the original startup clear against poisoned BSS
and checks every field of all 100 native records plus the generated labels.
This passes, as do the existing 784 full PLR-loader and 768 built-in initializer
comparisons. Configuration and mode-resolution regressions and the Amiga build
also pass. This introduces real startup-owned profile data, but profile-to-car,
participation and input selection are still not wired into the active race.

The muted PAL A1200/2 MiB/no-Fast run `.run/profile-defaults-v1` (port 24953)
passed `SETUP_BUILTIN_PROFILES_OK COUNT=3` and
`SETUP_DEFAULTS_OK FUEL=0 DAMAGE=0 ERROR=0`, reaching the race with the original
built-in flags, special vehicle selectors and normalized settings. This live
test covers the absent-PLR path; file payload coverage is currently host/oracle
only. The emulator session was stopped afterward.

### Profile field correction and exact car-palette interpolation

Tracing the six-byte profile records through DS:310c into `19d04..19dad`
proves they are two RGB endpoints, NOT keyboard controls. Earlier notes calling
them controls are superseded. The typed profile/selection fields, generated
resource names and tests now call them colours. Input ownership still needs
separate recovery; these bytes must never be wired into keyboard handling.

The complete original car-palette interpolator is now native and used by
`prepare_race_palette`. It computes each signed endpoint contribution with a
separate truncating divide by four, then adds the low bytes; the old single
linear interpolation had different rounding. All 65,536 signed endpoint
pairs pass complete original-routine comparisons, including all four car
ramps and untouched palette entries. Profile startup, selection and weighted
choice oracle suites still pass after the naming correction. The existing
four default endpoint pairs are still the production input; selecting the
endpoints from the loaded profiles remains pending, as does vehicle/input
wiring. No new live display or performance measurement is claimed here.

### Profile-selected production car colours

Race palette preparation no longer contains four hard-coded RGB endpoint
pairs. It uses the loaded profiles and configured profile indices, the
original generated fallback colours at DS:0433, and the original mode-5
override count at DS:0f1a. The colour-selection stages are shared with the
complete profile-selection translation; this rendering-only projection does
not reroll vehicles or run property-loading callbacks. Inactive slots retain
their previous endpoints, initially zero, matching DS:310c lifetime.

The full profile-selector suite now has 69,120 original-routine comparisons.
Its non-overridden and uniformly overridden cases additionally compare the
rendering-only colour projection directly to the original output. Generated
fallback/override data is verified against the supplied executable. All
profile-loading and 65,536 palette-interpolation comparisons still pass, and
the Amiga build succeeds. Vehicle selection, race participation and keyboard
input remain separate unfinished integration work.

Muted PAL A1200/2 MiB/no-Fast run `.run/profile-colours-v1` (port 24954)
reached the race and passed both profile/default startup checks. The actual
palette passed to the HUD was dumped for verification only: all 20 car
entries match the original default selected-profile endpoints and separate
truncating interpolation. No captured palette is an implementation input.
The test session stopped afterward.

### Vehicle-selection callback audit and complete property-read coverage

`verify-vehicle-properties` now checks every typed output of the original
`1cdba..1d141` property-reading block and the entire 64 KiB DS for unintended
changes, not just drive bias and road threshold. 20,520 cases pass: all ten
supplied assets, four driver destinations, the earlier bias/threshold sweep,
and varying values across every property byte (zero divisors excluded by the
native safety guard). This includes signed surface/sound values, the unusual
surface-channel ordering, and byte 30 being discarded by an eight-bit shift
before byte 31 becomes the auxiliary accumulator. Raw resource retention is
also checked. File get-byte/close remain the only mocked calls in this block.

The enclosing `1ccfc` callback also caches the selected vehicle, loads four
base images, rotates twelve additional images, extracts wheels, remaps driver
colours and generates shadows. Its complete behavior is not proved by this
property test. Existing wheel tests cover rotation/extraction/remapping.

Selection lifecycle audit found far-call sites at 1baa7, 1d56b, 2498a, 2593b,
26154, 2632b and 263f1. Initial startup at 26154 passes suppress=0/choose=1;
new-game selection at 263f1 passes suppress=0/choose=-1. Therefore it must not
be replaced by unconditional rerandomization at each native race preparation.
The original seeds its RNG at 25b19..25b2b from the low word of the time()
result via srand(), before profile loading. Native production RNG lifecycle
and profile-to-vehicle/participation wiring remain pending.

### Composed native setup-session lifecycle

`src/game/setup_session.h` now owns profile selection, resolved race options,
four inventories/cash balances and one shared random state. Startup seeds the
stream from an explicit 16-bit platform seed and selects with choose=1. The
new-game operation resolves options, initializes inventory/cash and reselects
with choose=-1 only when signed DS:0090 is positive. This corrects the earlier
unqualified description of the 263f1 call: it is conditional. Saved-game and
next-race paths must not use the new-game reset operation.

`make verify-setup-session` passes 2,880 composed transitions: startup followed
by two new games, six modes, varied seeds/profile selections, and gate values
-32768/-1/0/1/32767. The original override and weighted-choice/RNG routines run
for real; only the asset/property callback is replaced with an argument/order
recorder. Original option resolution, the full inventory loop and actual
conditional reselection branch execute. Selected vehicles, participation,
colours, ordering/count, all options, inventories/cash, callback order and
post-transition RNG state match. The conditional main-function slice must use
its original CS=1987, not an arbitrary segment that covers the physical PC.

This session owner is not connected to production yet. Platform time seeding,
property application, active-player gating and transfer of inventory/vehicle
state into the race still need to replace the existing per-race defaults.

### Production vehicle-selection and RNG handoff

Normal no-argument startup now starts a native setup session from the low
16 bits of the platform wall-clock seconds. Amiga `DateStamp` days/minutes/
50 Hz ticks are converted from the SDK's 1978 epoch to a 1970 epoch; matching
PC/Amiga clock and timezone settings is not assumed. Generated original
vehicle weights and item flags are verified against the executable.

Startup chooses all random profile vehicles once. GO preserves the current
human vehicle-menu choice, performs the original new-game operation and
copies all four resolved vehicle choices into race preparation. Native asset
loading is deferred to GO, where every vehicle's assets/properties are loaded
before rendering. The setup RNG is handed to the race after initialization
and copied back on return to the title, replacing the fixed per-race seed in
normal play. Existing CLI diagnostics retain explicit deterministic fixtures;
new `SETUP` runs the production lifecycle with diagnostic seed 0x1234.

Integration caught an important caller-state distinction: DS:0090 is not a
static reselection setting to copy from initialized data. The one-track path
sets it to one at 2633c. Native GO similarly prepares one track and must enable
the conditional new-game selection. The initial live v1 test used the initial
zero and is superseded by v2. The generated resource no longer exports that
zero as a setting. Composed setup and generated profile-data regressions pass.

The session now computes inventories/cash, but they are not yet applied to
the live car/HUD state. Active-player/AI gating, profile-derived driving
coefficients, inventory transfer and the full menu remain unfinished. Normal
vehicle variety is no longer forced to the earlier four-car debug selection.

The corrected muted PAL A1200/2 MiB/no-Fast test `.run/setup-session-v2`
(port 24956) passed `SETUP_SESSION_OK VEHICLES=0,0,5,0 RNG=a7020e1f COUNT=4`.
It checks each actual race vehicle and the RNG handed to gameplay after the
four startup and three new-game AI choices. The emulator stopped afterward.
This is a startup handoff test, not a full-race or performance claim.

### Session inventory handed to live driving state

Normal race preparation now passes the session's 4x13 inventory into
`slicks_race_set_inventory` before starting the race. The authoritative HUD
inventory and each car's driving setup receive those same quantities, and
fuel initialization uses inventory slot 2 rather than the former implicit
zero. Legacy diagnostic initialization remains explicit when no setup
inventory is supplied. This does not implement weapon firing, shop purchases
or selection/consumption; those remain open.

The setter rejects unsupported upgrade interpolation inputs outside 0..20
atomically, and rejects mutation after race start. Exact-knot interpolation
now reads only its own knot, avoiding an out-of-bounds next-knot read at level
20 even though that next value would have been multiplied by zero. This is
an address-safety fix, not a change to valid interpolation arithmetic.

Native startup regressions cover all ten vehicles with legacy setup and all
21 inventory upgrade levels, verify fuel capacity and complete driving/HUD
inventory copies, and reject invalid/live mutations. The full existing DOS AI
and fuel suite passes, including 1,176 original fuel-capacity cases and 655,360
route-AI comparisons. Driving-physics and HUD/dirty-region regressions and the
Amiga build also pass. Profile-derived steering/AI settings and participation
gating remain incomplete.

Muted PAL A1200/2 MiB/no-Fast `.run/setup-inventory-v1` (port 24957) passed
`SETUP_SESSION_INVENTORY_OK ALL_52_SLOTS` and the unchanged vehicle/RNG handoff
check. Every live HUD/driving inventory slot and each car's fuel upgrade was
checked against the setup session. The emulator session stopped afterward.

### Per-surface steering and speed limits; moving SETUP regression

The native post-collision surface dispatch now refreshes steering and maximum
speed from the selected vehicle properties every update, matching original
231bd..238eb. This removes the captured per-driver steering constants. Surface
3/4, 5/6, 7/8, 11/12 and 18 preserve the original property groups, unsigned
byte reads, signed low-word product/division and tick/property gates. Startup
steering is 1000, matching the original initializer. This is only the limit
portion of the switch: surface velocity damping and oil-spin remain open.

`make verify-vehicle-properties` passes 32,768 original dispatch comparisons
for steering/speed outputs and 20,520 complete property-reader comparisons.
The dispatch fixture holds velocities at zero and damping at unity, so it
does not verify surface damping. Driving-physics regressions pass after giving
the held-brake synthetic fixture its missing vehicle maximum-speed property.

The first moving SETUP regression exposed a diagnostic command collision:
the broad first-character S test selected frozen SCANOUT mode for SETUP too.
Both initial attempts stopped with actual/reported race frame zero. The CLI
now checks the SCANOUT prefix explicitly. Earlier setup/inventory startup
handoff results remain startup-only evidence, not evidence of moving play.

After that correction, muted PAL A1200/2 MiB/no-Fast `.run/surface-limits-v1`
(port 24960) completed 200 actual updates with zero race/collision errors.
Vehicles were 0,0,5,0; the final surface was zero for all four, steering 1000,
and maximum speeds 100,100,80,100. All 64,000 displayed bitplane pixels match
the authoritative chunky buffer. The emulator stopped afterward. This proves
the moving setup regression and display consistency, not complete DOS
trajectory equivalence, full surface physics or a frame-time target.

### Per-tick surface velocity translation

`apply_surface_velocity` now implements the paired-component updates in the
original surface switch, after car collisions alongside its limit updates.
Surface 3/4 uses unsigned Q15 factor 0x7dd4 minus driver bias, 5/6 uses 0x7ee9,
7/8 uses 0x8118, 11/12 uses 0x7c31, and 18 uses 0x8000 with the original
collision-sound property gate. These factors come from the original 2aeb6
initializer. Each signed elapsed tick repeats the multiply; the low 32-bit
product and arithmetic right shift are preserved, including negative velocity
and overflow. Surface 7/8 can amplify velocity and must not be approximated
as generic friction.

`verify-vehicle-properties` now additionally executes 21,120 paired-component
comparisons against the original 231bd..238eb dispatch: all four drivers,
eleven selected surface values, five biases, six signed tick counts, eight
velocity pairs and both property-gate states. All pass. The fixture suppresses
oil entry RNG/spin to isolate velocity, so it is explicitly not proof of oil
spin fidelity. That spin and its entry/reset latch remain unimplemented.

Property/limit, driving-physics, surface-emission, dirty/HUD and complete
existing AI regression suites pass. The rebuilt muted PAL A1200/2 MiB/no-Fast
test `.run/surface-velocity-v1` (port 24961) completes 200 updates without race
or collision error; all 64,000 displayed pixels match the chunky surface.
The emulator stopped normally. This remains an integration/display check,
not proof of complete original trajectories or performance.

### Oil-spin heading and entry lifecycle

The surface-18 spin at 23613..236b4/2372c is now native. On entry it consumes
one original RNG step to choose -1/+1, then retains that direction while the
oil latch is set. Heading uses the original signed speed multiply/divide,
signed tick multiply and low-word addition with single-bound corrections
(including the original strict `>19200` upper test). Zero/negative ticks do
not suppress the entry RNG operation. The vehicle collision-sound property
still gates this entire case, as in the executable.

Layer selection resets the latch when selected DS:537c is not 18, before
effective-surface suppression by special driving state. New race startup
clears the native latch/sign. `verify-vehicle-properties` executes 6,144
original dispatch cases comparing heading, RNG, latch and direction, varying
signed ticks/speed, word-boundary headings, entry/reuse and both gates. All
pass. The existing 196,608 original layer-transition comparisons now also
check latch reset and unchanged direction, including special-state cases.

Property/limits/velocity, physics, surface effects, HUD/dirty and full existing
AI suites pass. Muted A1200/2 MiB/no-Fast `.run/oil-spin-v1` (port 24962)
completes 200 updates without race/collision errors, and all 64,000 displayed
pixels match chunky. That BASIC run is a general integration check, not a
claim of observed oil entry on the target; original-DOS oracle tests establish
the oil semantics. The emulator stopped afterward. Full driving trajectory
equivalence and the wider port goal remain open.

### Upgrade-derived steering coefficient

The steering multiplier is now taken from `drive_coefficients[6]` after
inventory interpolation instead of hard-coded 104. Original steering at
20ccb reads DS:6aee, the seventh word of the per-driver DS:6ae2 block. Default
upgrade level four still produces 104; other supported levels now produce
their original values rather than silently retaining the default.

`verify-vehicle-properties` additionally executes the complete original
2e032..2e0f7 initializer, including its table-copy helpers, across all 21^3
supported driving-upgrade combinations for all four drivers. All 37,044
seven-coefficient comparisons pass. Native race startup verifies the actual
steering handoff for all ten vehicles and levels 0..20 plus legacy setup.
The original steering arithmetic suite (576 cases), damage yaw (1,080),
driving and existing AI regressions pass; the Amiga build succeeds.

This fixes the inventory-derived steering input, not all profile settings.
Position integration still assumes profile setting 100 and must be connected
to DS:3f42 with original multiply/divide overflow behavior. Profile AI
coefficients and participant gating also remain open.

Muted PAL A1200/2 MiB/no-Fast `.run/upgrade-steering-v1` (port 24963)
passes the startup inventory/RNG checks and confirms all four live steering
fields equal their interpolated coefficient (104 for this default loadout).
The emulator stopped afterward. Non-default upgrade coverage is provided by
the host/original tests above, not claimed for this default target run.

### Profile-dependent position integration

Each live car now receives the selected profile's unsigned DS:3f42 setting
at session race preparation. Explicit legacy diagnostics initialize scale
100. Position updates use original 21230..212bc semantics: low-32-bit
velocity-times-scale, signed division by 2000, then modulo-32-bit position
addition. This replaces `velocity/20`, which only matched scale 100 without
multiply overflow. No fallback turns a legitimate scale zero into 100.
Inactive/negative profile participation remains a separate unfinished gate;
this change does not claim to implement it.

`verify-vehicle-properties` executes both original coordinate paths and their
real arithmetic helpers in 82,944 cases: all 256 byte scales, four drivers
using distinct profile indices, and signed velocity/position extremes.
Every result matches. The original composed force/movement/track-collision
test now covers scales 0,10,50,100,255 and passes all 14,400 cases. Existing
physics, damage, AI and dirty/HUD suites pass, and the Amiga build succeeds.
Direct synthetic motion fixtures now explicitly specify their original
normal-profile scale instead of depending on zero-initialized new fields.

Muted PAL A1200/2 MiB/no-Fast `.run/profile-position-v1` (port 24964)
passes 200 updates and checks the live settings against selected profiles
2,1,1,1 (all scale 100 for this loadout). No race/collision error occurs and
all 64,000 displayed pixels match chunky. The emulator stopped afterward.
Other scales are established by the original-instruction/composed tests,
not by this default-profile target run. Profile AI coefficients, participant
gating and whole-race trajectory equivalence remain open.

### Profile-derived steering input word

Tracing the remaining setup inputs found that original 1fcf5..1fd3d derives
the steering input from the same unsigned profile DS:3f42 setting used for
position scaling. Positive DS:4bc6 participation applies integer 7/5;
nonpositive participation retains the byte value. The destination BP-48 is
a word, so AI input can reach 357. Native preparation/startup now derive this
word instead of hard-coding byte values 100/140. The current native human/AI
routing still supplies the role; selected-profile participant gating remains
unfinished and is not claimed by this change.

All 65,536 scale/signed-role pairs match the original initializer. The
downstream original steering test now covers every input 0..357 and passes
103,104 arithmetic cases (including wrapped intermediate products). A native
production-handoff fixture checks all 256 scales in both human/AI modes.
Existing profile/property, physics, composed movement/collision, damage and
dirty/HUD tests pass; the A1200 build succeeds. This is not evidence that the
nine stored profile coefficients are AI parameters: their remaining uses
still need tracing rather than assigning meaning from their storage alone.

Muted PAL A1200/2 MiB/no-Fast `.run/profile-steering-v1` (port 24965)
passes 200 updates, validates the live steering-input word for each car,
and has zero race/collision errors. All 64,000 displayed pixels match chunky.
This default-profile run exercises scale 100, not the above-255 steering
cases covered by the independent instruction tests. The emulator stopped.

### Selected-profile participation reaches the race

Production setup now transfers DS:4bc6-style signed roles into the race:
zero inactive, negative human and positive AI. Legacy diagnostics without a
session retain their explicit four-car/optional-car-zero-human behavior.
The role is used in both motion/tail passes, human-versus-AI controls,
steering input, AI contact age, car-pair collisions, grid/layered drawing,
shadows, countdown clocks and per-driver HUD updates. The native completion
counter/results rows count active entrants instead of always requiring four.
Finished active drivers retain motion/collision behavior. Role changes after
race start are rejected; native GO rejects an empty participant set.

3,072 original branch comparisons verify all signed byte roles/four drivers
at the motion (202f3), tail (221af) and AI (2035f) gates. Native tests cover
all 81 inactive/human/AI combinations through the update passes, preserving
inactive car state. All 15 nonempty masks pass grid presence, countdown,
inactive HUD division avoidance and native finish-count checks. Collision
tests reject an inactive current car or partner while retaining the original
active unequal-weight response. These tests do not establish the complete
original results/menu flow or all actor/HUD call sequencing.

Existing AI, property, physics, collision and dirty/HUD suites pass; the
Amiga build succeeds. Per-driver input storage is present, but the platform
still maps only car zero's arrow keys: multiple-human configurable mappings
remain open. In SETUP, the actual selected human car zero now waits for input
instead of silently running AI; the diagnostic no longer means four AI cars.

Muted PAL A1200/2 MiB/no-Fast `.run/participants-v1` (port 24966) passes
200 updates and verifies actual roles -1,1,1,1 against the setup session,
along with each profile scale and role-derived steering word. Race/collision
errors remain zero. All 64,000 displayed pixels match the authoritative
chunky surface. The emulator stopped. Inactive-player target scenarios and
multi-human input mappings still need target-side coverage; the mask coverage
above is host integration, not a claim of a complete native setup/results UI.

### Original keyboard callback and two-human target test

`driver_input.h` translates original 1aa3c..1aabb. Each of the four five-key
binding groups targets DS:4bf2's human-first ordering entry, not the group
number's car. It preserves press/release handling, duplicate bindings and
aliased order entries across accelerate/brake/left/right/fire. Native masks
store the equivalent boolean latch state; fire is retained but firing remains
unimplemented. Session human controls now consume each driver's own mask.

The complete original callback passes 65,792 comparisons, substituting only
the raw keyboard-byte provider. Coverage includes all input bytes, varied
initial controls and bindings, all order permutations, alias entries, and the
actual original default keys. Physics, dirty/HUD and property suites pass.
The Amiga adapter maps every original default binding plus existing menu
keys. It is not yet a complete arbitrary-PC-scan-code mapping. Normal session
play now uses original bindings: first human Q/A/Z/X (accelerate/brake/left/
right), Control fire; second human keypad 7/4/0/Enter, keypad 1 fire. Legacy
non-session diagnostics keep their arrow-key override. CFG loading/rebinding
UI and joystick adapters remain separate open work.

`SETUPI` (debug.sh `SLICKS_SETUP_INPUT=1`) selects profiles 2,0,2,1 before
running the real setup lifecycle, then queues Q/keypad-7 presses through the
platform key-event path. No car state or framebuffer is injected. The muted
PAL A1200/2 MiB/no-Fast `.run/driver-input-v1` run (port 24967) passes at 200
updates with roles -1,0,-1,1, both human throttle inputs and both human cars
moving from their grid positions. Inactive car 1 has no drawn sprite, lap,
clock or HUD validity; no race/collision error occurs. All 64,000 displayed
pixels match chunky. The emulator stopped. This adds target evidence for
two humans and an inactive slot, not full multiplayer/menu/weapon completion.

### Original configuration and profile serialization

`slicks_save_configuration` implements the CFG byte stream from original
2b097..2b2ca: 142 bytes, including big-endian words, the explicit machine
signature and low-byte-only fields. The original save instructions, actual
word writer and BIOS-date signature calculation pass 256 complete output
comparisons. Only file open/byte output are substituted. All shorter output
capacities are rejected without writes. The existing 1,024 full-loader and
256 startup-default comparisons still pass.

`slicks_save_player_profiles` translates the PLR stream at 2b2d8..2b474.
The three built-in profiles are omitted; each user record has 21 raw name
bytes, nine big-endian statistics, a reserved zero byte, a setting byte,
nine signed coefficients encoded with +40 byte wrapping, vehicle/flags and
six RGB endpoint bytes. Counts outside 3..100 and insufficient capacity are
rejected before any output. The encoder does not normalize fields on save:
the reader's setting-below-10 correction still belongs to loading.

`make verify-player-profiles` passes 25,088 complete save-stream comparisons
against the original instructions and word writer (all 98 supported record
counts times 256 byte patterns). Every shorter capacity for every supported
count leaves the output untouched; invalid counts are also rejected. The
existing 784 loader and 768 built-in initializer comparisons still pass.
The Amiga target compiles and links successfully; no new FS-UAE behavioral
claim is made for these not-yet-connected save primitives.

These are serialization primitives, not completed native persistence or menu
UI. They perform no filesystem writes or dirty-flag changes. Production CFG
loading and import signature policy, native save lifecycle/error handling,
and profile editing remain open. Production already loads optional PLR files.

### Original player-menu selection rules

`profile_setup.h` now translates the complete 279b6 assignment and 28344
navigation helpers. Flag bit 1 allows a profile to occupy multiple slots.
Explicit assignment of a non-shareable positive profile replaces other
matching slots with the destination slot's previous profile, then assigns
the destination. Navigation skips occupied non-shareable profiles and returns
the initial selection at either list boundary; it does not wrap. The native
caller must supply valid profile indices, a driver in 0..3, and a navigation
step of -1 or +1.

98,304 original-code comparisons cover every four-slot selection over four
profiles, every combination of shareability flags, all destinations/choices,
and both navigation directions. Return/stack and all four selections match,
including duplicate input selections and endpoints. Existing 69,120 setup
state/call-order, 65,536 mode and 7,680 weighted-choice tests still pass.
These helpers are not yet connected to a native player-menu screen; the
screen, original event dispatch and edit/create/delete flows remain open.

`src/ui/player_menu.h` now implements the next layer, the original key
dispatch at 28999..28dab. It preserves eight-row clamped navigation,
left/right profile stepping, signed-byte car cycling, redraw/dirty/exit
flags, Escape/F9/F10 exits and Enter/Control/Space activation. Dialog requests
are explicit picker/add/edit/delete/help actions; capacity 100 disables Add
without changing the dirty flag. Car cycling intentionally does not set that
flag, matching DOS. The original unchecked car-key access on rows 4..7 would
index beyond the four driver slots; native code ignores it instead.

8,032 non-modal original-dispatch comparisons pass, including ignored keys,
all rows, list endpoints, prior dirty states and signed vehicle bytes.
96 additional tests execute the original path to the modal boundary,
checking request selection, dirty/exit flags and counts 3/99/100. Only the
keyboard-drain platform call is skipped in those tests. The complete earlier
profile suites still pass. Modal dialog contents, screen drawing and native
platform event wiring remain unfinished; this is not yet a user-visible menu.

### Profile creation/deletion lifecycle

The copy helper at 2821e revealed two unsaved in-memory fields missing from
the typed model: a tenth statistic word per profile and byte array DS:4b5e.
Both now have explicit storage and startup initialization. PLR serialization
still writes exactly nine words; it does not invent additional file fields.

`slicks_copy_player_profile` preserves the original strcpy semantics for
names and the nine-byte field currently named `coefficients`: it copies
through the first zero and preserves the destination tail. This is further
evidence against assuming those bytes are AI coefficients. All ten statistic
words, DS:4b5e, flags, vehicle and RGB endpoints are copied, but DS:3f42
(`setting`) is deliberately not copied by DOS. Native code rejects strings
without a terminator within their own record rather than overrunning fields.

`slicks_delete_player_profile` translates confirmed deletion 28c3d..28cbd:
decrement count, clear selections of the deleted profile, decrement higher
selections and shift subsequent records with the exact copy behavior. It
does not clear the unused final record or shift slot settings. Built-in
deletion is rejected at the native boundary; all strings to be shifted are
validated before mutation. 2,619 complete 64-KiB DS comparisons pass across
counts 4..100, first/middle/last deletions and varied string terminators/tails.
A malformed late source is rejected atomically, including selected slots.

`slicks_begin_new_profile` implements 27a6d..27b05: clear ten statistics,
consume six original RNG draws for RGB endpoints, clear flags and the first
byte of the nine-byte field, and set the profile scale to 100. Name editing
uses a local dialog buffer; this stage does not commit a name, increment
count, or reset the vehicle. 768 full-DS and RNG comparisons pass. The focused
`build/verify_player_profiles --lifecycle` mode runs these tests plus startup
defaults. The full loader/save suite and setup/palette regressions also pass.
These lifecycle helpers are not yet wired to the incomplete modal dialogs;
no new live Amiga UI or persistence completion is claimed.

### Profile editor acceptance and cancellation

`slicks_finish_profile_edit` implements 28145..28195. A positive signed
dialog result and nonempty local name commit the name through its first zero,
preserving the remaining destination bytes. Accepted new profiles additionally
set DS:4b5e to 1 and clear the first byte at DS:47da. Success returns 0;
cancel/nonpositive result or empty name returns 1. The routine does not
increment profile count (that is the caller's next step) or roll back property
changes already made during editing, including new-profile initialization.
Native malformed/unterminated names return -1 without mutations.

630 comparisons execute the original tail including its real strcpy, checking
the complete 64-KiB DS and return byte over name lengths 0..20, three profile
indices, signed result extremes and existing/new profiles. All pass, as do
the focused deletion, creation/RNG and startup-default lifecycle tests.
This is the editor's commit boundary, not completion of its property controls,
text-entry/colour dialogs, screen rendering or Amiga event integration.

### Profile editor controls and iteration limits

`src/ui/profile_editor.h` translates non-modal dispatch 27f5b..28120 and
iteration clamp 27b92..27bea. Six-row navigation clamps at the ends. Left/
right changes the setting with byte wrapping or changes the signed vehicle
byte subject to the original bounds. Activation toggles flag bit 0, cycles
vehicles, accepts, or returns an explicit name/first-colour/second-colour
dialog request. F2 accepts; Escape/F9/F10 cancel. Redraw values 1, 2 and 255
are preserved rather than reduced to a boolean.

The following iteration separately clamps the setting to at least 50 and
at most 100, or 150 when flag bit 0 or original global DS:01a6 is nonzero.
The global remains an explicit address-named input pending identification.
Clamping is not folded into event handling: that would change byte-wrap and
immediate accept/cancel semantics.

6,108 non-modal dispatch comparisons match full DS and editor state, covering
all scan bytes, all rows, ignored keys, byte extremes and vehicle-count
overflow. All 1,024 setting/flag/global clamp combinations also match full DS.
The focused lifecycle suite passes. Modal request branches are translated
from the original jump tables but dialog contents remain unimplemented;
this does not establish pixel-equivalent screen rendering or native UI wiring.

### Original text-entry state

`src/ui/text_entry.h` implements the character-state portions of 2f6da:
initial clear/preserve behavior (2f753..2f784), character handling and filters
(2f92d..2faa7), and the original runtime-font character lookup (2feaf).
Preserve mode appends after the existing text. Enter accepts; Escape clears
the local buffer; Backspace truncates before filtering. Flags control font
fallback case conversion, uppercase/lowercase, spaces and glyph rejection in
the original order. Profile names use flags 0x203: retained text, uppercase,
and font membership. Text input is a character stream, not menu scan codes.

`make verify-text-entry` passes 42 original initialization comparisons and
196,608 character/filter/buffer-state comparisons (all 256 input bytes,
64 filter combinations, four positions including full capacity, and three
font character tables). Tests execute the actual original glyph lookup and
compare unchanged buffer tails and cursor/processed-character state. Only
VGA saved-under restoration is stubbed, and Enter/Escape stop at cleanup.
An initialization comparison caught an incorrect first reading of the loop;
the corrected native cursor is exactly the retained string length.

This does not implement modal drawing, cursor blink, keyboard character
translation/extended-key consumption or Amiga UI integration. Caller-provided
buffers/fonts must satisfy the documented bounds; initialization rejects
unterminated retained text. No captured screen is used as an input asset.

### Original colour-picker state

`src/ui/colour_picker.h` implements local RGB initialization, key handling
2f5be..2f676 and commit/cancel 2f6ad..2f6d6. Up/down clamp the selected
channel to 0..2. Left/right subtract/add three with byte wrapping before the
original signed lower/upper test; normal values remain 0..63, but malformed
saved bytes are not silently normalized differently. Escape discards the
local RGB copy; Enter/Space commit it. This differs intentionally from direct
profile-property editing, whose cancellation does not undo earlier mutations.

`make verify-colour-picker` passes 196,608 original key/state comparisons
(all input bytes, channels and stored colour bytes) and 256 original
commit/cancel comparisons including every signed result byte. No rendering
or keyboard service is substituted in these slices. The animated selection
highlight, bars/preview, saved-under restoration and native dialog wiring
remain open. No target UI or complete colour-dialog claim is made yet.

### Colour-picker dynamic drawing loop

`slicks_colour_picker_draw` now translates 2f43f..2f5b6 into explicit
rectangle and palette-nearest calls: two rectangles per channel and the RGB
preview. Coordinates preserve original signed-byte values and word wrapping.
The selected channel's highlight queries the palette before advancing its
pulse; a changed BIOS tick advances phase by two (not elapsed ticks), and
the signed comparison resets phase above 60. The caller supplies a platform
tick snapshot and the palette-derived bar/unselected colours.

6,144 comparisons execute the complete original loop with drawing and
palette-nearest calls recorded at their boundaries. Every rectangle,
palette query, call order and final pulse/tick state matches, including all
phase bytes, three channels, unchanged/changed ticks and coordinate wrapping.
The key and commit/cancel suites still pass. These are command/state tests,
not pixel comparisons: callbacks substitute both drawing and palette lookup.
Dialog frame/text, platform rendering hookup and end-to-end display testing
remain open; do not treat this as a completed visible colour picker.

### Colour-picker chunky rendering and pixel oracle

`src/ui/chunky_ui.h` connects the dynamic picker commands to a 320x200 chunky
surface. The clipped half-open rectangle painter uses the shared `mult320`
table and reports written bounds through an optional dirty-region callback;
it does not scan a shadow buffer. The palette matcher preserves original
signed query bytes, unsigned palette entries, reserved entry zero, initial
distance 300 and first-entry tie behavior. The wrapper derives the three bar
colours and unselected colour from the supplied current palette.

The colour-picker test now creates a fresh x86 engine and executes the real
original bar-colour setup, palette matcher, clipped rectangle wrapper and VGA
fill code. Only actual VGA port/memory hardware is modeled. 256 complete
320x200 frame comparisons pass over varied palettes/initial backgrounds,
all ordinary channel values, selection/pulse phases and top/left/right/bottom
clipping. Command, key and commit/cancel tests still pass. These are host
pixel comparisons, not a live A1200 run; the enclosing dialog frame/text and
menu/modal platform integration remain incomplete.

### Original menu BMP resources

The player menu calls 2eb0a with `/players.bmp`, which resolves an archive
stream and decodes it through 2ec59. It does not use the title's `.@I`
decoder. `src/ui/menu_bitmap.h` now implements the 8-bit uncompressed/RLE8
path to chunky pixels and a 6-bit RGB palette. It honors the DIB header size,
pixel offset, bottom-up rows, encoded/literal runs and odd literal padding.
The DOS RLE delta behavior deliberately discards dx and restarts x at zero;
the native implementation preserves that instead of substituting generic
BMP semantics. Native supported dimensions are positive, at most 320x200,
with width divisible by four; malformed/unsupported files are rejected.

`make verify-menu-bitmap` runs the complete original 2ec59 loader with only
file and allocation services replaced. The real decode, memset and integer
helpers execute. All 64,000 pixels, 768 palette bytes and final file positions
match for all four archive BMPs: players.bmp (26,574 consumed bytes),
loading.bmp (11,900), end1.bmp and end2.bmp (65,078 each). All header/palette
truncated prefixes, sampled payload cuts and missing-final-byte cases are
rejected. Failed decoding may leave partial output; callers must discard it.
Resources are read directly from the original archive, not framebuffer dumps.
Production player-menu asset loading/rendering remains to be connected.

### Chunky rounded selection highlight

`slicks_ui_bevel` translates original 309cf..30b5a, the rounded highlight
called by the player menu at 28758. It preserves the three palette-nearest
queries (RGB +15, unchanged, -20 with byte wrapping), width/5 corner inset,
halving sequence, top/bottom drawing order and inclusive centre iteration.
All writes go through the clipped chunky rectangle painter and its optional
dirty-bounds callback, using `mult320`. Callers supply positive screen-sized
dimensions; it is not a new general-purpose UI style.

768 full 320x200 framebuffer and return-colour comparisons pass against the
complete DOS routine, original palette matching and VGA fill/clip code.
Coverage includes widths 1/2/5/65/167/320, heights 1/2/11/30, varied signed
colour requests, backgrounds and clipped placements. The initial oracle call
had an incorrect palette argument (DOS uses its global palette and takes a
page base); correcting the harness resolved that failure. Existing picker
pixel/state suites still pass. The player-menu screen itself remains unwired.

### Assembled player-menu redraw commands

`src/ui/player_menu_draw.h` translates the complete redraw stage
286ab..28981. It restores the two original background subrectangles, emits
the rounded selected-row highlight, skips inactive/nonpositive selections,
places computer icons and adjusts name positions, chooses vehicle sprites
or the two random-vehicle labels, then draws the four centered action labels.
Font identities remain DS:680/DS:688 resource slots until the platform loader
maps them; labels are passed as original resources, not recreated wording.
Invalid profile/sprite indices and unterminated names are rejected before
drawing rather than allowing the original unchecked resource reads.

512 complete command/text/order comparisons execute the original redraw,
recording its four rendering boundaries. Coverage includes all eight rows,
varied selected profiles and signed participation, real action labels,
concrete/random/unsupported vehicle values. Existing selection, keyboard,
modal dispatch and setup tests also pass. This is command-level evidence,
not an end-to-end screen/pixel claim. The original second restore retains
source y=105/height=150; its native renderer must handle visible clipping
and source bounds deliberately. Static preparation, resource/font mapping,
platform callbacks and menu lifecycle remain incomplete.

### Shared original font-resource decoding

The startup loads at 19dd8..19e7d and archive ordering resolve the menu font
slots: DS:0680 is `kirj.@f` (action labels), DS:0684 is `pieni.@f` (help),
and DS:0688 is `iso.@f` (player names). The latter two loads use sequential
archive resources after the named first load.

`src/ui/font_resource.h` implements original 2fc0c's runtime layout without
the HUD cache's 1,600 packed-pixel restriction. It copies the runtime header,
palette, codes and widths, then pads every glyph row to four bytes. Capacity
and complete input lengths are checked before any output writes. The existing
production HUD loader now uses this decoder instead of a separate padding
implementation; larger menu fonts do not enlarge the HUD cache.

`verify-font-resource` compares all output and untouched allocation bytes
against the complete original loader for kirj (5,726 runtime bytes), pieni
(2,454), and iso (4,244), with only file/allocation services replaced. It also
compares the production HUD loader's full runtime buffer directly against
that original result. Every truncated input prefix and insufficient output
capacity is tested. This establishes font loading, not a running player-menu
screen: static preparation and production menu integration remain open.

### Original menu tint and background remap

`src/ui/palette_remap.h` translates the 344c5 palette-table builder and
34599 remap painter used by player-menu static preparation. The menu requests
RGB (15,15,45), percentage 50, then remaps two header rectangles, four player
rows and the action-label area. The table builder preserves signed-byte
percentage conversion, truncating division by 25, narrowed weight, 16-bit
blend overflow, arithmetic shift and the original nearest-colour search.
It does not substitute modern/clamped colour blending.

`make verify-palette-remap` executes the entire original table routine and
nearest-colour search without stubs. 288 complete 256-entry table comparisons
cover every percentage byte, the actual menu colour/percentage, uniform and
varied palettes, signed colour extremes, and untouched output guards. The
native remap painter is compared against the complete original 34599 code
with only VGA plane selection/memory modeled: 112 full-frame comparisons
include every static-menu rectangle, all x alignments, full-screen and
single-pixel writes, and empty rectangles. Exact dirty bounds are checked.

The native painter uses the shared `mult320` and authoritative chunky surface;
out-of-surface rectangles are rejected rather than reproducing raw wrapped
VGA access. These are static-preparation building blocks, not yet a connected
player-menu screen. Resource callbacks, save-under and the
production menu lifecycle still need integration and end-to-end verification.

### Assembled player-menu static preparation

`src/ui/player_menu_prepare.h` now composes 283c3..285ec's painting order:
header remaps, iso/kirj font colours, title command, footer tint, pieni font
colour/help command, four player-row remaps, and action-area remap. This runs
after decoding players.bmp and loading the three runtime fonts; the caller
still owns resource loading, saved-background storage, fading and display.
The footer call at 34654 is a tint-table/remap wrapper, not a separate frame
or border primitive. Its percentage comes from DS:16fd (initial value 66).

The preparation comparison executes the original stage with its real tint,
nearest-colour, font-colour, memory-copy and VGA remap routines. Only the
computer-icon resource load and two text-renderer boundaries are intercepted,
plus modeled VGA plane selection/memory. It compares the complete background,
font bytes and exact text commands. All 24 varied-background/palette/percentage
cases pass, alongside the 288 tint-table and 112 remap-frame cases. It
deliberately does not claim text
rasterization or end-to-end menu display coverage.

The first comparison exposed a wrong interpretation of 36227: it is a
replacement-string lookup wrapper, not uppercase conversion. With no lookup
table it returns the original `players` string unchanged. The native caller
must pass the resolved resource string without changing its case.

### Visible menu saved-background restoration

`src/ui/menu_background.h` implements the visible-page result of original
3b9de for a saved 320x200 chunky screen. Source x is rounded down to four
pixels and width rounded up independently; the page origin is added to the
source crop coordinates. Thus the first player-menu restore covers x=40..231,
and the second covers x=32..131, not x=35..134. Both use the prepared menu
background, not a DOS framebuffer capture.

The original second request has source y=105 and height=150, so it reads and
writes past visible row 199. The native routine clips those invisible rows
and never accesses beyond its 64,000-byte snapshot. `verify-menu-restore`
executes the entire original copy with VGA writes modeled, comparing all
visible pixels and exact dirty coverage. 116 cases pass across all four
destination-plane phases, varied backgrounds, odd crop widths, full-screen,
edge and both real menu rectangles. Rows outside the display are deliberately
not part of the equivalence claim. The native routine reports its clipped
written rectangle directly and uses `mult320` for both buffer addresses.

This resolves the previously open bounds/rounding question in the redraw
callback. Connecting that callback and the rest of the menu renderer to the
production UI remains open.

### Menu-font coverage of the native text renderer

`verify-font-glyph` now runs the actual 68020 glyph, measurement and complete
string routines against the corresponding original DOS code with all three
fonts, loaded through the shared resource decoder. Buffers accommodate the
larger fonts instead of the previous pieni-only limits. Per-font results:

- pieni: 1,236 complete-frame glyph cases, 4,240 measurement cases, 520 strings.
- kirj: 1,764 complete-frame glyph cases, 4,240 measurement cases, 512 strings.
- iso: 792 complete-frame glyph cases, 4,240 measurement cases, 512 strings.

Coverage includes every glyph, palette substitutions, screen-edge placement,
all character bytes for measurement, spacing, alignment, highlighting,
tabs/newlines, menu-like strings and preserved 68020 registers. This verifies
the six- and nine-pixel-high menu fonts as well as the five-pixel HUD font.
The exact arrow-key/help footer is also tested with pieni at its real (1,193)
position; larger fonts deliberately do not render that footer off-screen.

### Full static-menu pixels with original assets and native text

The palette/preparation verifier now additionally decodes actual players.bmp
and all three fonts, runs native static preparation with the real 68020 string
renderer, and compares all 64,000 pixels against original 283c3..285ec including
the actual DOS font rasterizer. Four footer percentages include the original
66 plus 0/50/100. No captured framebuffer supplies native pixels.

This combined test caught reversed read/write plane selectors in the earlier
VGA model. Remaps set both planes identically and therefore could not expose
that mistake; the footer's text did. Correcting the harness made the complete
images match without changing native text code. The full-image test now runs
the actual plane-select I/O instructions and models their ports directly.
Only the unused computer-icon asset-load call is intercepted during this
static stage. Dynamic rows/icons, menu input and live Amiga integration still
remain outside this static-screen verification.

### Original computer and car-selection icons

`src/ui/menu_icon.h` decodes the original tB1/`@16` menu resources, distinct
from the HUD's indexed/RLE image format. It preserves the high transparency
bit and original RGB expansion (five-bit components doubled to 0..62), then
uses the original nearest-palette lookup. Transparent pixels become zero;
opaque pixels retain a nonzero palette index for the existing 68020 chunky
transparent blitter. Decode once at first use with the current palette,
matching DOS's B3-to-B2 sprite cache; do not reconvert after a palette change.

`make verify-menu-icon` runs the complete original 2e0f8 loader and 2e2d2
first/cached drawing paths. Only file/allocation services and VGA hardware are
modeled; original palette lookup, cache creation, copy and transparent
blitting execute. The native decoder plus actual 68020 transparent blitter
matches all 64,000 screen pixels in 90 cases across computer.@16 and
auto01.@16..auto09.@16, three palettes, three placements, and repeated draws
after changing the palette. Every truncated input prefix is rejected without
writing output. Native geometry is limited to byte-sized on-screen icons,
with capacity/header validation before decoding. The new decoder is ready
for the player-menu resource callbacks, which remain to be wired.

### Composed player-menu renderer

`src/ui/player_menu_renderer.h` now supplies one renderer context for the
prepared chunky surface, native saved background, decoded fonts/icons and
low-level native drawing adapters. It connects the verified restore, bevel,
static preparation and redraw operations, with explicit font mapping:
preparation slots kirj/pieni/iso become redraw slots kirj/iso. Sprite -1
maps to the computer icon, followed by vehicle indices. Text/icon adapters
are responsible for reporting their painted bounds; restore/bevel painters
report their bounds directly.

The four full-image static-preparation comparisons now execute through this
context with the actual 68020 text renderer, and also verify that its saved
background exactly equals the prepared native image. The new dynamic adapter
composition still needs combined pixel coverage; existing component and
original redraw-command comparisons are not treated as that evidence. This
context is not yet installed into the production Amiga menu event loop.

### Complete composed player-menu redraw pixel comparisons

The composed renderer now has 96 sequential full-screen comparisons against
the complete original 286ab..28981 redraw, covering all eight selected rows,
inactive profiles, human/computer participation, every car-selection icon,
both random-car labels and unsupported vehicle selections. Original action
labels come from DS resources; names are controlled test profiles. Each draw
keeps the previous frame rather than resetting the screen, exercising the
two saved-background restores as selection and profile contents change.

Native text and icons execute the actual 68020 routines. DOS executes its
real text, bevel, restore and icon drawing code, including VGA port writes.
DOS icon inputs are original 15-bit resource pixels, not native-decoded
palette indices. The shared prepared background has already been independently
pixel-verified; it is encoded into DOS's save-under representation for this
redraw comparison. All 64,000 visible pixels match after every draw. The VGA
model now handles multi-plane/word writes and discards invisible restore
rows, whose visible equivalence was separately established.

This closes the dynamic renderer-composition verification gap noted above.
It does not yet verify a running Amiga menu, keyboard/modal lifecycle, or
dirty reporting by production text/icon adapters; those integration items
remain open.

### Amiga player-menu resource/rendering adapter

`src/platform/amiga/amiga_player_menu.c` now owns target-side menu resource
loading and the composed renderer. It reads players.bmp, all three fonts and
the ten original icons from SLICKS.000 while AmigaOS is available, releases
its temporary input buffer, and retains decoded data plus the prepared native
saved background. Labels/title/footer remain caller-supplied original DS
resources, not newly invented strings. Cleanup releases the adapter allocation.

Text and icons call native assembly; `sui_menu_bridge.s` provides C ABI text
measurement and flags-aware drawing entry points with the original startup
text settings. The complete 96-redraw and four-static-screen comparisons now
call the actual text C ABI bridge and verify preserved registers. Both adapter
and bridge cross-compile with target warnings treated as errors. Existing
multi-statement clipping checks were split to satisfy the target compiler's
misleading-indentation warning without changing semantics.

The adapter records painted row intervals in a bounded 16-entry list, merging
overlap/adjacency and conservatively collapsing at capacity. There is no
per-row boolean map or framebuffer comparison. Production input/display-loop
wiring, generated menu-string export, target dirty-interval testing and a live
A1200 run remain open; merely compiling these objects does not make the menu
reachable from the running program. The existing executable was not replaced.

### PLAYERS event-loop integration and vehicle-icon correction

The main program now opens the original player screen from the title's
PLAYERS activation, temporarily restoring AmigaOS for archive I/O and then
resuming hardware takeover with the menu palette. Navigation, left/right
profile stepping, A/C vehicle cycling and Escape/Exit run through the verified
original key logic. Renderers paint chunky data and only their merged dirty
row intervals are converted. Returning to the title restores its palette
and frees the menu resources. Add/edit/delete, profile picker and help actions
remain explicitly pending; the program does not claim those modals completed.

Menu strings are exported from original DS offsets into ignored
`src/gen/setup_defaults.h`, including the initial footer tint percentage.
The `PLAYERS` diagnostic uses the same opening/input/rendering paths on a
separate `out/SlicksPlayerMenu` executable, leaving existing debugger ELFs
unchanged. Debug audio remains host-muted without changing emulated DMA.

Integration tracing found a substantive icon-list error in the earlier
nine-car test fixture: 2a03b loads `/carimage16` into vehicle slot zero, then
2a05e loads subsequent archive resources sequentially. `carimage16` is a
104-byte image, not an empty marker. The adapter and tests now include all ten
vehicle images (`carimage16`, auto01..auto09) plus computer.@16, with vehicle
count 10 and random sentinels 10/11. Icon first/cached-draw coverage rises to
99 cases; the composed redraw fixture now uses 104 cases. Earlier nine-car
fixture results were valid only for that restricted fixture, not the full
original resource mapping.

The first live A1200 menu rendered successfully and its complete 64,000-pixel
chunky/bitplane comparison passed. Debugger-side queue writes did not deliver
the planned input sequence (head/tail remained zero); the bounded diagnostic
now injects ordinary key events on the target, following the existing CONFIG
test convention. The corrected run completed six draws (initial, Down, C,
Right, Left, Up), returning to row zero with no adapter error or pending
modal action. Both the initial and edited frames pass all 64,000 pixels of
chunky-to-interleaved-bitplane comparison on A1200, 2 MiB chip, zero fast RAM.
The UI still needs modal dialogs, close/reopen/race-handoff regression and
remaining original setup screens before native menu-driven setup is complete.

### Player-menu close/reopen and race handoff

`PLAYERSR` extends the ordinary target-side key sequence with Escape, Return,
Escape, Up, Return: close the edited player menu, reopen it, close it again,
select GO and enter the race. `amiga/diag_player_handoff.gdb` checks both
resource releases, active display restoration, the retained edited vehicle,
and all four drivers' setup participation/vehicle values at race entry.

The first run exposed a diagnostic-mode collision: the old P-prefix particle
disable switch also matched PLAYERS/PLAYERSR. The race reached frame 700
without runtime/collision errors, but particles were disabled and the test
expected frame 200. Player-menu modes now bypass that switch; the regression
explicitly rejects disabled particles or an altered checkpoint at race entry.

The corrected `out/SlicksPlayerHandoffFixed` run passed on A1200 with 2 MiB
chip RAM and zero fast RAM, host audio muted but emulated audio unchanged:
`PLAYER_HANDOFF_OK TWO_CLOSES_REOPEN_SETUP_PRESERVED_RACE_200`.
Reopened-menu, restored-title and race-200 chunky/bitplane dumps each match
all 64,000 pixels. The title copperlist is byte-identical before and after
the two menu visits. Dumps remain ignored verification artifacts, never game
assets. The default run.sh executable was not replaced by this diagnostic
build. Modal profile picker/add/edit/delete/help and the remaining original
setup screens are still open; this gate proves only the exercised handoff.

### Shared profile-list dialog input and return semantics

The player picker wrapper at 281d1 calls the shared dialog at 30cc4 with
21-byte profile names, the original SELECT caption, current selection and
cancel-restores-selection flag. Edit/Delete call the same dialog with other
action captions and flags; they must not be replaced with a new picker UI.

`src/ui/list_dialog.h` translates the actual 315b6..316cf key dispatch,
3115d..311b5 next-update clipping/scrolling and 31760..31773 return encoding.
It preserves separate list/action focus, Tab action cycling, Up/Down,
Home/End, PageUp/PageDown, Return/Space/F10 acceptance, Escape/Backspace
cancellation, and the redraw/pulse state touched by those instructions.
Page steps remain unclipped until the next update, as in DOS. The result
contains the selected index plus the action shifted into bits 12..15.

`make verify-list-dialog` passes 61,440 comparisons running those original
machine-code regions, across every scan byte, counts 0/1/3/10/100, visible
row counts 1/7/11, both focus modes, action counts 0..3 and both cancel flags.
The oracle verifies its stopping addresses and all represented state fields,
then compares the encoded return value. This is a component gate only:
dialog initialization, rendering, save-under/font restoration and production
modal lifecycle still need translation/integration before the picker works.

### List-dialog initialization and row drawing

`slicks_list_dialog_init` now covers negative initial selections (action
selection), caption-supplied action overrides, cancel-return selection,
visible row count, centered/clamped initial scroll position, scrollbar-thumb
height and initial redraw/pulse flags. It preserves the original low-word
multiply before signed division in thumb sizing. Invalid native geometry is
rejected without altering output state. The caption parser is still separate
pending work, not implicitly replaced by caller-invented strings.

The initialization oracle executes 30cca..30ce7, 30f39..30fa9 and
3113c..31155: 5,760 cases match every represented state field and the thumb
size when DOS initializes it. Counts 0/1/3/10/100, font heights 5/6/9,
dialog heights 50/100/180, negative/ordinary selections, action counts and
caption overrides are covered. The existing 61,440 input/result cases pass.

`src/ui/list_dialog_draw.h` translates 3125b..313dc's list-row pass. It
preserves incremental redraw of the two affected rows, saved tinted-dialog
background restores, the three ordered selection-highlight fills and text
positions/profile indices. All 540 command-sequence cases match original
DOS instructions with only drawing leaf calls intercepted. This does not
yet establish pixel equivalence: caption rendering, scrollbar drawing,
selection pulse, dialog save-under and production adapters remain open.

### List scrollbar and selection-pulse semantics

The row command oracle now runs through 315a7, covering the original
scrollbar restore, five ordered thumb fills, signed low-word positioning
arithmetic, no-scroll case and redraw-flag clearing. All 540 composed
row/scrollbar cases pass, alongside 5,760 initialization cases.

`slicks_list_dialog_pulse` translates 311b5..31223. Its palette query occurs
before updating the pulse. A changed 32-bit DOS clock tick advances exactly
once, even after a multi-tick gap; crossing 0/50 reverses the signed step
without clamping the value. The 1,300 original-code cases cover both step
directions, values -7..57, unchanged/changed clocks, word-boundary crossing
and 32-bit wrap. They compare RGB query order/bytes, returned palette index,
tick and pulse state. Palette lookup is an intercepted leaf here, already
covered independently; complete displayed pulse pixels remain unverified.

The adjacent pulse-entry/scroll-stop regions require a fresh Unicorn engine
before the key/scroll suite so previously translated blocks cannot cross the
requested stop. Every oracle checks its stopping address. The full verifier
passes, including all 61,440 key/scroll/result cases. Caption parsing/drawing,
active-text repaint, tinted save-under and live modal integration remain open.

### Original comma-separated action captions

`src/ui/list_captions.h` translates the complete 30b5b caption helper. It
splits comma-separated action labels, skips a leading hyphen, either measures
maximum width or draws right-aligned labels, and restores the font colour
after each draw. Negative signed selection bytes choose measurement-only
mode. Its packed count/width return preserves the original addition. A
bounded local label replaces temporary writes into DOS resource strings;
unterminated inputs are rejected before any drawing or font-state change.

`make verify-list-captions` runs the complete original routine in 6,144
cases with only font measurement/drawing/colour and palette lookup leaves
intercepted. It compares every command and string, return value, final font
colour and restored input bytes for all selection bytes, three font heights,
actual SELECT and MODIFY/REMOVE/CANCEL labels, empty labels, repeated commas
and leading hyphens. This proves composition, not new font-pixel coverage.
The existing list initialization, row/scrollbar, pulse and key suites also
pass. Caller-side caption metadata interpretation, active-text repaint,
save-under/restore and live modal integration remain outstanding.

### Active-text repaint and caption metadata

The list draw oracle now executes the original branch at 31252 through
315a7. On redraw it restores rows and paints the scrollbar; otherwise only
list focus repaints the selected text using the pulse colour and restores
the font colour afterward. All 900 complete command/state cases pass. The
native caller must retain that branch order: clearing redraw in the row pass
must not accidentally trigger active-text repaint on the same update.

`slicks_list_caption_layout` translates 30d28..30dc1 with the actual caption
helper executed by the oracle. All 200 metadata cases pass, including empty
captions, signed leading control bytes and hyphens. The original hyphen scan
assigns the total action count, not the marked action's index; subsequent
initialization resets that out-of-range value. This oddity is preserved.
Signed high-byte action values are now also accepted by the native init
boundary, with expanded 10,080 initialization comparisons. The 6,144 caption
helper, 1,300 pulse and 61,440 input/result comparisons remain passing.

These helpers are still not the running modal dialog. Original preparation,
saved/tinted background ownership, rendering adapters, exit restoration and
live input dispatch must be composed and pixel/target verified next.

### Compact native dialog save-under

`src/ui/saved_rectangle.h` now provides caller-owned compact chunky capture
and restore for modal dialogs. It translates the visible semantics of 3abe5,
3aaf2 and 3b9de: capture width rounds up to four pixels while retaining the
unaligned source X; crop source X rounds down to four and crop width rounds
up independently. The extra saved pixels are retained, not silently dropped.
Destinations use shared mult320 offsets, and restorers report actual painted
bounds without comparing framebuffers. This is explicit modal save-under,
not a full-screen shadow used for dirty detection.

`make verify-saved-rectangle` passes 192 cases executing all three original
VGA routines. It compares every captured pixel (including rounded padding),
full displayed pixels after full/cropped restore, and dirty-bound coverage,
across six geometries and all source/destination X phases. Undersized buffers
and out-of-screen/source accesses reject without writes. There are no new
runtime allocations in this helper; the future dialog owner must allocate
its original-screen and tinted-background snapshots and release them on exit.
The helper is not yet connected to a running dialog or claimed as completed
picker integration.

### Composed list renderer (not yet live)

`src/ui/list_renderer.h` now composes the translated helpers into modal
preparation, per-update rendering and exit restoration. Preparation saves the
caption background, tints/draws its borders and labels, selects list font
colours, saves the original main rectangle, tints it and its scrollbar track,
then saves the tinted redraw rectangle. Exit restores main then caption and
the old font colour, respecting the original keep-main-screen flag. Buffers
and resource lifetimes are caller-owned; all sizes and text termination are
checked before painting. It reuses mult320 addressing and painter-reported
dirty bounds through the existing primitives.

`make verify-list-renderer` passes 16 synthetic-font composition cycles across
four player-row positions, two profile counts and picker/edit action labels.
Paging, focus changes, pulse/unchanged-tick repaint and cancellation restore
all 64,000 original screen pixels and font state. Insufficient storage does
not paint. The same test passes AddressSanitizer/UndefinedBehaviorSanitizer.
All original-code caption/list/snapshot component suites remain passing.

This is not yet a complete DOS-composed pixel oracle or Amiga integration.
The synthetic font is deliberately a test double, never a production font.
The renderer initializes its previous pulse tick to zero; DOS leaves that
stack local unspecified before first comparison, so first-tick phase is not
claimed identical. The full preparation command/pixel comparison and actual
Amiga dialog allocation/input/close wiring are the next gates.

### Full composed dialog pixel oracle

`make verify-list-pixels` now executes original 30cc4 preparation, complete
update rendering, keyboard dispatch and exit restoration against the native
composed renderer. Only DOS allocation/free and keyboard draining are
substituted; original palette, caption, font, tint, save-under, rectangle and
VGA routines execute. Native text measurement and drawing execute the actual
68020 C ABI bridges with the decoded kirj.@f resource, not a synthetic font.
The starting picture/palette is decoded players.bmp; neither side uses a
captured screen as an asset.

All 52 complete 64,000-pixel/font-colour comparisons pass across SELECT and
MODIFY/REMOVE/CANCEL captions, 3/100 profiles and four vertical positions.
The sequence covers preparation, initial paint, Down/PageDown/End/Up/PageUp/
Home, action/list focus changes, active pulse repaint and exit. Original and
native return values agree and both DOS snapshot allocations are freed.
Oracle stop/return addresses are checked. The initial mismatch proved to be
missing full-screen clipping bounds in the new DOS fixture; after setting
the established VGA clipping state, no production renderer correction was
needed. Previous-tick locals are explicitly zeroed on the DOS side to match
the native defined initialization rather than comparing unspecified stack
contents. This closes the composed pixel gate for the exercised sequences,
not live A1200 picker integration, which remains the next step.

### Live original profile picker on A1200

Activating one of the four player rows now opens the verified original list
dialog. The adapter uses the original kirj font, profile names and SELECT
caption exported from DS:1343, and allocates compact original/tinted/caption
buffers outside hardware takeover. Every painter feeds the existing merged
dirty-row C2P path. Acceptance/cancellation returns through original 279b6
profile assignment and setup refresh, restoring menu pixels and freeing the
dialog allocation. Per-frame pulse uses the established PAL-to-BIOS clock
accumulator. Backspace/Tab and keypad PageUp/PageDown mappings are added.
Add/edit/delete/help remain explicitly pending.

GCC tried to tail-branch across sections from the measurement callback to
assembly; elf2hunk rejects that PC32 relocation. A result-register compiler
barrier retains a regular call/return, and the final target build succeeds.

The `PLAYERSK` diagnostic injects ordinary keys, not selected-profile state:
open (profile 2), Up (1), accept, reopen (1), Down (2), cancel (back to 1).
`amiga/diag_profile_picker.gdb` passes on 2 MiB chip/zero-fast A1200 using
`out/SlicksProfilePicker2`, port 24973, host audio muted. It observes four
picker draws, both allocation releases and the retained accepted setup:
`PROFILE_PICKER_OK ACCEPT_REOPEN_CANCEL_PRESERVED`.
Open, changed and restored-menu frames each match all 64,000 actual bitplane
pixels against the authoritative chunky buffer. The first attempt stopped
on an uninitialized GDB convenience variable; the final fixture initializes
it and also requires the accepted selection to differ from the initial one.
This diagnostic does not replace the default run.sh executable. Picker-to-
race handoff, further modal integration and broader interactive regression
remain required before claiming complete native setup.

### Picker selection carried into a live race

`PLAYERSG` extends the ordinary picker accept/reopen/cancel key sequence with
Escape, Up and Return to close the menu and activate GO. The target injects
only input events. `amiga/diag_picker_race.gdb` confirms the accepted profile
differs from the initial profile, survives cancellation/reopening, and reaches
race setup after both picker and menu resources have been released. All four
drivers' participation and active vehicles match the setup session.

The muted A1200 2 MiB chip/zero-fast run at port 24974 using
`out/SlicksPickerRace` passed: `PICKER_RACE_ENTERED selected=1`, then
`PICKER_RACE_OK PROFILE_PARTICIPATION_VEHICLES_FRAME200`. Particles remained
enabled and no runtime/collision error was reported. The frame-200 chunky
surface matches all 64,000 displayed bitplane pixels. This closes the tested
picker-to-race handoff, not Add/Edit/Delete, help or the rest of native setup.
The normal run.sh executable has not been overwritten by this diagnostic.

### Edit/Delete result routing and original confirmation painter

`profile_actions.h` translates 28b00..28b48's list-result dispatch and
28c29..28c3d's confirmation keys. `make verify-profile-actions` passes
131,072 complete-word comparisons across every result/key value with the
edit-protection flag clear/set. Edit requires result >1 and <0x1000 and
flag bit 2 clear; deletion confirmation uses >0x1000 and <0x1fa4 and masks
the index to 12 bits. Cancel/other action encodings do nothing. Only the
original Y/K/J scan codes confirm, not Return or Space. The native caller
must still validate the decoded index against available profile storage.

`profile_delete_prompt.h` translates the original 28b48..28c21 painter:
grey tint over (90,80)..(230,110), original palette-selected text colour,
centered profile name at (160,84) and resource confirmation text at (160,94).
The old font colour is retained for both confirmation and cancellation exits.
The full DOS/native-68020 pixel suite now includes this painter and original
font restoration, increasing to 60 full-screen/font-state comparisons; all
pass. The existing player-menu snapshot, not another invented dialog
background, supplies restoration after this prompt.

Integration issue found: the original deletion branch has no built-in-record
or edit-protection check, whereas `slicks_delete_player_profile` currently
rejects indices below 3 and downstream helpers assume at least three records.
Do not connect Delete while silently presenting that native restriction as
original behavior. Reconcile the lifecycle/count assumptions and extend its
oracle coverage before wiring the shared Edit/Delete list and confirmation.
No actual profiles were deleted in these component tests.

### Deletion-created low profile counts

The native delete helper now permits slots 1/2 as the original dispatch does,
while retaining slot-zero/index/storage guards and atomic malformed-string
rejection. Expanded original deletion coverage passes 4,419 full-data-segment
comparisons, including count 2 deleting slot 1 to leave count 1. Record-copy
tails and per-slot settings retain the original behavior.

The original override selector can choose allocated slots 1/2 *after* its
logical-count check. Player-menu drawing and picker entry/return now validate
those indices against the 100 allocated slots, without inventing another
logical-count clamp. Setup comparisons now cover counts 1..4 and pass
276,480 four-driver state/call-order cases. Menu command comparisons pass
with counts 1..7. The complete dialog pixel suite now covers counts 1/2/3/100
and passes 120 whole-screen/font-state comparisons, including restoration
and delete-prompt painting.

PLR serialization has been extended to represent counts 1/2 with wrapped
16-bit extra-record words -2/-1, matching the original word addition to
three and absence of appended records. The full verifier passes 800 original
loader comparisons and 25,600 complete original save-byte-stream comparisons,
including all short-capacity rejection checks, plus the existing profile
editor/new-record gates. `out/SlicksProfileCounts` cross-builds successfully.
Live Delete remains unconnected, and no user profile files have been changed.
