# Resident menu cache verification

Implementation/evidence, 2026-09-29. Open integration work is maintained only
in `open-work.md`. This is not a claim that every menu is disk-free yet.

## Ownership and storage

Startup retains the 57 additional resources in `menu_resources.h`; decoded
title artwork and its palette already have permanent owners. Private lossless
run-pair storage is selected only when smaller than the original encoded bytes.
The target cache occupies **130,098 bytes**, including metadata, before Exec
allocation rounding. Original data is read at runtime, not committed or replaced
with captured frames. A streaming constructor avoids a 64K temporary buffer.

Cached archive handles are explicit borrowed providers: no disk handle,
directory allocation or silent miss fallback. Their owner lives until cleanup.
Normal startup releases the redundant 64,003-byte title staging buffer;
bitmap-audit diagnostics retain it. Keyboard layout is snapshotted once before
takeover; menu constructors only copy the immutable mapping.

## Native integration

Title Help, Options, Controllers, Players and the main Tracks selector use the
cache. RAM-only open/close and player name/colour/picker transitions retain
hardware ownership. Pause Help/Controllers/speed and race resume also retain it.
Palette list replacement waits for display blanking; switching views does not
restore AmigaOS. Actual file operations still have explicit boundaries.
Shop and registration Help also use cached resources and retain takeover for
their Help navigation. Shop screenshot saving and registration exit-image
loading remain explicit disk operations.
Intermission resources and track-preview fonts/icons use the cache; selected
track and SLICKS.DAT loading remains an allowed disk operation.

The larger resident set exposed preview memory pressure. File buffers now use
their actual bounded lengths (still rejecting empty/oversized files); icon
scratch is 512 bytes instead of 32K, versus a largest original icon of 338 bytes.
The 64K preview arena is reserved before smaller file buffers. Before that
ordering correction, a real failed preview had 81,544 bytes free but a largest
block of only 55,200. The corrected preview passes open, animation, close,
reopen and race entry. Rendering operations/order are unchanged.

A temporary 512-byte automatic language buffer overflowed the default stack on
the intermission path and led to a later illegal instruction. It is now static
staging owned by the synchronous menu operation. The corrected native run
confirms a 4,096-byte process stack, nine intermission edit inputs, next-race
progression, zero race error and restoration 31. No larger stack is requested.

## Checks

### Track-list chooser RAM-only navigation

Removed the owner-wide end/begin pair around every `track_lists_key` call.
Picker movement, name typing/cancellation, delete confirmation/cancellation
and selection from the in-memory catalogue retain hardware ownership. The
actual transactional save/delete owner now releases hardware immediately
before storage, and the caller republishes/reacquires only when necessary.
Opening the catalogue still reads SLICKS.TRK; startup catalogue caching remains
open. This is not a claim that entering the chooser is disk-free yet.

`diag_track_lists_resident.gdb` supplies shared guards for selection/save and
delete tests: no platform teardown or archive opening while navigating;
catalogue disk loads require released hardware; save/delete must enter their
boundary with active hardware and call storage only after release. Native
`SLICKS_TRACK_MENU=2` passes name entry/save/race with two name frames and one
disk commit. Values 3 and 4 then pass saved-list load/race with zero commits,
and name cancellation/delete cancellation/confirmed deletion/empty reopen
with exactly one commit. These use the same isolated
`FSUAE_RUN=.run/track-lists-resident`, muted warp mode, and respectively
`diag_track_lists.gdb` and `diag_track_lists_delete.gdb`; all runners exit and
close their emulators at the race checkpoint. Local logs are
`tmp/track-lists-resident-{save,load,delete}.log`.

`make verify-track-lists verify-track-list-storage` also passes 60 original
reader and 80 original writer comparisons, plus 361 transaction/load/failure
cases. Serialization and transaction implementations were not changed.

### Track-information resident close and title-font memory repair

Track-information and its load-warning dismissal now keep hardware ownership:
their close functions restore saved pixels/font colours and free memory, with
no file operations. The explicit selected-track load remains a disk boundary.
`diag_track_info_faults.gdb` guards the whole ready-to-dismiss interval against
platform teardown and archive opening, and requires active hardware ownership
at both close checkpoints.

The first rerun exposed memory exhaustion before fault stage 2, not a successful
fault injection: only 64,880 bytes were free (largest 62,816), whereas the
information dialog requires 67,336. The three retained title fonts reserved
8,192 bytes each. They now allocate validated decoded lengths: iso 4,244,
kirj 5,726 and pieni 2,454 bytes, saving 12,152 bytes before Exec rounding.
Cleanup frees each allocation with its exact size. The shared size validator
is also the decoder's validation path; malformed/truncated resources still
fail before any output writes. No font bytes, glyph padding or rendered
behaviour are discarded.

Fresh `make verify-font-resource` compares exact-capacity output and untouched
tail bytes against the original loader for all three fonts, including all
truncations and undersized capacities. `make verify-track-info` passes all
195 supplied tracks and its bounded-input cases. The native five-fault gate
then passes dismissal/retry, two successful preview opens/closes and race
entry. Each of the five restored 64,000-byte screenshots matches the pre-open
snapshot byte-for-byte. The diagnostic logs the reached stage and available
memory on failure rather than silently accepting an earlier allocation failure.

Reproduce from `amiga/`: `SLICKS_TRACK_MENU=8 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/track-info-faults-v1 ./debug.sh '' diag_track_info_faults.gdb`.
Local log: `tmp/track-info-resident-close.log`; snapshots stay in the ignored
run directory. This muted stock A1200/2 MiB gate ends at race entry, not normal
system restoration; the runner closes its emulator. Broader release gates
remain open.

- `make verify-resource-archive`: 182 original-resource comparisons; all 57
  cached resources byte-identical; misses/capacity rejection/close perform no
  I/O or allocation; all 1,193 injected constructor failures release ownership.
- `diag_menu_cache.gdb` with `SLICKS_HELP_MENU=4`: two title Help opens/closes,
  explicit archive-open and platform-end breakpoints reject either during the
  entire navigation interval. Passes with restoration 31.
- Options Help: original chapter/link/history navigation and reopen pass.
- Name dialog: create, edit, reopen, cancel pass. Colour dialog: both endpoints,
  accept, cancel, reopen pass.
- Tracks: entry, selection toggle, return/reopen and correct race handoff pass.
- Track information: open/animate/close/reopen/race pass after the memory repair.
- Live pause: all 13 steps, child dialogs, speed update, unchanged paused race
  state and clock advancement after resume pass.
- Intermission: focused original-policy/edit/next-race test passes after the
  stack repair. The separately repaired UIMENU2 fixture now also passes its
  17 failure/edit/reopen phases using the resident archive and a borrowed
  startup snapshot; see the fidelity audit for allocation and boundary checks.
- Failure recovery: all five track-info injections dismiss/retry/reopen/race;
  every restored screen compares byte-for-byte. Name allocation failure retains
  the editor and passes retry/edit/cancel, with exact saved pixels and profiles.
  Title Help archive/surface/viewer/navigation failure recovery passes.
- Shop: purchases/sale, Help open/close and correct cash/inventory at race
  entry pass. Keyless registration exit Help reaches original chapter 353,
  closes with byte-identical restored pixels and system restoration 31.
  `SLICKS_REGISTRATION_TEST=2/3` supplies the existing Y/F1 diagnostic input;
  value 1 retains the ordinary registration fixture.

Local-only logs are `tmp/menu-cache-*.log`, with the successful intermission
stack run in `tmp/cache-intermission-stack.log`. All native runs are muted PAL
A1200, 2 MiB Chip/no Fast RAM; runners close their owned emulators. Measured
Options free Chip RAM is 192,504 bytes (largest 122,520); nested Help has 82,552
bytes free (largest 66,096). These checkpoints precede the last small staging
change and are not an exhaustive worst-case bound or release validation.

## Saved-game RAM-only modal transitions

The picker-to-name transition, name close, and notice open/close no longer
unconditionally release hardware ownership. Notices reacquire only when needed.
Enumeration, existence checks, delete, store and load retain explicit release
boundaries. The dialog's released-on-return contract is unchanged; catalogue
rereads, return-owner migration and full disk-free navigation remain open.

`diag_championship_save.gdb` now includes `diag_saved_resident.gdb`, checking
active ownership at exercised widget transitions and inactive ownership at
filesystem entry points. The native hidden Save route passes picker cancel,
reopen, name entry, save notice and normal exit with restoration 31. This is
not coverage of every failure cleanup, overwrite, delete or load branch.

Reproduce in a fresh run directory from `amiga/`:
`SLICKS_CHAMPIONSHIP=save SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/championship-resident ./debug.sh '' diag_championship_save.gdb`.
Local log `tmp/saved-resident-save-fresh.log` ends with
`NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`. An initial run using the old directory
encountered an existing E2E.SSS and its overwrite prompt; the fixture expects
a fresh save. The existing file was preserved. The fixture's auxiliary state
snapshots still use `.run/championship-v1`; game file I/O uses the selected
run directory. The muted native runner closed its owned emulator.

Fresh host gates also pass: `verify-saved-file-dialog` (262,144 original-caller
comparisons), `verify-saved-game-storage` (1,485 save-failure and 2,310 load-I/O
cases), and `verify-championship` (state/codec/staging and atomic rejection).
No storage transaction or file-format implementation changed.

## Startup-resident SLICKS.TRK catalogue

Startup now loads the validated catalogue into exact-sized retained storage.
The picker borrows its immutable view rather than allocating 65,536 bytes and
reading the file on every open. Its constructor and ordinary opening transition
are RAM-only and keep hardware ownership. All borrowed modal views close before
the cache refresh at an explicit save/delete disk boundary. Cleanup releases
the snapshot after destroying its borrowers. Missing files retain the original
empty-catalogue semantics (eight bytes). External file edits/repairs are picked
up on the next startup, not through hidden navigation reads.

Refresh preserves the previous allocation on every failure, but records the
error so a failed/recovery refresh cannot silently expose stale entries. The
existing NEW/BAK rules remain authoritative. Successful refresh replaces the
snapshot atomically. Its temporary load buffers remain bounded at 64 KiB each;
these exist only at startup/transaction boundaries, not during list navigation.
This is not yet a whole-game peak-memory or release-gate result.

Fresh host gates: `verify-track-lists` passes 60 reader and 80 writer comparisons
against original instructions. `verify-track-list-storage` passes 361 save-fault
cases and its prior load checks, plus cache allocation/read/close faults,
all catalogue truncations, recovery-artifact preservation, exact-size replacement
and leak-free cleanup. The mock explicitly retains one live cache allocation
between refreshes; its original reset helper requires zero live allocations and
was not suitable for these persistent-owner cases.

Muted native A1200/2 MiB tests use `FSUAE_RUN=.run/track-cache-resident`,
`SLICKS_DEBUG_WARP=1`, and `SLICKS_TRACK_MENU` 2, 3, 4, then 5:

- `diag_track_lists.gdb`: save two selected tracks; subsequent run selects
  the saved list with zero commits. Both reach race entry correctly.
- `diag_track_lists_delete.gdb`: cancel name, cancel delete, confirm delete,
  reopen the newly empty cached catalogue; exactly one disk commit.
- `diag_track_lists_save_failure.gdb` with `SLICKS_DEBUG_READ_ONLY=1`: error
  214, warning dismissal and correct unchanged playlist at race entry.

The shared residency guard now starts at the picker constructor, requiring
active display ownership and prohibiting catalogue/archive loads and teardown
through RAM-only input. Explicit transactions still require released ownership.
Logs: `tmp/track-cache-{save,load,delete,readonly}.log`. All four gates pass;
the runner closes each owned emulator. They terminate at race entry, not normal
system restoration. Saved-game filename caching and broader release gates
remain separate open work.

## Startup-resident saved-game filenames

The saved-game chooser now borrows a fixed 40-by-9-byte startup name snapshot.
Ordinary open, name cancellation, and notice return no longer enumerate the
directory. Explicit store/delete/load operations refresh names before returning
to navigation; existence checks and selected save loading still use real I/O
at their explicit boundaries. The dialog's released-on-return contract is
unchanged and remains open migration work. External edits require restart
unless an explicit file operation refreshes the snapshot.

Enumeration order, filtering and the 40-file overflow error are unchanged.
Refresh stages names before publication: failure keeps prior bytes but publishes
the negative status, so no partial/stale list is displayed as a valid result.
Successful refresh clears unused slots. The cache holds 360 bytes plus its count;
the temporary 360-byte refresh array replaces the previous chooser-local array
on the tested call path rather than adding another persistent allocation.

Fresh host `verify-saved-files` covers startup names, directory filtering,
40/41 entries, empty replacement, lock/examine/read failures (including failure
after one valid entry), preservation on failure and requester restoration.
`verify-saved-game-storage` still passes 1,485 save-fault and 2,310 load-fault/
truncation cases; transaction recovery rules were not changed.

Native `SLICKS_CHAMPIONSHIP=save SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/championship-catalogue ./debug.sh '' diag_championship_save.gdb`
passes the hidden Save route, cancel/reopen, E2E name entry, save and normal
exit/restoration 31. Added assertions require exactly one enumeration before
both picker visits and exactly two after saving, with one published cache entry.
This proves refresh after creation and no rescan on cancel/reopen, not native
overwrite/delete/error coverage; those fixtures remain open. Local log:
`tmp/saved-cache-native.log`. The muted runner closed its owned emulator.

## Saved-game return ownership

Removed the unconditional platform release from `run_saved_game_dialog` cleanup.
RAM-only close/return now preserves ownership; explicit file operations still
release it. Intermission already reacquires conditionally. The existing title
Load owner now uses the resident archive and conditional menu acquisition on
cancel/error rather than reading SLICKS.000 and unconditionally beginning the
display. Its title-entry route is still under audit, so that caller change has
build/source evidence but not an end-to-end native Load-route pass.

The fresh native hidden Save fixture adds `slicks_diag_saved_closed` assertions:
both cancellation and successful-save notice return have active hardware and
no remaining picker/name/message widget. It passes exactly two returns, one
race start, cached enumeration counts and normal restoration 31. Reproduction:
`SLICKS_CHAMPIONSHIP=save SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/championship-return ./debug.sh '' diag_championship_save.gdb`.
Local log `tmp/saved-return-native.log` ends with the native save/exit pass;
the muted emulator exited. Fatal constructor/cleanup paths and original Load
entry still require broader coverage, not inferred from this save-only test.

## Repeated save/edit through the real intermission

CHAMPEDIT no longer starts by trying to select the hidden title Load row.
It follows the same ordinary title/Tracks/GO keys as CHAMPSAVE, advances through
the existing between-race key route, and enters the original hidden Save action
at the first intermission. Only the obsolete initial Load-dialog phase is
skipped in the diagnostic key sequencer; production menus/storage are unchanged.

`diag_championship_edit.gdb` requires a new game, track index zero, three selected
tracks and the 15 subsequent modal phases. It checks the shared hardware/I/O
boundaries and cached name/enumeration counts at every phase. Starting with
one generated E2E.SSS, the sequence cancels name entry, creates TEMP, cancels
and accepts overwrite of E2E, cancels and accepts deletion of TEMP, then exits.
Enumeration stays at one until TEMP creation, rises to two after creation,
three after overwrite, and four after deletion. Names change 1→2→1. No cancelled
operation triggers enumeration. Exit restores the system with status 31 and
does not trigger the diagnostic force-exit fallback.

Reproduce after the CHAMPSAVE fixture in the same isolated directory:
`SLICKS_CHAMPIONSHIP=edit SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/championship-return ./debug.sh '' diag_championship_edit.gdb`.
Local `tmp/saved-edit-native.log` reports
`NATIVE_CHAMPIONSHIP_RESAVE_OVERWRITE_DELETE_CANCEL_OK`. E2E.SSS remains (196
bytes); only the generated TEMP.SSS was deleted. The muted runner closed its
emulator. This replaces the stale entry assumption for edit testing, not the
still-unresolved Load/resume/rejection fixtures or save-failure coverage.

## Read-only championship save failure

CHAMPSAVF (`SLICKS_CHAMPIONSHIP=save-fail`) is diagnostic-only raw-key input:
the same new-race/first-intermission route, initial picker cancellation, name
entry, warning dismissal, reopened picker cancellation and exit. The production
save transaction and error handling are unchanged.

Fresh muted stock A1200/2 MiB run:
`SLICKS_CHAMPIONSHIP=save-fail SLICKS_DEBUG_READ_ONLY=1 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/championship-save-readonly ./debug.sh '' diag_championship_save_failure.gdb`.
The gate checks one failed-save warning, three picker visits, one name dialog,
two display-owned returns, one race start, unchanged empty cache and normal
restoration 31 without forced exit. Exactly two enumerations occur: startup
and refresh after the explicit failed transaction. The DOS requester pointer
matches its pre-transaction value. No SSS/new/bak file was created.

`tmp/saved-failure-native-recheck.log` ends with
`NATIVE_CHAMPIONSHIP_READONLY_WARNING_REOPEN_CANCEL_EXIT_OK`; the runner closed
its emulator. The initial attempt reached the correct warning but its duplicate
store breakpoint did not initialize the requester-check variable; that debugger
error is not a pass. Capture now lives in the shared store breakpoint.

Host regressions also pass: saved-file cache/path/delete checks, 1,485 save
faults, 2,310 load/truncation cases and 262,144 original saved-file caller
comparisons. Native recovery-artifact/enumeration-failure and Load-entry coverage
remain separate; a write-protection test does not establish those branches.

## Intermission return without OS teardown

Normal intermission cleanup now releases its RAM-owned objects and switches
back to the race copper list at display blanking without ending/restarting
hardware ownership. If an explicit preceding boundary left it inactive, it
still acquires the race view. The caller retains its explicit release before
next-track preparation. Fatal paths reach the outer cleanup; retry/skip native
coverage still needs refreshing for this lifetime change.

The extended `diag_intermission_live.gdb` guards from the first live intermission
checkpoint through its closed checkpoint against platform teardown and archive
opening. It requires the modal object gone and ownership active at return.
Fresh `SLICKS_INTERMISSION_LIVE=1 SLICKS_DEBUG_WARP=1
FSUAE_RUN=.run/intermission-live-v1 ./debug.sh '' diag_intermission_live.gdb`
passes all nine Change Cars inputs, two race starts with the exact edited
vehicles, unchanged RNG, one reward per race and restoration 31. Profile
snapshots before/after compare byte-for-byte. The muted emulator exited;
local log `tmp/intermission-return-native.log` ends with
`NATIVE_INTERMISSION_REPEATED_EDITS_SECOND_RACE_OK`.

Fresh host gates pass original intermission initialization/action/draw/prepare
and Change Cars input comparisons, plus the composition/lifetime harnesses
(the latter use stub assets and are not independent pixel evidence). No
selection, rendering or reward policy was changed by this return-path edit.

## Intermission recovery warning return

The allocation-free retry notice no longer releases hardware before restoring
its saved pixels. Retry's existing `retry:` disk boundary still releases before
loading track data; End Match returns through RAM-only cleanup and the owned
race view. The diagnostic-only OPTIONSTK route selects Escape at the same
injected constructor failure that OPTIONSTJ retries, then exits through title
Escape instead of launching another race.

Fresh muted A1200/2 MiB gates use `SLICKS_DEBUG_WARP=1` and
`FSUAE_RUN=.run/pause-transitions-v1`:

- `SLICKS_INTERMISSION_LIVE=2 ./debug.sh '' diag_intermission_retry.gdb`:
  one failed-open warning, successful retry, second track/race, rewards exactly
  once per race, and restoration 31. `retry.before` and `retry.after` session
  snapshots compare byte-for-byte. Local log `tmp/intermission-recovery-retry.log`
  ends with `NATIVE_PAUSE_SKIP_END_TWO_RACES_REWARDS_ONCE_OK`.
- `SLICKS_INTERMISSION_LIVE=3 ./debug.sh '' diag_intermission_skip.gdb`:
  one warning, one owned intermission return, no successful modal open or second
  race load, one reward, and title-Escape exit with restoration 31 and no forced
  exit. `tmp/intermission-recovery-skip.log` reports
  `NATIVE_INTERMISSION_FAILED_OPEN_END_MATCH_EXIT_OK`.

Both share a guard rejecting platform teardown between the warning checkpoint
and emergency-warning close, require active ownership at close, and require
the intermission owner gone with hardware active at return. The runner closed
both emulators. This checks the injected constructor-failure retry/end branches,
not arbitrary low-memory timing or the remaining results owners.

## Post-race record-panel residency

The record-panel constructor now uses the startup archive provider for fonts
and icons. Record reads/date acquisition and writes remain explicit OS work.
After a read-failure retry the load label releases ownership before I/O, and
each save attempt does likewise. The panel's RAM restoration, emergency-warning
close and final RAM cleanup no longer unconditionally end hardware ownership.
Successful return rebuilds the race view and either switches it at blanking
while active or acquires it if the preceding disk operation released ownership.

`diag_records_resident.gdb`, included by the standings gate, forbids archive
opening within the record owner, teardown during warning close/final cleanup,
and record writes with hardware owned. It requires owned returns. The shared
fixture does not instrument every inlined file-read site; those explicit read
boundaries are source-audited and exercised by injected read failure/retry.

Fresh host gates pass 25,272 original post-race qualification/insertion cases,
192 original result waits, 216 records draw traces, both sets of 1,225 record
storage fault cases, and 12 complete records-panel pixel/font comparisons.
The pixel suite also passes its title-independent shared-renderer gates,
including 32 championship cup frames; it is not proof of every live caller.

The first two combined native launches exceeded the debugger's breakpoint
capacity and stopped before gameplay. The fixture now uses one machine-entry
breakpoint for platform teardown and omits the multi-location inlined file
reader breakpoint. Those aborted launches are not passes.

The first read-skip run reached restoration but failed the historical fixture's
assumption that no save fault can follow a skipped first-track read. The pending
one-shot write fault can be reached by a qualifying second-track record.
The fixture now derives expected write-failure count from the actual original
qualification result and still checks one insertion per successfully read
track, two owned returns, and complete standings/statistics/restoration.

Fresh native matrix, from `amiga/`, uses `SLICKS_DEBUG_WARP=1`,
`FSUAE_RUN=.run/post-race-records-v1`, `./debug.sh '' diag_standings.gdb` and
`SLICKS_RECORD_RECOVERY=retry`, `skip`, then `read-skip`. All three pass two
race/result returns, injected read failure, the applicable write-failure route,
three standings phases, profile statistics/persistence and restoration 31.
The passing local logs are `tmp/records-resident-retry-final.log`,
`tmp/records-resident-save-skip.log` and
`tmp/records-resident-read-skip-recheck.log`. All runs were muted PAL A1200,
2 MiB Chip/no Fast RAM, and their runner closed the emulators. This is a
record-owner lifetime/recovery check, not a whole-game release or timing gate.

## Championship results return residency

Championship results now keep display ownership through the initial race fade,
RAM-only cleanup and return. A nonzero-score cup still explicitly releases
ownership to load its on-demand bitmap/palette, closes the disk archive before
takeover, and uses the cached font provider. Zero-score return performs no cup
load. Both copper palettes finish black before the title owner reconstructs
pixels and installs the title palette, avoiding an exposed intermediate view.

Fresh muted native runs passed `diag_championship_zero_resident.gdb` with
`SLICKS_INTERMISSION_LIVE=3` and `diag_championship_resident.gdb` with
`SLICKS_OPTIONS_MENU=8`. Local logs `tmp/cup-resident-zero.log` and
`tmp/cup-resident-points.log` report `NATIVE_CUP_RESIDENT_RETURN_OK`, respectively
zero and one disk loads. Guards checked owned returns, explicit disk boundaries,
all 256 colours in both banks of both copper lists black at return, title
reconstruction entry while owned, and restoration 31. The runner closed both
emulators. Host palette-fade, championship-standings and standings-draw gates
also passed. This is scoped lifetime/fade evidence, not complete menu rectangle
coverage or a refreshed release validation.

## Shared menu rectangle publication

The shared menu dirty callback previously discarded X bounds and published
whole-width row intervals. It now retains up to 16 painter-reported rectangles,
clips them to the visible screen, aligns X to 16-pixel C2P blocks, merges touching
or overlapping rectangles transitively and collapses overflow to their combined
bounds. No shadow framebuffer or per-row flags are used. Empty publications
do not wait; nonempty ones use the existing display-blank wait and rectangle
converter. Actual full-screen constructors still invalidate the whole surface.

`make verify-menu-dirty verify-c2p16` passes 40,000 independently checked
clipped coverage steps plus narrow/disjoint, merge and overflow cases, and the
native converter's single-bit, random, exhaustive horizontal-span, empty,
canary and ABI checks. The normal Amiga build passes.

Fresh muted native tests use `diag_options_rectangles.gdb` with
`SLICKS_OPTIONS_MENU=1`, then `diag_player_rectangles.gdb` with
`SLICKS_PLAYER_MENU=1`. Both retain the existing workflow assertions.
`tools/check_menu_publications.py` independently decodes every captured planar
surface and compares all 64,000 pixels: ten Options publications and six Players
publications pass. Options includes close/reopen and race handoff; Players
includes Down/C/Right/Left/Up. Both runners closed their emulators.
Local logs are `tmp/options-rectangles.log` and `tmp/player-rectangles.log`.

Ordinary Players redraws publish (32,28)-(240,100) and
(32,105)-(144,200): 25,616 pixels rather than the previous full-width row
coverage of 53,440. These cover the original painter's two broad restoration
regions, not merely its highlighted label. This demonstrates reduced publication
area, not measured beam safety or coverage of all nested dialogs. The remaining
per-owner, full-screen restoration and text-bound audits stay on the open list.

Title Help's two-open/two-close workflow also passes all four full-surface
publication comparisons and its no-disk/no-teardown/restoration guards, using
`SLICKS_HELP_MENU=4` with `diag_help_rectangles.gdb`; local log
`tmp/help-rectangles-recheck.log`. The first launch used the Options-Help
scenario by mistake and failed the title-specific fixture; it is not counted
as a workflow pass. Help still marks full-width text rows and restores the
whole saved screen on close; this check establishes pixel correctness, not
completion of its repaint-area reduction.

## Help lifetime restoration bounds

Help now records a bounded rectangle union over its complete modal lifetime,
independently of the pending-publication list. Closing restores those blocks
from the original full-page snapshot, then restores the parent's dirty callback
and font state. Earlier pages remain covered after navigation and publication.
The font adapter reports through the supplied UI callback so text participates
in both lifetime tracking and pending publication. No framebuffer comparison or
extra screen-sized storage is used. Existing full-width text bounds remain
conservative and are still an open tightening item.

`verify-help-dirty` checks empty/disjoint/overflow/reopen cases, full-surface
restoration and callback/font ownership. `verify-help-pixels` passes 98 original
page comparisons, 16 original viewer-entry comparisons and 1,166 text/pixel
comparisons, including saved-screen restoration. `verify-help-refresh` passes
3,024 original orchestration cases. The normal Amiga build passes.

Muted native `SLICKS_HELP_MENU=4` / `diag_help_rectangles.gdb` passes the title
two-open/close ownership gate and all four full-surface publication comparisons.
`SLICKS_HELP_MENU=1` / `diag_nested_help_rectangles.gdb` passes Options chapter,
link/history, close/reopen and system restoration, with all eight published
surfaces matching chunky pixels. Its saved before/after surfaces match byte for
byte. Close publications now cover (0,15)-(320,189), 55,680 pixels instead of
64,000; further horizontal reduction is not claimed. Both runners closed their
emulators. Local logs: `tmp/help-footprint-native.log` and
`tmp/nested-help-footprint-native.log`.

## Help horizontal text bounds

The Help font adapter no longer marks whole rows. Its bounds helper follows
individual glyph positions, including glyph-zero versus missing-character
advance, signed spacing, ten-pixel tabs relative to the anchor, line breaks,
the 1,000-character renderer limit and visible clipping. It deliberately does
not infer the painted extent from whole-string measured width or final advance;
negative spacing and last-glyph overhang make those insufficient. Text bounds
feed both pending publication and the modal lifetime restore list.

The host font gate now records actual native 68020 framebuffer stores and
requires all stores to lie within the reported bounds, including writes that
leave an existing pixel unchanged. A narrow-glyph assertion rejects a fallback
to whole-row reporting. The final gate covers 2,684 strings, including 140
spacing/tab/newline/screen-edge cases, and retains the 98 original page,
16 viewer-entry and 1,166 original line/pixel comparisons. All pass.

The normal build and fresh muted Options-Help chapter/history/reopen native
workflow pass. All eight published surfaces compare exactly, and saved menu
before/after bytes match. The observed Help region is now (16,15)-(304,189),
50,112 converted pixels, including close. Local logs:
`tmp/help-text-bounds-host-final.log` and `tmp/help-tight-bounds-native.log`.
The runner closed the emulator. Other Help owners/failure paths remain in
the publication audit rather than being inferred from this workflow.

## Tracks publication and Track Information restoration

Baseline shared-publication captures pass nine Tracks navigation/reopen/race
surfaces and fifteen Track Information open/animate/close/reopen surfaces.
Track Information now retains painter-reported lifetime bounds, including
preview shimmer, independently of pending dirty-list clears. Close detaches
the owner before restoring/reporting those blocks from the saved menu, so it
does not mutate the list being traversed. It no longer restores all 64,000
pixels unconditionally.

The normal build passes. Fresh `SLICKS_TRACK_MENU=6` with
`diag_track_info_rectangles.gdb` passes all fifteen full-surface comparisons;
saved before/after menus match byte-for-byte. Its close region is
(64,15)-(320,200), 47,360 pixels. `SLICKS_TRACK_MENU=8` with
`diag_track_info_faults.gdb` passes five allocation/resource fault dismissals,
retry/reopen/race and no-teardown guards; all five restored menus match the
baseline. These faults precede drawing; they do not prove arbitrary failures
after painting has begun. Both muted runners closed their emulators.

Host track-info, preview-failure, preview-shimmer and preview-pixels gates
pass: 195 tracks, 16 signed-coordinate/format cases, ten resource-failure
cases, 1,024 shimmer/RNG comparisons and 228 scaler/painter-bound comparisons.
An initial host invocation used the nonexistent `verify-track-info-pixels`
target; the corrected gate uses `verify-track-preview-pixels`.
Local logs: `tmp/tracks-rectangles.log`, `tmp/track-info-bounds-native.log`,
`tmp/track-info-bounds-faults.log`, `tmp/track-info-bounds-host-final.log`.

## Track-list publication and bounded scrollbar restoration

The new rectangle wrappers retain the existing catalogue/cache/transaction
guards and capture every shared menu publication. A fresh isolated catalogue
passes save/name entry (16 surfaces), load (11) and name/delete cancellation,
confirmed deletion and empty reopen (26). All 53 complete planar surfaces
match chunky pixels. Only the fixture-created list is deleted.

The list renderer's auxiliary four-column save-under previously restored
all 200 rows on every close. It now restores the modal vertical extent plus
any actual outlying scrollbar rectangle rows, retaining the original signed
thumb arithmetic. The original full-height 800-byte snapshot remains; no
shadow scan or pixel comparison is used to select dirty bounds. The 88 host
composition cycles up to 2,849 entries still restore all 64,000 pixels and
font state, with an explicit small-list bound check.

After the change, fresh muted save and cancellation/delete/reopen workflows
pass all 42 publication comparisons and their existing storage/ownership
assertions. Both runners close their emulators. Logs:
`tmp/track-lists-rectangles-{save,load,delete}.log` (baseline),
`tmp/track-lists-bounded-{save,delete}.log` (changed build).
An initial host invocation used the nonexistent `verify-list-draw` target;
the actual pixel gate is `verify-list-pixels`. Native large-list and error
publication coverage remains open.

The corrected `make verify-list-renderer verify-list-pixels` gate passes,
including 390 original composed-list full-screen/font comparisons. Two
large-list cases retain the original outlying scrollbar pixels before the
platform's auxiliary restore, which the separate 88-cycle composition gate
checks. The shared suite also retains its name/colour/options/controllers
pixel and font checks. Log: `tmp/list-scrollbar-bounds-host-recheck.log`.

## Resident large catalogue: remove duplicated titles

The new large-list publication audit found an actual 2 MiB regression:
the resident 65,528-byte catalogue loaded, but its separate 59,808-byte
title allocation failed with no-free-store (103). The diagnostic now reports
close failure state, and an allocation-release probe confirmed a null title
allocation with that requested size. The failed runs are not rendering passes.

Track-list pickers now borrow the validated, immutable cached title bytes via
a native 16-bit offset table. Each title lies in the bounded 64 KiB catalogue;
the modal closes before the cache is refreshed/freed. Ordinary profile and
saved-filename lists keep their packed-name storage. At 2,848 entries this
uses 5,696 bytes instead of 59,808, saving 54,112 bytes, with constant-time row
lookup and no extra disk read. Allocation-failure boundaries remain intact.

The normal build and 88 host list composition/restoration cycles pass, with
offset-backed names exercised alongside packed names. Fresh muted native
`SLICKS_TRACK_MENU=9` / `diag_track_lists_large_rectangles.gdb` reaches entry
2,847, cancels, reopens, loads its playlist and enters the race. All fifteen
published surfaces match, and cancel restores the complete saved menu.
`SLICKS_TRACK_MENU=10` / `diag_track_lists_alloc_rectangles.gdb` passes both
injected allocation warnings, byte-exact dismissal restores and successful
retry/load/race; all eighteen publications match. Both runners closed their
emulators. These use the existing isolated synthetic catalogues, not reference
asset modifications. Logs: `tmp/track-lists-large-memory.log` (failure),
`tmp/list-offset-large-native.log`, `tmp/list-offset-fault-native.log`.

The original pixel gate also passes all 390 composed-list comparisons, with
alternating native cases reading titles through offsets into deliberately
different 22-byte records while DOS reads the original packed 21-byte records.
This independently checks that title indexing leaves the displayed text and
navigation result unchanged, including large lists. The shared modal/font
gates remain green. Log: `tmp/list-offset-pixels.log`.

## Players prepared-background return bounds

Players now keeps a second bounded rectangle list for all writes since its
prepared background snapshot. This list is independent of publication clears
and includes the ordinary rows and nested dialogs. Common modal return restores
those rectangles from the original prepared image, clears the lifetime list,
and continues tracking subsequent writes. Tracking is enabled only after the
Players snapshot is prepared; other surface owners retain their existing
behaviour. There is no shadow comparison or additional full-screen allocation.

Before changing restoration, native name-entry and colour fixtures passed
28 and 42 full-surface publication comparisons. After the change, every one
of those 70 chunky surfaces also matches its baseline byte-for-byte, and all
70 bitplane surfaces match their chunky source. Fixtures preserve create/edit,
accept/cancel/reopen, RGB endpoint and font-state checks. Observed editor
returns cover (32,28)-(320,200), 49,536 pixels instead of 64,000.

The shared profile-delete path passes another fifteen complete publication
comparisons, cancel/confirm/count/font restoration checks and unchanged profile
bytes after cancellation. Its final return covers (32,28)-(272,200).
All three runs are muted A1200/2 MiB fixtures and their runners closed the
emulators. Logs: `tmp/name-return-bounds.log`, `tmp/colour-return-bounds.log`,
`tmp/profile-delete-return-bounds.log`; baseline logs are
`tmp/name-rectangles.log` and `tmp/colour-rectangles.log`.

`verify-menu-dirty` and `verify-palette-remap` pass, including 40,000 dirty
coverage steps, 108 original editor and 104 original Players full-frame
comparisons and four original preparation comparisons. The normal build
passes. These runs do not establish all remaining picker/controller/failure
routes or the outstanding full release gate.

## Controllers and profile-dialog failure publications

Fresh muted `SLICKS_OPTIONS_MENU=2` with `diag_controllers_rectangles.gdb`
passes all 38 complete bitplane/chunky comparisons and its existing capture,
reserved-key rejection, defaults, reopen and re-edit assertions. Its final
close publishes (96,78)-(304,162); no new renderer change was needed.
Log: `tmp/controllers-rectangles.log`.

`SLICKS_PROFILE_DIALOG_FAILURE=NF/NL/CF/CL` with the corresponding
`diag_name_failure_rectangles.gdb` / `diag_colour_failure_rectangles.gdb`
passes 30/30/44/44 complete publication comparisons. These cover allocation
and post-paint failure for each child, warning dismissal with the parent editor
retained, successful retry, and the original create/edit/accept/cancel/reopen
assertions. In all four runs, captured screen and profile bytes before the
failure and after warning dismissal match exactly. Logs:
`tmp/profile-failure-rectangles-{NF,NL,CF,CL}.log`.

All five runners closed their emulators. These are 2 MiB A1200 publication and
modal-lifetime checks; they do not close other menu owners, manual joystick
testing or the release-validation gate.

## Pause bounds and emergency-warning ownership

Pause now accumulates lifetime painter bounds in the existing saved-screen
rectangle list. Closing restores only those blocks in chunky memory, including
areas touched by child dialogs; returning to the untouched race bitmap needs
no full-screen conversion. The saved snapshot itself remains full-size.

The normal pause/Help/Controllers/speed/resume scenario and nested child-failure
retry scenario each pass 13 complete bitplane/chunky publication comparisons.
Captured race pixels and car state before and after the menus match exactly.
The original race-menu oracle passes 17,920 key/navigation/result/modal cases.
Fixtures: `diag_pause_rectangles.gdb`, `diag_pause_nested_rectangles.gdb`.
Logs: `tmp/pause-bounds-native.log`, `tmp/pause-nested-bounds.log`,
`tmp/pause-bounds-host.log`. The normal pre-change control is recorded in
`tmp/pause-rectangles.log`.

The opening-failure gate initially failed: the emergency warning called
`platform_begin` while the cached pause owner already held the display.
It now uses the ownership-aware view switch and retains ownership on dismissal.
The first warning initializes the inactive menu bitmap in full, since that
bitmap can still contain the title; this is not an incremental selection repaint.

The strengthened `diag_pause_failure.gdb` passes all five injected opening
failures, warning dismissal, successful retry and resumed engine/simulation
checks. It rejects display release during each pause interval. All five warning
bitmaps independently decode to their complete 64,000-byte chunky surfaces;
all five before/after race structures, configurations and chunky surfaces are
byte-identical. Logs: `tmp/pause-warning-native.log` and
`tmp/pause-warning-ownership.log`; initial failure: `tmp/pause-failure-bounds.log`.
The normal target build passes. Debug runs were muted and their runners closed
the emulators. These checks do not establish coverage of other menu owners or
the broader release gate.

## Standings text bounds and records background initialization

Standings text now reports individual glyph bounds, including alignment and
the original (1,1) shadow, instead of full-width rows. The existing Help bounds
helper shares this calculation with its previous unaligned/unshadowed settings.
`verify-standings-dirty` checks all native 68020 stores in 2,592 combinations
of strings, alignment/shadow flags and clipped positions, including glyph zero,
missing glyphs, tabs and newlines. A single visible glyph must report less than
one whole row. The Help gate still passes 2,684 write-coverage strings, 1,166
original pixel/font comparisons, 98 pages and 16 viewer entries. Standings
drawing passes 20,736 original command-trace comparisons.

The broader native capture exposed a separate records-entry defect: only the
records rectangle was converted into view 0, whose background could still be an
older menu. Capture 43 in `tmp/cup-bounds-native.log` differs outside that panel.
Records entry now initializes all of view 0 and its race palette before showing
it. This is an actual bitmap replacement, not a full-screen selection repaint.
Championship entry likewise legitimately replaces both bitmaps for its fades.

`diag_championship_rectangles.gdb` combines the existing ownership/disk-boundary
gate with shared-publication captures, both final cup bitmaps, and explicit
checks of all 256 high/low copper colours on records entry. The final muted
A1200 run (`SLICKS_OPTIONS_MENU=8`, log `tmp/cup-palette-verify.log`) passes
48 complete 64,000-pixel comparisons, two records-palette checks, one explicit
cup disk load, all three cup phases, owned title return and restoration 31.
The emulator runner exited successfully. Target build and host test logs:
`tmp/standings-final-build.log`, `tmp/standings-dirty.log`.

An intermediate test used two continuing breakpoints at the same standings
checkpoint, suppressing the older phase counter; the capture is now integrated
into the existing checkpoint. A palette assertion initially read an unreliable
optimized caller argument; it now reads the live menu's palette copy. Neither
failed test is counted as verification. These checks cover this championship
route, not all remaining intermission/results/error owners or release gates.

## Shop painted-region restoration

The shop value painter restored the complete decorated background on every
input. It now restores its accumulated painted rectangles, using the existing
menu save-under tracking. The snapshot is enabled only after static composition
is saved. Help writes participate in the same lifetime tracking, so returning
from Help restores all affected pixels, then subsequent value updates return
to their smaller bounds. Labels use glyph/alignment bounds with the original
(1,0) shadow; static icon painting now reports its own rectangle too.

The muted `SLICKS_NATURAL_RESULTS=shop` / `diag_shop_rectangles.gdb` control and
changed runs each produce seven publications. All seven resulting chunky
frames are byte-identical to the control, and every complete bitplane decode
matches its chunky surface. The baseline converted the full screen each time;
the changed ordinary value updates cover (80,10)-(272,118) and
(112,122)-(160,133), totaling 21,264 pixels. Help and its return have appropriately
larger bounds; initial shop entry still initializes the full bitmap.
Cash, purchased/sold inventory, Help open/close and race inventory handoff pass.
The fixture captures the surface pointer at the native function entry instead
of using an unreliable optimized local in the caller.

Host checks pass 5,184 native text-store coverage cases for both shadow modes,
168 complete original shop pixel comparisons and the original shop drawing
oracle. Logs: `tmp/shop-bounds-control.log`, `tmp/shop-bounds-native.log`,
`tmp/shop-bounds-host.log`, `tmp/shop-bounds-build.log`. Both runners closed their
emulators. This closes the unconditional full-screen restore, not the remaining
audit of refresh selectors, all driver/row transitions or failure paths.

## Intermission lifetime restoration and live publications

Intermission now enables lifetime painted bounds after taking its saved-screen
snapshot. Close, including rollback after a post-paint opening failure, restores
only those rectangles. The font colour restoration is unchanged; the saved
snapshot remains full-size. Child vehicle dialogs feed the same tracking.

The focused owner fixture (`SLICKS_INTERMISSION_SURFACE=1`,
`diag_intermission_rectangles.gdb`) passes all 17 failure/reopen/edit/close
phases. Its eight publications are byte-identical to the full-restore control,
and complete bitplane decoding matches every chunky surface. Final restoration
publishes (16,30)-(288,171), rather than the full screen. Logs:
`tmp/intermission-bounds-control.log`, `tmp/intermission-bounds-native.log`.

The live fixture (`SLICKS_INTERMISSION_LIVE=1`,
`diag_intermission_live_rectangles.gdb`) passes 54 complete publication checks,
nine repeated-edit inputs, unchanged profile bytes, deterministic random state,
correct edited vehicles in the second race, two rewards and final system
restoration. Existing guards forbid archive opening and display release during
resident navigation. Log: `tmp/intermission-bounds-live.log`.

Target build and host gates pass: original dispatcher initialization, 65,536
Change Cars words, 3,072 key/selection cases, 30 renderer composition/failure
cases, 75 original intermission pixel/font comparisons, eight Change Cars
open/close comparisons and 20 row redraws. The renderer-only test uses stub
assets; the separate pixel test uses real original/68020 painters. Logs:
`tmp/intermission-bounds-build.log`, `tmp/intermission-bounds-host.log`.
All debug runs were muted and their runners closed the emulators. Saved-game
recovery and other intermission error routes remain separate open coverage.

## Registration Help return publication

Registration Help close now publishes its restored painter bounds before
destroying the surface. The caller no longer reconverts both complete bitmaps:
Help paints view 0 only, while view 1 retains the registration image. Warning
close is explicit before publication as well; failure injection for that route
is still separate coverage.

Muted keyless Y and F1 fixtures (`SLICKS_REGISTRATION_TEST=2/3`,
`diag_registration_rectangles.gdb`) each pass six complete bitplane/chunky
comparisons: open, two navigation publications, close, and both return bitmaps.
The pre-change three navigation frames are byte-identical; before/after Help
screens match exactly. Both keys reach original chapter 353, finish with system
restoration 31, and never release the display inside Help. Return converts
(16,15)-(304,189), 50,112 pixels in view 0, instead of 128,000 pixels across both
views. The palette-only fade continues using both intact images.

Host Help lifetime bounds, 40,000 menu rectangle coverage steps and original
registration pixel comparisons pass. Logs: `tmp/registration-bounds-control.log`,
`tmp/registration-bounds-native.log`, `tmp/registration-bounds-f1.log`,
`tmp/registration-bounds-host.log`, `tmp/registration-bounds-build.log`.
All runners closed their emulators. These checks use no private registration
key and do not close optional order-form, trial-prompt or warning-failure coverage.

## Saved-game failure publications and real catalogue overflow

`diag_championship_save_failure_rectangles.gdb` passes the real read-only
save failure, notice dismissal, reopen/cancel and intermission return on a fresh
isolated read-only volume. All 24 complete bitplane/chunky comparisons pass,
with the existing requester, cache-refresh-count and display-ownership guards.
Log: `tmp/save-failure-rectangles.log`.

An isolated DH1 containing 41 synthetic `S00.SSS`..`S40.SSS` filenames reaches
the actual enumeration overflow status -2. `diag_saved_overflow.gdb`, run with
`SLICKS_CHAMPIONSHIP=save-fail`, passes the single warning/return and final
restoration checks with exactly one enumeration (startup only), no picker
and no second race. All 19 publications match their complete chunky surfaces.
The synthetic files are catalogue-only placeholders and never loaded as games.
Log: `tmp/saved-overflow-native.log`.

`diag_saved_recovery_rectangles.gdb` uses the same native save-failure route on
a writable isolated volume containing only a synthetic `E2E.SSS.new` recovery
artifact. The actual transaction refuses it, reports recovery required, and
passes warning/reopen/cancel/exit with 24 complete publication comparisons.
The artifact's complete text remains unchanged; neither the primary save nor
a `.bak` appears. Log: `tmp/saved-recovery-native.log`. The saved-file host
oracle also passes filtering, bounds, filesystem faults, cache retention,
recovery retention and requester restoration (`tmp/saved-cache-failures-host.log`).

Attempts to inject an enumeration failure through debugger returns/registers
or a cache write did not produce the requested negative status in the actual
menu (it still observed count zero). Those attempts are not verification; the
unverified injection fixture was removed. Native enumeration-I/O error coverage
remains open, separately from the passing real overflow and host fault tests.
All debug runs were muted and all runners exited. No synthetic fixtures or
captured game data are committed.

## Trial reminder prompt bounds

The delayed trial prompt now uses the native font painter's glyph/alignment
bounds and the shared rectangle merger instead of hard-coded full-width rows
145..155. Initial trial composition remains a full image replacement; only
the delayed text update uses rectangles. Both fade bitmaps receive the same
prompt rectangle, with timing and original text/palette unchanged.

`diag_registration_prompt.gdb` with `SLICKS_REGISTRATION_TEST=1` on a fresh
keyless isolated directory prepared by `build/registration_test_config` passes
the actual expired-trial path and restoration 31. The reported bounds are
(144,145)-(176,151): 192 pixels per bitmap instead of 3,200. Both complete
64,000-pixel bitplane decodes match the authoritative chunky surface. The
original trial tint/text/prompt and synthetic owner-label pixel oracle passes,
along with 5,184 native font-write coverage cases for both shadow modes.
Logs: `tmp/trial-prompt-native.log`, `tmp/trial-prompt-host.log`,
`tmp/registration-prompt-build.log`. The muted runner closed its emulator.
The private key was not used; optional image and warning-failure coverage remain
open, and this is not the complete default-stack release gate.

## Combined title and child-menu publication regression

Fresh muted runs on the current integrated build pass the original native
title audits plus independent child-menu bitplane decoding:

- `SLICKS_REGISTRATION_TEST=5`, `diag_title_transition_rectangles.gdb`:
  modes=31, roles=7, counts=3, 33 complete logical-VGA/chunky/planar title
  checks, zero errors, and seven complete child-menu publication comparisons.
- `SLICKS_REGISTRATION_TEST=7`, `diag_arcade_title_rectangles.gdb`:
  counts=15, 30 Arcade draws, 45 complete title checks, zero errors, one
  Options return/publication, and the expected two-human/two-computer race
  handoff. The Options bitmap independently matches all 64,000 chunky pixels.
- `SLICKS_REGISTRATION_TEST=6`, `diag_title_animation.gdb`: 72 pulse updates,
  19 palette-index changes, 73 complete title checks, zero errors, and exactly
  one full-screen publication over more than a complete counter cycle.

All three runs reach restoration 31 and their runners close the emulators.
Logs: `tmp/title-transition-rectangles.log`,
`tmp/arcade-transition-rectangles.log`, `tmp/title-pulse-final.log`.
These close the listed normal title transition/pulse publication checks on the
integrated build; they do not measure wall-clock cadence against DOS or settle
the remaining F10/F11/F12/input/demo callers. No production change was needed.

## Players picker and Help publication coverage

Fresh muted native runs with the shared complete bitplane decoder pass:

- `SLICKS_PLAYER_MENU=3`, `diag_profile_picker_rectangles.gdb`: 12
  publications and four picker visits, with changed selection accepted,
  preserved on reopen and unchanged by cancellation.
- `SLICKS_HELP_MENU=2`, `diag_nested_help_rectangles.gdb`: eight publications,
  link/history navigation, close and reopen; before/after chunky bytes match.
- `SLICKS_HELP_MENU=7`, `diag_help_failure_rectangles.gdb`: seven publications,
  missing-resource and allocation-failure warnings, dismissal, successful retry
  and close. Both warning returns exactly restore the prior screen and preserve
  profile count and all selected profiles.
- `SLICKS_HELP_MENU=3`, `diag_help_page_rectangles.gdb`: six publications,
  previous/next page, Contents and close, with exact before/after restoration.

Every publication compares all 64,000 pixels. The Help fixtures require system
restoration 31; the focused picker fixture ends at its explicit accepted/cancel
checkpoint, and its runner closes the emulator. Logs:
`tmp/profile-picker-rectangles.log`, `tmp/players-help-rectangles.log`,
`tmp/players-help-failure-rectangles.log`, `tmp/players-help-page-rectangles.log`.
An initial picker invocation mistakenly used persistence mode 9 and failed the
picker visit-count gate; only the corrected mode 3 run is counted. No production
change was needed. Extended picker scrolling remains open.

## Tracks storage/format and Help failure publications

Fresh muted native runs pass the following complete-pixel checks:

- `SLICKS_TRACK_MENU=5 SLICKS_DEBUG_READ_ONLY=1` with
  `diag_track_lists_save_failure_rectangles.gdb`: 18 publications, actual DOS
  error 214 on saving, requester restoration, warning dismissal and the
  unchanged two-track playlist reaching race entry.
- `SLICKS_HELP_MENU=8` with `diag_help_failure_rectangles.gdb`: seven
  publications, missing-resource/allocation warnings, exact before/after screen
  restoration for each warning, unchanged profiles/selections, successful Help
  retry and system restoration 31.
- `SLICKS_TRACK_MENU=3` with `diag_track_lists_invalid_rectangles.gdb` and
  an isolated synthetic malformed `SLICKS.TRK`: 12 publications, invalid-format
  result 2, warning dismissal and unchanged one-track playlist at race entry.
  The malformed input remains byte-identical; it is not replaced or repaired.

Every captured bitmap decodes to all 64,000 authoritative chunky pixels.
Logs: `tmp/track-readonly-rectangles.log`,
`tmp/tracks-help-failure-rectangles.log`, `tmp/track-invalid-rectangles.log`.
All runners closed their emulators. No production change was needed. These
cover the named failures, not every catalogue truncation, recovery artifact or
storage failure; the host storage oracle covers additional cases separately.

## Tracks real startup Open failure (2026-09-30)

`diag_track_lists_io_rectangles.gdb`, using TRACKSR on an isolated installation
with an empty directory named `SLICKS.TRK`, observes a real AmigaDOS Open error
212. No target result or state is injected. The cached report remains I/O
failure 1 and names SLICKS.TRK. Warning display/dismissal and race entry preserve
the original one-track playlist (entry zero), with no race error or remaining
Tracks menu. Exactly one track-list load occurs, at startup; repeated loads
would fail the check. Archive reopening and platform teardown are forbidden
while the Tracks menu exists, including its warning and return navigation.

Run `tmp/standalone-release-h7papn50` passes on stock PAL 68020, 2 MiB Chip/no
Fast and the confirmed 4 KiB stack. The independent interleaved-bitmap decoder
matches all 64,000 chunky pixels for each of 12 publications. The isolated
`tmp/tracks-io-directory-mOq4EF/data/SLICKS.TRK` remains an empty directory.
The run is muted and its emulator closed by the harness. No production change
was needed. This closes this real Open failure, not Read/Close failures,
all malformed formats, or post-race/system-restoration behavior: the fixture
terminates at verified race entry.

The first attempted fixture used two command lists at the same ready address;
one continued before the other's counter ran. That test failure was corrected
by keeping warning accounting in the shared failure fixture, which now also
requires a nonzero OS error for I/O failures. It was not a game failure or pass.

## Full Players catalogue scrolling (2026-09-29)

`SLICKS_PLAYER_MENU=19` / `PLAYERSS` with
`diag_profile_scroll_rectangles.gdb` exercises the real input, picker and
publication paths against 100 profiles. Generate the isolated `dh1/SLICKS.PLR`
with `tools/create_picker_scroll_fixture.py`; it writes synthetic names and
fields, refuses to overwrite an existing file, and contains no original game
bytes. The runner supplies only raw key events, not selection-state writes.

The sequence covers both bracket keys and keypad 9/3, scrolling in both
directions, End/Home, Page Up clamping at the first entry, accepting entry 99,
reopening at 99, paging to 89 and cancelling back to the accepted profile.
The fixture asserts all twelve visible selections and final assignment.
All fifteen shared publications decode to the full 64,000 authoritative
chunky pixels. Ordinary scroll publications are one rectangle
`(160,34)..(320,125)` (14,560 pixels), not a full-screen conversion. Modal
open and close publications cover their larger actual owner areas.

Verification: `make verify-amiga-key-scan verify-list-dialog` also passes,
including 172,032 original key/state/scroll/result comparisons and 2,520
original complete drawing/font-state comparisons. Native logs are
`tmp/profile-scroll-native-boundaries.log`; the first page-only run is in
`tmp/profile-scroll-native.log`. Captures are local-only. The muted A1200
runner closed its emulator. This is picker scrolling/publication coverage,
not a claim about every Players dialog or the remaining whole-port audit.

## Tracks paging and boundary restoration (2026-09-29)

`SLICKS_TRACK_MENU=11` / `TRACKSP`, with
`diag_track_scroll_rectangles.gdb`, drives the real Tracks owner through
both bracket and keypad page aliases, End/Home, attempted movement beyond
each endpoint, close and reopen. The original 195-track catalogue reaches
cursor 194/top 173 and restores that position on reopening. All eleven owner
checkpoints pass; the complete playlist is byte-identical before and after.
The initial fixture incorrectly assumed a sorted playlist; the final fixture
captures and compares the actual ordering instead of changing game state.

All nine shared publications decode to all 64,000 chunky pixels. Seven
ordinary redraws use three rectangles `(176,3)..(240,13)`,
`(112,29)..(224,129)` and `(0,10)..(80,192)`: 26,400 pixels, not 64,000.
The two screen entries initialize the full bitmap. Clamped Up/Down at the
first/last entry cause no publication. The title between closing and reopening
uses its separate title publisher and is not counted by this fixture.

`make verify-track-menu verify-track-menu-draw` passes 62,720 original
navigation/state/action comparisons and 392 original full drawing-command,
text/order/state comparisons. Native evidence is
`tmp/track-scroll-native.log`, host results `tmp/track-scroll-host.log`;
captures and playlist snapshots remain local-only. The muted debug runner
closed its emulator. Other storage/format failures remain separate open work.

## Records return: retain the untouched race bitmap (2026-09-29)

The direct row-converter audit found `run_record_results` converting all
64,000 race pixels into view 1 on return, although records and their recovery
warnings only paint view 0. The return now keeps view 1 untouched, preserving
the palette restoration and blanking-safe view switch. No gameplay pacing,
particle ordering or race simulation changed.

`diag_records_resident.gdb` now captures chunky source and view-1 bitplanes
at the ABI entry and completed return of each records owner.
`tools/check_record_return.py` checks two returns: full byte preservation of
both surfaces, plus independent decoding of every interleaved pixel against
the source. The pre-change retry control passed these checks, establishing
that the old conversion was redundant rather than repairing stale pixels.
That control is in `tmp/record-view-control.log`.

Validation uses the existing two-track standings/recovery fixture, retaining
its state, resource-ownership, statistics, persistence and restoration gates.
Run `SLICKS_RECORD_RECOVERY=retry`, `skip`, and `read-skip`, with
`SLICKS_DEBUG_WARP=1`, `FSUAE_RUN=.run/post-race-records-v1` and
`./debug.sh '' diag_standings.gdb`, then run the checker after each launch.
Logs are `tmp/record-return-retry.log`, `tmp/record-return-skip.log` and
`tmp/record-return-read-skip.log`. The source/pixel checks supplement, rather
than replace, the original-code gates: 25,272 post-race qualification cases,
192 wait cases and 216 ordered drawing traces (`tmp/record-return-host.log`).
Other direct row-converter call sites and untested error routes remain open.
All three patched runs passed both return-image checks and the final native
standings/statistics/restoration gate (two returns each, restoration 31).
All were muted and their Slicks emulators exited; no other project's emulator
was stopped.

## Records text producer bounds (2026-09-29)

The platform records-text callback now uses `slicks_font_text_dirty`, with
the records bridge's horizontal-only shadow, instead of deriving a single
line's bounds from measured string width. This preserves painting and existing
input validation, but accounts for actual glyph advances, glyph zero,
tabs/newlines and clipping. It also removes a second independent bounds
implementation. Ordinary-record screenshots had not demonstrated a missing
pixel; this is producer-coverage hardening, not a claim that all reported
menu artifacts had this cause.

`make verify-standings-dirty` passes 5,184 native-store coverage cases for
both records and standings shadow variants. `make verify-track-records-pixels`
passes twelve original records-panel and twelve Track Information surround
full-screen/font comparisons, plus its shared-renderer gates. Logs:
`tmp/records-glyph-bounds-stores.log` and
`tmp/records-glyph-bounds-host.log`.

Live validation uses `SLICKS_TRACK_MENU=6`, `SLICKS_DEBUG_WARP=1`,
`FSUAE_RUN=.run/track-info-v1` and `diag_track_info_rectangles.gdb`.
All fifteen publications match all chunky pixels; the before/after dialog
background is byte-identical. Two open/animate/close cycles, font restoration,
unchanged selection and the subsequent race entry pass. The muted runner
exited. Native log: `tmp/records-glyph-bounds-native.log`.

## Existing saved-game backup preservation (2026-09-29)

`SLICKS_CHAMPIONSHIP=save-fail`, `SLICKS_DEBUG_WARP=1`,
`FSUAE_RUN=.run/saved-backup-v1`, and
`diag_saved_backup_rectangles.gdb` exercise the genuine first-intermission
Save path with an existing synthetic `dh1/E2E.SSS.bak`. The local-only file
contains exactly `Synthetic retained championship backup. Saving must not
replace this artifact.` followed by a newline; no original save or private
data is used. No `E2E.SSS` or `E2E.SSS.new` exists before the run.

The actual filesystem transaction returns recovery-required, presents
`SAVE RECOVERY REQUIRED - KEEP NEW/BAK`, reopens the picker, accepts cancellation
and restores system state 31. The existing lifetime fixture verifies that
RAM-only picker/name/warning closures retain ownership, the transaction runs
with AmigaOS available, and requester state is restored. The backup remains
byte-identical and neither a primary save nor a `.new` file is created.
All 24 shared menu publications decode to their complete chunky surfaces.
The muted runner closed its emulator. Log: `tmp/saved-backup-native.log`.

This covers refusal to overwrite a pre-existing backup, not a failure to
remove a backup after committing a successful save. That later failure and
blocked deletion remain separate open cases.

## Blocked and read-only saved-game deletion (2026-09-29)

`SLICKS_CHAMPIONSHIP=delete-fail` (`CHAMPSAVD`) uses the real first-intermission
Save entry and native raw input: select Delete, confirm Y, dismiss the failure
warning, then cancel the reopened picker and return to title/exit. The new
diagnostic branch is not used during ordinary play. Its four-phase guard
rejects unexpected dialogs rather than feeding further keys into them.

Two isolated filesystem cases pass with `SLICKS_DEBUG_WARP=1` and
`diag_saved_delete_failure_rectangles.gdb`:

- `.run/saved-delete-blocked-v1`: synthetic `E2E.SSS` and `E2E.SSS.bak`;
  the recovery guard refuses deletion and preserves both files byte-for-byte.
- `.run/saved-delete-readonly-v1`, with `SLICKS_DEBUG_READ_ONLY=1`: the same
  primary file without `.new`/`.bak`; the actual filesystem deletion fails
  and preserves it. No recovery sidecar is created.

The primary's contents are `Synthetic primary save; the deletion test must
preserve this file.` plus newline; the backup uses `Synthetic backup; the
deletion test must preserve this file.` plus newline. They are deliberately
synthetic, local-only fixtures; deletion never parses a save payload.

Each run checks one deletion attempt, the confirmation and failure notices,
two picker presentations, refreshed one-entry catalogue, one owned return and
system restoration 31. All 22 publications per run match all chunky pixels.
The runners are muted and close their emulators. Logs:
`tmp/saved-delete-blocked.log`, `tmp/saved-delete-readonly.log`.
`make verify-saved-files verify-saved-file-dialog` also passes native catalogue,
fault/recovery/requester tests and 262,144 original action/result comparisons
(`tmp/saved-delete-host.log`). Post-save backup cleanup failure and native
enumeration I/O failure remain separate work.

## Rejected native enumeration-error probe (2026-09-29)

A temporary diagnostic changed the process directory to a file lock only
during startup catalogue refresh, then restored the directory and requester
state. The host-filesystem run reported `ExNext called for a file`, but did
not reach the error-screen gate. Repeating with a uniquely named temporary
RAM-file lock also failed that gate. An expanded failure diagnostic established
one enumeration with cache count **0**, not the required **-1**. File-as-directory
enumeration is therefore not a valid I/O-error fixture on this setup.

These are rejected probes, not native error-handling passes. The temporary
entry mode and filesystem probe were removed; the RAM probe file was deleted
after restoring the process directory. Logs remain local-only:
`tmp/saved-enumeration-dos.log`, `tmp/saved-enumeration-ram.log`,
`tmp/saved-enumeration-ram-detail.log`. The generic catalogue fixture now prints
enumeration count and cached status when its final gate fails. Future native
coverage must use a controlled failing operation/handler and prove the actual
negative result before testing the UI; no return-value patch was accepted.
After removing the probe, the normal executable was rebuilt and the genuine
41-file overflow fixture passed again (`count=-2`, one warning/return/race,
restoration 31). All 19 publications match their chunky pixels. Log:
`tmp/enumeration-probe-restored-overflow.log`. The runner closed its emulator.

## Title Help warning bounded publication (2026-09-29)

The allocation-independent title warning previously called the full-screen
converter both when drawn and when its saved background was restored. Both
calls now use painter-reported `(20,90)..(300,112)` bounds through the shared
title publisher: block alignment yields `(16,90)..(304,112)`, 6,336 pixels
instead of 64,000. No framebuffer comparison/shadow is used to find damage.

The native build succeeds. In `SLICKS_HELP_MENU=6`, the archive, surface and
viewer failure warnings each open and close with exactly 6,336 published
pixels. `diag_title_help_failure.gdb` captures those six publications under
`.run/title-help-publications`; `tools/check_menu_publications.py` independently
decodes all 64,000 pixels of each and finds no chunky/bitplane mismatches.
Per-call before/after captures for warnings 1 and 2 are byte-identical. The
first call's full-surface comparison differs outside the warning near the
animated title label, so it is not claimed as an exact full-background pass.

**The complete recovery gate does not pass.** The subsequent ordinary Help
open fails the real 110,088-byte viewer allocation on the 2 MiB/no-Fast-RAM
configuration. A temporary instruction-boundary probe observed AllocMem
returning null; the keymap was ready and the injected allocation-failure flag
was clear. That probe was removed. This exposes release-memory work, not a
successful navigation/reopen test or proof of a regression's origin. Logs:
`tmp/title-help-dirty-final.log`, `tmp/title-help-dirty-probe.log`. The muted
runners closed their emulators. The fixture intentionally still fails at the
unmet recovery gate; it has not been weakened to call this a complete pass.

## Title Help backing ownership and recovery repair (2026-09-29)

The viewer no longer embeds a second 64,000-byte save-under array. Its caller
supplies the backing buffer. Only the fresh title Help surface lends its
otherwise-unused `saved` array; it has no parent menu image to preserve.
Players/Tracks/Options/shop/pause and other nested owners allocate separate
backing, freed on open failure, ordinary close or owner destruction. This
reduces title Help peak storage by 64,000 bytes and the viewer allocation from
110,088 to 46,088 bytes; it does not discard resident resources or change
rendered pixels. Nested Help still uses the same total payload, split into
two allocations, and must never alias its parent's backing.

`make verify-help-pixels verify-help-refresh` passes: 98 original Help page
comparisons, 16 topic/language entry comparisons, 1,166 renderer comparisons,
2,684 dirty-text coverage checks and 3,024 refresh/state cases. The native
build passes. The previously failing title recovery sequence now completes
all five opens, four warnings, two successful viewer entries and final system
restoration 31 on 2 MiB/no Fast RAM (`tmp/help-backing-native.log`).
The repeat with an explicit borrowed-pointer assertion also passes
(`tmp/help-backing-title-final.log`). All debug runners were muted and closed
their own emulators.

Nested Players Help also passes missing-resource/allocation-failure recovery,
successful retry/close and restoration 31 (`tmp/help-backing-nested.log`).
Its fixture explicitly rejects a viewer backing pointer equal to the parent
buffer. Both warning returns restore the prior chunky surface byte-for-byte;
all seven shared publications independently decode to all 64,000 chunky
pixels. The title's six fallback publications also still decode correctly.
These close the identified title allocation failure, not the broader release
memory/stack gates or every remaining Help owner.

## Setup-save failure cancellation retains takeover (2026-09-29)

Escape from the setup-save warning used to restore AmigaOS and immediately
take over again while releasing modal objects and rebuilding the resident
title. There is no I/O in that cancellation branch. It now retains ownership
and uses `show_menu`, with a full title publication because the error screen
is being replaced. Actual save attempts remain explicit disk boundaries.

`SLICKS_PLAYER_MENU=12`, `diag_setup_cancel.gdb`, in fresh isolated
`.run/setup-cancel-owned-v2` passes the full create/edit/fail/cancel/reopen/save
sequence on 2 MiB/no Fast RAM. The guard counts **function entries**, not
optimized source locations: between warning and cancelled checkpoint the only
teardown is the diagnostic's deliberate removal of its CFG.new obstruction.
There is none for cancellation. Profile and setup-session snapshots before
and after Escape are byte-identical; ABC remains selected after reopening
Players, the second save succeeds and system restoration is 31.
Log: `tmp/setup-cancel-owned-detail.log`. An earlier source-line breakpoint
overcounted calls (`tmp/setup-cancel-owned.log`); the entry trace resolves it.
Restarting that same isolated disk with `SLICKS_SETUP_RELOAD=1` and
`diag_setup_cancel_reload.gdb` passes the persisted configuration/profile
load and race handoff: selected ABC, vehicle, six colour endpoints and all
four participation/vehicle assignments match (`tmp/setup-cancel-owned-reload.log`).
Both runners are muted and close their emulators. No ordinary-launch save
files were touched.

## Title Read This/F1 publication and lifetime gate (2026-09-29)

`SLICKS_HELP_MENU=4` with `diag_title_help_rectangles.gdb` combines the actual
native title input route with shared publication captures and guards against
display teardown or archive reopening while the title Help owner exists.
Read This resolves to original chapter 353/page 0; title F1's empty topic
resolves to chapter 9589/page 0, not the in-viewer Contents action. Both open
and close successfully, and final system restoration is 31.

All four publications are `(16,15)..(304,189)`, 50,112 pixels rather than
64,000. `tools/check_menu_publications.py` independently decodes each complete
bitmap and matches all chunky pixels. Both `after-read.chunky` and
`after-f1.chunky` match `before.chunky` byte-for-byte. The isolated runner
`.run/title-help-v1` is muted and closes its emulator. Log:
`tmp/title-help-rectangles-current.log`. This checks normal title Help; it
does not cover registration Help or every nested owner/error route.

## Registration Help normal and allocation-failure publication (2026-09-29)

The keyless exit-screen caller was checked separately from title Help on the
current 2 MiB/no-Fast-RAM build. `SLICKS_REGISTRATION_TEST=2` with
`diag_registration_rectangles.gdb` passes chapter 353/page 0, link down/up,
close, optional absent-order-form handling and restoration 31. Four normal
Help publications plus both returned display buffers pass independent full
64,000-pixel decoding; exit before/after chunky images are byte-identical.
Log: `tmp/registration-help-current.log`.

New diagnostic `REGCHECKG` (`SLICKS_REGISTRATION_TEST=9`) applies the existing
viewer-allocation fault only at the registration Help caller. It uses ordinary
F1 and acknowledgement key events, not a patched return value. Normal launches
are unchanged. `diag_registration_help_failure.gdb` asserts the fault was
consumed, no viewer was opened, the warning exists, and display ownership is
retained through return. The two warning publications are exactly
`(64,96)..(256,110)`, 2,688 pixels each. Those and both restored display buffers
pass full chunky/planar comparison; the before/after exit images are identical.
The keyless optional-image attempt and restoration 31 still pass. Log:
`tmp/registration-help-failure.log`.

Both runs used fresh isolated data directories, were muted and closed their
emulators. No private key/owner data was captured. This closes the registration
Help *caller's* viewer-allocation warning case using keyless data; it does not
claim archive/surface allocation failures, malformed navigation, or new
registered-key coverage. The normal case's captures are preserved under
`.run/menu-rectangles-registration-help-current`; failure captures use
`.run/menu-rectangles`.

## Registered title-owner bounds (2026-09-29)

The owner-name pulse previously dirtied the full `(0,190)..(320,200)` strip.
It now uses the shared glyph-aware bounds walker with the actual small font,
right-aligned anchor `(310,190)`, spacing 1 and flags 2, matching the native
registration text bridge. The owner name is fixed during a session, so no
old-name erasure rectangle is needed. Full-screen replacements still publish
the full screen; this changes ordinary title repaint bounds only.

The independent bounds suite passes (`tmp/title-owner-host.log`). A muted
registered REGCHECKT run passes 29 bounded owner publications and 33 complete
logical/chunky/bitplane comparisons, with zero display errors and restoration
mask 31 (`tmp/title-owner-native.log`). The diagnostic rejects full-width
bottom-strip rectangles for this supplied name while checking ordinary modes,
roles and count-edit coverage. Very long names may legitimately clip across
the whole width; that is not prohibited by production code.

The explicit private key fixture remains ignored, and no key/owner contents
or registered framebuffer dumps were logged. The emulator exited and closed.
Build log: `tmp/title-owner-build.log`. This closes this one over-wide callback,
not the remaining whole-menu dirty-region audit.

## Ordinary menu text uses glyph bounds (2026-09-29)

The shared `amiga_player_menu.c` text callback now reports bounds through
`slicks_font_text_dirty`, matching the other font adapters. Its native drawing,
alignment, single-line validation and no-shadow policy are unchanged. Measured
string advance still determines alignment, but no longer substitutes for the
actual glyph footprint when publishing dirty pixels.

`verify-standings-dirty` now also executes the ordinary `slicks_menu_text`
bridge, alongside records and standings, and checks every native store against
the reported bounds: 7,776 cases pass, including glyph zero, alignment,
spacing controls, clipping and both shadow forms. This bridge-level suite is
broader than the ordinary callback's accepted single-line/no-shadow inputs.
Log: `tmp/menu-text-bounds-host.log`; build: `tmp/menu-text-bounds-build.log`.

The current stripped executable passes the Players-menu six-draw sequence
(Down, C, Right, Left, Up) on a stock-speed 68020, 2 MiB Chip/no Fast and
confirmed 4 KiB stack. All six published bitmaps match all 64,000 chunky
pixels using the independent planar decoder. Evidence:
`tmp/standalone-release-3ujchf78`, with captures in its
`.run/menu-rectangles`. The muted emulator was closed by the harness after
the checkpoint; this is not a normal-exit check. Other owners of the shared
callback still require their own workflow/error coverage. No speedup or
reduction in merged rectangle area is claimed from this run.

## Saved catalogue directory-lock failure (2026-09-29)

`CHAMPSAVX` selects a one-shot native catalogue fault before the startup
snapshot. The scanner calls real AmigaDOS `Lock` on
`SLICKS.000/scan-failure`, traversing a regular original asset as a directory;
no file is created, renamed or modified and no return value is fabricated.
Unlike the earlier probe that examined a file lock as a directory and saw an
empty catalogue, this operation fails at Lock with error 212 (wrong type).
Normal launches continue to lock the current directory.

The current stripped binary passes on stock-speed PAL 68020, 2 MiB Chip/no
Fast and confirmed 4 KiB stack. After a genuine first intermission, the Save
owner observes the cached error and displays `CANNOT READ SAVED GAMES`.
The diagnostic requires one enumeration, fault consumption, restoration of
the process requester pointer, no save call, retained display ownership at
warning/return, and normal system-restoring exit (mask 31).

All 19 captured menu publications match all 64,000 chunky pixels using the
independent planar decoder. Warning and restoration each publish only
`(80,96)..(240,110)`. Evidence:
`tmp/standalone-release-e468y0bq`, with captures beneath
`.run/menu-rectangles`; build and host regression logs:
`tmp/saved-scan-build.log`, `tmp/saved-scan-host.log`.
The debug run was muted and its emulator closed.

This covers a real Lock failure and its cached menu route, not a partial
`ExNext` failure, repairing an externally damaged directory while running,
or the separately unresolved Load Game entry route.

## Registration Help failures before viewer creation (2026-09-29)

Previously an unavailable cached archive or failure to create the Help surface
returned an error immediately from registration Help, aborting the remaining
exit presentation. These native platform failures now show the existing
allocation-free emergency warning with the resident small font. The warning
retains display ownership, restores its saved rectangle on acknowledgement,
and allows normal exit presentation to continue. This is native error recovery,
not a claim that this error text occurs in the DOS original.

Both registration bitmaps already contain the exit image. Only the warning
rectangle in view 0 is converted on opening and closing, with X bounds expanded
to the converter's 16-pixel block boundary. The emergency dialog exposes its
saved rectangle through a checked read-only bounds accessor; it does not add
another screen buffer or allocate memory to report the failure.

Explicit keyless fixtures `REGCHECKH` (unavailable cache) and `REGCHECKI`
(null surface result) exercise the production recovery. They pass on the
current stripped executable, stock-speed PAL 68020, 2 MiB Chip/no Fast and
confirmed 4 KiB stack. Each requires one warning/return, no display teardown
or full-row converter while the warning owns the display, continuation to
the optional-image attempt, and system restoration 31. All three captured
surfaces per case (visible warning and both restored bitmaps) match all 64,000
chunky pixels. The pre-warning and restored images compare byte-for-byte.

Passing runs: `tmp/standalone-release-blqk6ose` (archive) and
`tmp/standalone-release-de_ctecy` (surface). Normal registration Help also
passes on this build: `tmp/standalone-release-zk_92it3`.
Build: `tmp/registration-early-final-build.log`. All runs were muted and
their emulators closed. An earlier unaligned publication passed lifecycle
checks but failed the independent pixel comparison; it was corrected before
acceptance and is not passing display evidence.

These fixtures simulate the unavailable cache/null surface boundaries; they
do not exhaust every partial allocation inside surface creation. Malformed
Help navigation remains a separate open route.

## Registration Help navigation error recovery (2026-09-29)

A Help parser/rendering error during registration navigation previously
returned an error after closing the viewer, without publishing its restored
background, and aborted the exit presentation. The owner now closes the
viewer, publishes its accumulated restoration bounds, and enters the
allocation-free warning path before continuing normal exit presentation.

`REGCHECKJ` first opens the real registration Help topic, corrupts one byte
in the viewer's private chapter buffer, then sends the ordinary Down event.
The actual parser rejects the redraw; its return value is not patched.
The native failure/return fixture now captures the reference background at
the registration-screen checkpoint, before Help opens, rather than at warning
entry. All three final surfaces (warning and both restored bitmaps) match
all 64,000 chunky pixels; the final background matches that original reference
byte-for-byte. The warning retains the display and avoids full-row conversion.
Normal exit reaches restoration mask 31 on the stripped binary with a
confirmed 4 KiB stack, stock-speed PAL 68020 and 2 MiB Chip/no Fast.

Evidence: `tmp/standalone-release-dc64f6ji`; build:
`tmp/registration-malformed-build.log`. This tests the common error-return
route using one malformed chapter, not exhaustive malformed-document fuzzing.
The test alters only the private viewer copy, never the archive or cache.
Normal registration Help also passes on this build
(`tmp/standalone-release-gqbvgpxh`). Both runs were muted and their emulators
closed.

## Shop no-op and Help-return publication (2026-09-29)

The production shop now redraws values only for redraw, buy and sell actions.
Ignored input and exit do not redraw; closing Help publishes its saved-image
restoration without repainting the shop over it. The original loop at
`2cf6d` skips to `2cfcc` when both refresh selectors are zero; the
original-instruction shop verifier now executes and checks that branch.
Its 4,096 dynamic and 512 background command/font comparisons also pass.

The native fixture inserts an ignored A key before its capture, buy/sell,
Help and exit sequence and requires exactly five shop draws: initial creation,
post-computer purchases, two buys and one sell. The stripped-binary run
`tmp/standalone-release-_w7jo_l8` reaches `SHOP_RACE_ENTRY_OK`, with the
expected cash, inventory, selected weapon and Help return state. All six
recorded publications match all 64,000 chunky pixels. This is a race-entry
gate, not a normal-exit gate. It used a confirmed default 4 KiB stack,
stock-speed PAL 68020, 2 MiB Chip RAM and no Fast RAM; the muted emulator
was closed. Build evidence is `tmp/shop-noop-build.log`.

This does not finish driver/row-specific refresh selectors or shop failure
coverage, and is not a performance benchmark. Those remain actionable.

## Shop selective-refresh caller audit (2026-09-29)

Later caller-level resolution: see **Sparse shop caller undefined selection**
below. The mismatch is an original undefined-result bug, not evidence that
the native caller should blindly pass packed columns as actual drivers.

The original row-navigation branches (`2d075` and `2d085`, stopping at
`2d229`) now have 840 executable comparisons in `verify-shop-draw`: every
row count 0–13, every valid row, four columns and both directions. A genuine
row change sets the driver selector to selected column plus one and the row
selector to -1; a boundary key leaves both selectors zero. These checks pass
alongside the existing zero-refresh and painter comparisons.

This exposes two integration constraints, not a completed selective-refresh
implementation. `slicks_amiga_shop_draw` currently restores every recorded
saved-background dirty area and passes -1/-1 to the painter. Partial selectors
must be paired with removing that blanket restore: the original painter
already restores its selected cells. Also, the original input caller derives
the driver selector from its packed column, whereas the painter compares it
against actual driver indices. Sparse participating-driver configurations
therefore need original caller-level coverage before choosing a mapping;
passing the native actual driver without checking would not reproduce those
instructions. Horizontal navigation and transactions still need their
selector/lifetime integration checks. No production behavior changed in this
audit, and no target performance result is claimed.

## Native shop constructor failure cleanup (2026-09-29)

`NATURALWF` runs five one-shot constructor fault cases against the existing
startup cache: unavailable palette, unavailable surface result, failed 64 KiB
staging allocation, unavailable background and unavailable icon. The last
case fails after partial painting. Every attempt must return NULL, consume
its fault and recover its pre-attempt free-memory total. Brief `Forbid`/`Permit`
bracketing prevents unrelated tasks skewing that accounting; interrupts remain
enabled and the constructor reads only resident resources. The surface-result
case is an injected boundary failure, not proof of all nested font/allocation
failures or physical memory exhaustion.

After those failures the normal constructor and `NATURALW` input sequence run.
The target passes the existing two-human selection, purchase/sale, Help-return,
draw-count, inventory and race-entry gates. All ten shared publications match
the complete chunky surfaces. No failed construction is published; the fresh
successful constructor reloads its background from the cache.

Evidence: `tmp/standalone-release-r_6b8kkl`, stripped binary on stock PAL 68020,
2 MiB/no Fast, default 4 KiB stack; build `tmp/shop-create-failures-build.log`.
The owned muted emulator was closed. This closes the five named cleanup
boundaries, not live error-notice dismissal/retry, nested surface/font failure
coverage, or the separate sparse-player policy question. No production cleanup
fix was needed.

## Native shop live failure dismissal and retry (2026-09-29)

`SETUPS1..5` inject the same five constructor boundaries individually through
ordinary GO preparation. Each run displays error 9, dismisses it with Escape,
opens and closes Players, retries GO, opens the shop successfully, and exits
the shop with Escape to reach gameplay. The consumed one-shot fault is not
reapplied. These diagnostic fixtures do not alter normal launch behavior.

`diag_shop_live_failure.gdb` combines the normal preparation transaction gate
with complete publication capture. Session and configuration dumps before
preparation and after failure are byte-identical in every case. Each run has
four full 64,000-pixel comparisons, including the error screen and restored
title; all match their chunky surfaces. This verifies publication consistency,
not an independent DOS pixel oracle for the generic platform error message.

Runs in fault order: `tmp/standalone-release-6f7zkrb2`,
`tmp/standalone-release-0ccn_hl3`, `tmp/standalone-release-ns2x31t8`,
`tmp/standalone-release-ylugiccn`, `tmp/standalone-release-8e_rx1cl`.
All use the stripped binary, confirmed 4 KiB stack, stock PAL 68020 and
2 MiB Chip/no Fast. Build: `tmp/shop-live-failure-build.log`. All muted
emulators were closed. No production recovery fix was required. These gates
end at race entry, not normal process exit; nested font/surface helper
failures and sparse-driver policy are not covered by these five boundaries.

## Native shop purchase rejection boundaries (2026-09-29)

Explicit `NATURALWX/Y/Z` fixtures set up the first visible weapon before shop
creation: respectively cash one unit below its price, its item count at the
original limit, and no remaining vehicle carrying capacity for that weapon.
The carrying-capacity fixture seeds other weapon slots with their ownership
sentinel; it is controlled test setup, not a naturally acquired inventory or
a registration-policy change. Normal launches cannot enable these fixtures.
All three press Buy twice through the normal key queue, then exit toward the
prepared race. No debugger operation changes target state.

`diag_shop_rejection.gdb` independently checks the intended boundary, snapshots
all four cash values and 52 inventory words, and requires them unchanged after
both attempts. There are exactly two initial shop paints (creation and the
post-computer setup), no attempted-purchase repaint and no additional shared
C2P publication. Each initial publication compares correctly across all
64,000 pixels. The gate reaches race entry; it does not claim normal race exit
or unrelated shop resource-failure recovery.

Final-build runs on stripped binaries, stock PAL 68020, 2 MiB/no Fast and a
default 4 KiB stack:

- Cash: `tmp/standalone-release-nbz1r3iu`.
- Item limit: `tmp/standalone-release-idobzdz0`.
- Vehicle carrying capacity: `tmp/standalone-release-i_fr005r`.

All owned muted emulators were closed. Build: `tmp/shop-rejection-build.log`.
The original transaction regression also passes 32,768 price and 65,536
buy/sell comparisons, plus computer/RNG and navigation/row checks
(`tmp/shop-rejection-host.log`). No production transaction change was needed.

## Sparse shop caller undefined selection (2026-09-29)

`verify-shop-draw` now executes the original caller before invoking the painter,
rather than supplying a valid packed column directly. Two valid sparse profile
layouts expose a defect: only driver 3 active, and drivers 1/3 active followed
by Right. Initialization `2cf50..2cf5b` calls the original human-driver scan;
Right `2d03f..2d229` also returns an actual driver index. Both routes produce
index 2, which the painter `2c574` interprets as a packed column even though
there are only one or two displayed columns.

The painter initializes BP-1 and BP-3, but not its result at BP-2. No column
matches, and `2c960` returns the untouched result byte. The test seeds that
local with 77 and 99 in separate runs and observes each returned unchanged.
It stops there: no transaction consumes those undefined indices. This is
original-instruction evidence, not a DOSBox UI observation or permission to
reproduce out-of-range inventory access.

The existing native mapping keeps actual driver IDs for transactions and
converts them to packed columns for drawing; its painter rejects unmatched
columns with -1. No production mapping changed in this audit. The user has
been asked whether to retain correct sparse mapping as an explicit compatibility
fix or reject sparse shop setups. Native sparse navigation/purchase/publication
coverage remains open pending that policy decision. Contiguous layouts remain
covered by the existing original comparisons. `make verify-shop-draw` passes
the two new sparse cases (two stale-result seeds each), 840 row-navigation
cases, 4,096 painter comparisons and 512 background comparisons.

## Selective shop publication integration (2026-09-29)

`slicks_amiga_shop_refresh` now passes driver/row selectors to the original
painter without first restoring every saved dirty area. The painter's own
cell restores are authoritative. Initial creation and post-computer purchases
still request all cells. Vertical movement and transactions refresh one
driver's rows; horizontal movement refreshes the selected row across drivers.
Unchanged navigation skips publication. This retains the native caller's
existing actual-driver selection and packed display-column conversion; it
does not resolve the original sparse-selection caller discrepancy above.

`verify-shop-pixels` now exercises full, driver-only and row-only refreshes
in sequence without resetting the framebuffer between them. All 440 complete
screen comparisons pass against original instructions and assets, including
native 68020 text. The 840 row-navigation, 4,096 dynamic drawing and 512
background comparisons also pass.

The stripped native fixture adds Up at the first row, Down/Up and Right with
only one human selectable. It requires seven painter calls and the existing
buy/sell cash/inventory and Help-return checks. Run
`tmp/standalone-release-zfab4owj` passes; all eight publications match all
64,000 chunky pixels. Configuration: stock-speed PAL 68020, 2 MiB Chip,
no Fast, confirmed default 4 KiB stack. This is a race-entry checkpoint, not
a normal-exit check. The muted emulator was closed. Build evidence:
`tmp/shop-selective-build.log`.

Remaining shop coverage includes actual multi-human horizontal changes,
sparse selection caller behavior, rejected transactions and failure routes.
No gameplay performance benchmark was run; performance work remains paused.

## Two-human shop and rejected transaction publication (2026-09-29)

The native caller now checks the original transaction helper's changed-item
count and skips drawing when it is zero. This follows the original caller:
buy refresh selectors are assigned after an actual increment (`2d13c`),
and an empty sale returns at `2d199` before the refresh assignment. Partial
successful batches still redraw.

The ordinary `NATURALW` fixture now selects two human profiles and moves
Right/Left before making an empty-inventory sale, two purchases and one sale,
opening/closing Help and entering the race. Weapon-specific and transition
fixtures retain their original one-human configuration. The gate explicitly
observes driver 1, requires nine painter calls for four transaction attempts
and the navigation sequence, and checks final driver-0 cash/inventory and race
handoff. Run `tmp/standalone-release-18l5o5jy` passes. All ten publications
match all 64,000 chunky pixels. It uses the stripped binary, confirmed 4 KiB
stack, stock-speed PAL 68020, 2 MiB Chip and no Fast RAM. The muted emulator
was closed; this remains a race-entry rather than normal-exit gate.

`tmp/shop-rejected-host.log` records 32,768 original price comparisons,
65,536 buy/sell transactions, 4,096 computer-shop calls, 1,215 driver-navigation
and 2,720 row-mapping comparisons. Build: `tmp/shop-rejected-build.log`.
This does not establish native insufficient-cash/capacity boundary coverage,
sparse profile selection behavior or shop asset/allocation failure recovery.

## Partial saved-game catalogue error return (2026-09-29)

The scanner already returns failure, not its accumulated filename count,
when enumeration ends with an error other than `ERROR_NO_MORE_ENTRIES`.
`CHAMPSAVY` now tests the native caller after one real matching filename:
an explicit diagnostic injects a failed next-entry boundary with error 212,
then lets the normal IoErr handling reject the partial catalogue. This is
an injected API error, not evidence of a genuinely failing disk or filesystem.
The private fixture's `PROBE.SSS` is only a catalogue-name probe (copied data,
not a valid save); no attempt is made to load it.

`tmp/standalone-release-en_5y6_p` reaches the genuine first intermission,
shows CANNOT READ SAVED GAMES, returns with hardware ownership retained, and
exits with restoration mask 31. The gate requires exactly one startup scan,
one accepted filename before the injected failure, consumed fault state,
cache count -1, restored requester pointer, one warning and return, and no
save attempt. All 19 menu publications match all 64,000 chunky pixels;
warning and restoration use the same 80,96–240,110 rectangle.
Configuration: stripped binary, confirmed default 4 KiB stack, stock-speed
PAL 68020, 2 MiB Chip/no Fast. The muted emulator was closed.

Build: `tmp/saved-partial-build.log`. `make verify-saved-files` also passes
its existing host partial-enumeration and cache-retention tests. Production
error policy is unchanged; this closes the bounded native partial-scan
warning/return coverage, not all filesystem failure modes.

## Committed save with failed backup cleanup (2026-09-29)

`CHAMPSAVB` selects a private existing E2E save from the real first-intermission
picker, accepts overwrite, acknowledges the result and exits. Immediately
before backup cleanup, its explicit diagnostic sets AmigaDOS delete protection
on `E2E.SSS.bak`. The real DeleteFile fails; its result is not fabricated.
Normal launches never enable this fixture. The backup is deliberately left
protected and intact in the private test directory.

`tmp/standalone-release-49gtqbsq` passes the overwrite confirmation,
GAME SAVED - BACKUP REMAINS notice, retained-display dialog return and normal
exit with restoration mask 31. All 21 menu publications match all 64,000
chunky pixels. The stripped binary runs with a confirmed default 4 KiB stack,
stock-speed PAL 68020, 2 MiB Chip/no Fast. The muted emulator was closed.

In `tmp/saved-cleanup-release-0W8Ag7/data`, E2E.SSS is a new structurally
valid 196-byte original-format save with three tracks, next-track index one
and four driver records; no `.new` remains. The `.bak` is byte-identical to
the original 41,500-byte private probe (a copy of SLICKS.DAT, never loaded as
a save). This proves preservation across cleanup failure, not loading that
probe or a subsequent resume while a backup remains.

Build: `tmp/saved-cleanup-build.log`. Storage regressions in
`tmp/saved-cleanup-host.log` pass setup transaction/load, track publication,
1,485 saved-game transaction faults and 2,310 saved-game load fault/truncation
cases. No production save/recovery policy was changed.

## Tracks recovery-artifact publication (2026-09-29)

Current stripped-binary `TRACKSR` runs with separately isolated
`SLICKS.TRK.new` and `SLICKS.TRK.bak` pass
`diag_track_lists_recovery_rectangles.gdb`. Each fixture supplies an existing
recovery artifact before startup; the real loader returns recovery result 4
with the matching path. The menu shows and dismisses the warning, retains the
original one-track playlist, and reaches the race without another catalogue
load on navigation. The artifacts remain byte-identical to their input probes.

Runs: `tmp/standalone-release-1yanv_u0` (.new) and
`tmp/standalone-release-9vz_c24s` (.bak). Each passes 12 full 64,000-pixel
publication comparisons, including identical 64,96–256,110 warning/restore
bounds. Both use a confirmed default 4 KiB stack, stock-speed PAL 68020,
2 MiB Chip/no Fast. Their muted emulators were closed. These are race-entry
checks, not normal-exit or recovery-file repair/resume checks. No production
behavior changed, and no recovery artifact was deleted or overwritten.

## Direct full-screen publication inventory (source audit, 2026-09-29)

Inspection of `slicks_diag.c` distinguishes full-screen initialization from
ordinary menu updates. The following direct `slicks_chunky_rows_to_amiga`
callers require destination-background validity before they can be narrowed:

| Caller group | Reason for full publication at this boundary |
| --- | --- |
| Race preparation | Installs the newly rendered track in view 1. |
| Shop entry | Installs the tuning screen in view 0 before interaction. Subsequent shop draws use the shared dirty publisher. |
| Pause/intermission entry | View 0 may contain an earlier title/menu rather than the current race background. Subsequent menu interaction uses the shared publisher. |
| Emergency pause/intermission/record recovery | Creates a warning over the current chunky background, but the destination view can be stale. This is not evidence that a warning-sized conversion alone is safe. |
| Record results entry | Explicitly initializes view 0 from the race background plus overlay. |
| Registration and championship fades | Initializes both bitmaps before alternating them for palette fades; a fade itself is not repeated full-screen C2P. |
| Component pause/intermission diagnostics and rendering reference | Test setup/reference conversions, not normal selection-update loops. |

This inventory is source-level classification, not a native proof of every
error route or ownership transition. It does not authorize a blanket removal
of these conversions, nor establish that every initialization is minimal.
If a caller is changed to reuse a view already containing its background,
that invariant and the resulting partial publication need separate pixel
coverage. The actionable list retains remaining error-route and lifetime
checks; this inventory narrows where to look for selection-time overdraw.
