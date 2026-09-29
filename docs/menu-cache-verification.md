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
