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
