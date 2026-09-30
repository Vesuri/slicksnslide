# Development release audit

## 2026-10-01 — final native memory workflow; B12 closed

All runs below use the stripped executable with SHA-256
`cd579a700f6490e32e61c9832f2bb79aca0649469e26fb696943a3278fe15282`.

- `tmp/standalone-release-i4n7z5me`: DEMOPLR, Workbench-loaded PAL A1200,
  2 MiB Chip/no Fast/default 4 KiB stack. Two natural demo returns and Players
  open/close pass, then the entire launch is repeated in the same OS session.
  First-launch audit: 131 allocations, zero failures, return 0, no outstanding
  blocks or workspace conflicts. The extra allocation is the explicitly
  diagnostic playlist snapshot; this run does not claim startup-only mode.
  Both launches leave exactly 1,891,728 Chip bytes available and a 1,890,816-byte
  largest block. No cumulative loss or fragmentation appears in this test.
- `...-vafppvog` CHAMPSAVE then `...-rlg49zof` CHAMPEDIT: same stock settings,
  strict startup-only allocation audits pass. Each has 130 startup allocations,
  zero failed/outstanding blocks, return 0 and no workspace conflicts. Saving,
  editing/deletion and cancellation reuse the reservations.
- `tmp/whdload-test-mo5up_ag` and `...-qav2v3yw`: stripped game launches and
  returns normally through the diagnostic exit slave, with and without PRELOAD
  respectively; 2 MiB Chip plus documented 4 MiB Fast. These checks do not
  replace the production-icon manual race workflow.
- Every emulator started for these checks has exited.

B12 completion evidence combines the current source/caller census (all
remaining production allocations are startup-only), target-sized lifetime
layout and ownership guards, bounded/zero-allocation host storage tests,
native strict save/edit/error-recovery audits, low-memory launch cleanup,
natural-demo/menu/relaunch cleanup and the clean release/package check.
The nine diagnostic-only source allocation expressions are expressly outside
the normal-game contract. OS filesystem/library internals remain OS-owned;
their errors are recoverable, not guaranteed away by startup reservations.

B11's final manual intermission/save retry and the broader D-3 release checklist
remain open. This closes memory ownership, not those separate observations.

## 2026-10-01 — clean release-check after startup-memory migrations

`make release-check` passes at 4aa7262. This includes the timing/physics,
collision, Help, keyboard, loading/font and partial-menu rendering oracles;
378 complete Tracks preparation and 168 sequential redraw comparisons; and
installer extraction of all 197 original files with key/settings preservation
and corrupt/truncated/wrong-ZIP rejection.

Two clean builds produce identical stripped executables. The subsequent
package rebuild matches them, and the scratch LHA passes the independent
11-member allowlist/decompression/CRC/executable/script/icon audit.

- `build/release/Slicks` SHA-256:
  `cd579a700f6490e32e61c9832f2bb79aca0649469e26fb696943a3278fe15282`
- `build/release-check/Slicks-0.1.lha` SHA-256:
  `842372b4e148e228660813d210a6bcb0941516ada8a60a9a5952d66c2aba41ff`

This remains a local validation candidate, not the final 0.90 release. Native
workflow validation is separate, and the manual release gate remains open.

## 2026-10-01 — final storage and PCM regression sweep

At 19f7c23, the host suites for setup storage/load, saved-game storage,
track-list storage, track records, capture storage and resource archives all
pass. Coverage includes 2,600 setup transaction cases, 254 setup-load checks,
1,431 saved-game write faults and 2,309 read/truncation cases, the maximum
10,000-track zero-allocation roundtrip, 324 saved-list fault cases, 1,176 cases
each for record publication and track storage, 984 capture faults, and 1,193
cache-construction failures. Reserved archive rereads remain byte-exact and
allocation-free; failed refreshes retain recoverable catalogue state.

The audio suite also passes all 44 startup PCM allocation failures with full
cleanup, alongside sample-bank, one-shot, channel-priority and pitch checks.
These are host adapter tests, not substitutes for the native release workflow.

## 2026-10-01 — strict menu recovery and insufficient-memory checks

- `tmp/standalone-release-8_glb40e`: PLAYERSV on Workbench-loaded stock PAL
  A1200, 2 MiB Chip/no Fast/default 4 KiB stack. The save-failure/cancel/reopen/
  save path passes `--allocation-audit --startup-only`: boundary 125 blocks /
  1,383,692 bytes, 130 startup allocation calls, return 0, no failed or leaked
  blocks, no workspace conflicts and no post-startup allocation calls.
- `tmp/standalone-release-7urdpck5`: the same launcher restricted to 1 MiB for
  a deliberate low-memory failure. Return 20, 19 allocation attempts including
  one failure, zero outstanding blocks. The game never reaches its startup-
  complete checkpoint, as required for a failed reservation. This proves this
  failure path, not every possible OS memory-pressure pattern.
- Both muted test emulators exited. The updated static census classifies all
  34 remaining direct AllocMem expressions, including nine diagnostic-only
  expressions, and distinguishes unlinked imported framework helpers.

## 2026-10-01 — allocation-free copper palette construction

The new `--allocation-audit --startup-only` mode stops on any directly
game-owned Exec allocation after `slicks_diag_startup_complete`, before race
preparation or interactive menus. It still tracks all startup blocks through
shutdown. OS-internal allocations are excluded by caller address.

This caught a real late allocation in `Palette24Bit::setColors`, reached from
`build_copper` during view changes (`tmp/standalone-release-mggqhuvv`). Copper
construction now borrows two 256-colour BSS arrays rather than allocating two
arrays and using a 1 KiB stack table. The synchronous construction has no
overlapping callers; both arrays are reserved by the executable loader.

- `make verify-framework-palette verify-copper-palette` passes: all 256 fades
  at three base colours match the owning implementation, borrowed destruction
  does not free caller storage, and all copper encoding checks pass.
- `tmp/standalone-release-p82od2x4` (CHAMPSAVE) and
  `tmp/standalone-release-jk2szde3` (CHAMPEDIT): Workbench-loaded stock PAL
  A1200, 2 MiB Chip/no Fast/default 4 KiB stack. Both reach the startup boundary
  with 125 live blocks / 1,383,692 tracked heap bytes, then complete with no
  later allocations. Each returns 0 after 130 total allocation calls, no
  failures, no outstanding blocks and no workspace conflicts. These heap
  counts exclude executable/BSS and OS-owned storage.
- `tmp/standalone-release-9vyv_bec` (OPTIONSBC): two-race results/trophy flow
  passes; all 19 full 64,000-pixel chunky/planar publications match.

These tests are complete and their emulators have exited. The broader B12
source/lifetime audit and B11 final release workflow remain open.

## 2026-10-01 — archive directory retained from startup

The first SLICKS.000 index allocation is adopted by a reservation and retained
until shutdown. All later disk opens reread into that exclusive span, without
allocation; the supplied archive has 192 entries requiring 3,648 bytes. Failed
Open/header/index reads release the lease and file handle. Concurrent borrowers
and directory growth beyond startup capacity fail explicitly. Memory-only
cached handles remain independent. Archive diagnostic breakpoints now use the
common open implementation, with its normal stack ABI retained.

- Host adapter: two repeated complete byte-exact archive sweeps using the same
  pointer, zero post-reservation allocation calls, exclusive-lease/destroy
  guards and Open/header/index/oversize failure recovery pass. Existing 182
  resource comparisons, EOF/seek/read checks and 1,193 cache-construction fault
  points pass.
- `tmp/standalone-release-16n202v8`: stock Workbench/68020/2 MiB/no Fast/default
  4 KiB stack, maximal track-list catalogue, REGCHECK allocation audit and two
  same-OS launches pass. First launch: return 0, 200 allocation calls, no failed
  or outstanding blocks and no workspace conflicts. SAME_OS_RESTART_OK=2.
- `...-3m0sgglv`: OPTIONSBC race/results/trophy return flow passes; trophy loads
  once under retained display ownership, and all 19 captured publications match
  every chunky pixel. No test emulator remains running.

Final production-path allocation audit and end-to-end release validation remain
open; these checks establish the directory migration, not completion of B12/B11.

## 2026-10-01 — track-list cache reserves its capacity at launch

The retained catalogue now allocates 65,536 bytes once, before small menu-cache
blocks fragment startup scratch. Refresh reads and validates the separate
modal staging span, then publishes into the reservation without allocating.
Failure retains the previous bytes/view but records an error, preventing stale
browsing. Shutdown frees the reservation even when its first refresh failed.
The unused standalone loader also accepts explicit scratch instead of allocating.

- Host: 324 save fault cases plus read/Close faults, truncation, recovery-file
  retention, reservation failure and cleanup pass. Three repeated full-capacity
  growth/invalid-refresh/shrink cycles preserve pointer identity and bytes;
  cumulative allocation calls do not increase after startup reservation.
- `tmp/standalone-release-1_rqo5tb`: stock Workbench/68020/2 MiB/no Fast/4 KiB
  stack TRACKSL save/reopen/select/race passes; all 16 menu publications match
  all 64,000 pixels. An initial run used the wrong warning fixture and is not
  a pass (`...-h90197pq`).
- `...-0ft1aoym`: maximal 65,528-byte catalogue, same stock configuration,
  REGCHECK allocation audit and two launches in the same OS session pass:
  return 0, 204 allocation calls, no failed or outstanding blocks, no workspace
  conflicts, and SAME_OS_RESTART_OK. The full reservation adds only eight bytes
  over this former exact-size catalogue; an empty catalogue reserves 65,528
  more bytes so future edits cannot require heap growth.
- `...-as2divqy`: TRACKSQ rejects an injected Read failure without publishing
  a partial cache; warning dismissal and race entry pass, with matching menu
  publications. TRACKSC (`...-s2rx71l5`) also selects the scroll-cost diagnostic
  and was stopped explicitly, not counted as a recovery pass. Host tests cover
  Close failures. The warning fixture now observes the actual cache-refresh
  entry point rather than the unused standalone loader.

All test emulators are closed and all file writes used private installation
copies. Archive-directory allocation remains B12 work.

## 2026-10-01 — championship persistence uses reserved scratch

Championship save/load adapters now require caller scratch and never allocate.
The native caller borrows modal storage only after picker/name/confirmation
children close, and releases it before notices. The completed-race VGA track
name array and live intermission parent remain separate. A compile-time bound
proves the full 80,218-byte encoding fits the existing reservation; startup
memory does not increase. Hidden Load remains hidden.

- Host: 1,431 save transaction fault cases and 2,309 read/truncation cases pass,
  including missing/short scratch rejection before I/O, preservation of outputs
  on failure, 300-track capacity guards and a maximum 10,000-track round trip.
  The cumulative allocator-call assertion is zero. Record-storage regression
  also passes both 1,176-case fault matrices.
- Stock Workbench/68020/2 MiB/no Fast/default 4 KiB stack:
  `tmp/standalone-release-pt2ds9gl` (CHAMPSAVE) reaches native intermission,
  picker/name/write and normal exit.
- `...-85f920cv` restarts the same private installation with CHAMPEDIT; resave,
  overwrite/delete acceptance and cancellation, catalogue refresh and normal
  shutdown pass with no workspace conflicts.
- `...-nx9ua32x` (CHAMPSAVF) encounters an intentionally pre-existing private
  E2E.SSS.new directory; warning, picker reopening, cancellation and restoration
  pass with no workspace conflicts. The initial capture-composed fixture
  (`...-i8hg1bcq`) exceeded the debugger breakpoint table before running the
  game and is not counted as a pass; the behavioral fixture rerun passes.

All emulator instances are closed. No original data, private key or runtime
capture is committed; tests changed only disposable installation copies.

## 2026-09-30 — track records use startup-owned scratch

Post-race record input and transactional record writes now borrow 8,192 bytes
from the modal storage reservation. Parsing copies the table before releasing
input storage. Read warnings, record surfaces and save-retry notices therefore
never overlap the lease. Clear Records closes its confirmation dialog, borrows
one span for the entire catalogue, then releases it before opening a notice.
No extra startup memory is required; neither record path has a heap fallback.

- Host adapter: 1,176 single/double-fault publication cases and 1,176 clear
  cases pass. Missing/short scratch is rejected before I/O. A cumulative
  allocator-call assertion proves these paths make zero allocations, not
  merely zero outstanding allocations. Shared capture regression: 984 cases.
- `tmp/standalone-release-j86s98os`: stock Workbench/68020/2 MiB/no Fast/4 KiB
  stack OPTIONSBW read/view failure matrix passes across two races; one read
  rejection and four table-construction failures recover without duplicate
  insertion, workspace conflicts or leaked storage. Restoration mask is 31.
- `...-uh0ss4en`: OPTIONSBX rejected save scratch, Retry, retained table commit,
  second race/results and normal shutdown pass, with zero workspace conflicts.
- `...-623ji566`: OPTIONSBY takes Skip after rejected save scratch; the next
  race, result table, standings and shutdown pass, with zero workspace conflicts.
- `...-xlkvd21w`: OPTIONSR cancels then confirms clearing all 195 tracks;
  all 25 menu publications match all 64,000 pixels and system restoration passes.

The native tests use private data copies, not the user's installation. All
four emulator instances have exited.

## 2026-09-30 — recoverable race failures retain the display (B10 closed)

Race preparation and shop failures no longer restore Workbench. Preparation
closes its I/O window on failure; failure to enter that window is an error,
not permission to tear down and continue. The warning and its dismissal to
the title hold the outgoing bitmap/palette while building the replacement,
then publish together in blanking. Session/configuration rollback is unchanged.

Stock Workbench/68020/2 MiB/no Fast/default 4 KiB-stack checks:

- `tmp/standalone-release-6_ac47xd` (SETUPF): early load failure, warning,
  dismissal, Players and successful retry; session/configuration rollback is
  byte-identical and both warning/title publications match all 64,000 pixels.
- `...-ul63d_r1` (SETUPG): later asset failure, same recovery sequence and
  exact rollback/publications. Failed and successful load windows both retain
  identical bitmap/palette bytes and active ownership with no OS view.
  Final candidate rerun `...-dgcb5h_u` passes the same checks after adding
  SETUPH's diagnostic exit sequence.
- `...-vi_sjyll` (SETUPSC): late shop-constructor failure and the same retry
  workflow; all four captured publications and rollback bytes match.
- `...-fs7qc7ba` (SETUPH): failure, dismissal and ordinary Escape exit;
  restoration mask 31 and both captured publications pass. SETUPH queues
  the final Escape in the diagnostic code. Two earlier debugger-injection
  attempts instead followed the existing retry sequence; those are not exit
  passes and did not motivate a production change.

The source teardown census leaves only `test_pause_children`,
`test_pause_surface`, `test_intermission_surface`, final main cleanup and
the platform destructor's defensive cleanup. The only saved-OS `LoadView`
publication is in platform teardown, not the retained I/O service windows.
All test instances are closed; original data and user installations are untouched.

## 2026-09-30 — reserved setup encoding and retained save recovery

Setup saving borrows 5,771 bytes of modal storage for the maximum CFG/PLR
encoding. The adapter rejects absent/short caller scratch before I/O. The
caller stops audio in blanking, services the save under the retained display,
releases scratch, and closes the I/O window before presentation. A failure
screen is built offscreen; cancellation also holds the warning while drawing
the title. Diagnostic obstruction removal uses the same retained I/O service.

- Host setup transaction suite: 2,600 success/single/double failure cases pass.
  Missing/short setup scratch also passes the production-adapter checks.
- Stock Workbench/68020/2 MiB/no Fast/4 KiB stack:
  `tmp/standalone-release-2wj601la` (PLAYERSV) passes failure, cancellation,
  title return, Players reopen with edits and successful second save.
  Profiles/session bytes match across cancellation; no teardown occurs there.
- `...-0oext16d` (PLAYERST) passes direct failure/retry/save/restoration.
  Each run has three I/O windows (save, diagnostic obstruction removal, retry);
  all six before/after bitmap/palette pairs are byte-identical.
- `...-8w01w588` (SETUPR) restarts the saved data and validates selected profile,
  vehicle, colours and race handoff. An earlier `...-dnnitl9a` native check also
  passed but the harness expected a misspelled marker; the clean rerun passes.

All test emulators closed; no user installation or saves were modified.

## 2026-09-30 — shop capture uses reserved storage and retained display

The shop's original screenshot feature borrows 65,078 bytes from the modal
storage lease while no child dialog is active. The chunky source/parent stay
live and separate. Storage requires explicit sufficient scratch and rejects
missing/short spans before filesystem operations. Release precedes warnings.
Disk I/O services run under the unchanged shop bitmap/palette.

- Host capture-storage suite: 984 single-fault points, no overwrite, retained
  recovery files, and missing/short scratch rejection without I/O pass.
- `tmp/standalone-release-ewthnhdp`: stock Workbench/68020/2 MiB/no Fast/4 KiB
  stack NATURALW, both screenshot writes and ordinary shop/race handoff pass.
  Both windows retain identical bitmap/palette bytes. The two real BMP files
  have exact source pixels, bottom-up rows and the original DAC palette format.
- `...-8ahqcqos`: same private data mounted read-only; the capture warning
  consumes the next key, then the existing shop transaction/Help/race-entry
  assertions pass. The one failed-write window retains bitmap/palette bytes;
  no third BMP is created. Both runs are closed.

No captured images or original data are committed or used as implementation
assets. This preserves the existing user-accessible screenshot feature.

## 2026-09-30 — track records retain display ownership

Tracks' record viewer and Options' confirmed Clear Records now service disk
I/O without restoring Workbench. Warnings, redraws and input run only after
closing the service window, including failed preview reads and partial clears.

Stock Workbench/68020/2 MiB/no Fast/default 4 KiB-stack runs:

- `tmp/standalone-release-6i6aa2yr`: TRACKSV failed Close, warning dismissal,
  retry, reopen and race entry pass. All three I/O windows retain identical
  bitmap/palette bytes; all 12 menu publications match all pixels. Earlier
  `...-7gvo_fhl` passed the workflow before capture guards were added.
- `...-gi1_qj94`: OPTIONSR cancellation then confirmed clearing of the complete
  catalogue passes, with 25 matching menu publications and restoration 31.
- `...-1wgcef6w`: OPTIONSS clears the first track then encounters a private
  pre-existing second-track .new file; partial-failure warning, path notice,
  recovery-file preservation and restoration pass. Files are isolated copies.

Guards forbid teardown while these menus are owned, permit exactly one I/O
window on confirmed clearing, and require active game display, OS I/O service
and null ActiView during record operations. All test emulators closed.

## 2026-09-30 — registration display retention

Registration archive/external-image reads now retain the existing game display
inside I/O service windows. A decoded image is prepared in view 0 while the
outgoing view is held, published black, then faded with the original timings.
Trial-to-title reconstruction also holds the outgoing image. Missing optional
images close their I/O window without exposing Workbench. Only actual shutdown
restores the OS display.

Stock Workbench/68020/2 MiB/no Fast/default 4 KiB-stack evidence:

- `tmp/standalone-release-4jwdqbg8`: REGCHECK, three registration owners close,
  two runtime I/O windows, restoration 31. Before/after displayed bitmap and
  palette bytes are identical for both windows. Guards forbid teardown within
  an owner and require OS service for its runtime file accesses.
- `...-uhr_gl52`: REGCHECKY Help navigation/restoration passes; all six menu
  publications match all pixels, including both restored registration views.
- `...-xvkt7tvk` / `...-z42urjte`: REGCHECKK/L external read/Close failures
  consumed after real I/O, no failed image presented, normal restoration.
  The private fixture uses non-image bytes, never replacement order artwork.

All owned emulators closed. Archive-directory allocations remain B12 work.

## 2026-09-30 — B9: atomic Players/Tracks image and palette transitions

Title-owned Players/Tracks opens and closes copy the outgoing interleaved
bitmap and its actual copper palette to existing view 1, switch to it at
blanking, then rebuild incoming view 0 offscreen. Only a completed image and
palette are published. The retained view cannot be a resumable race: these
four call sites belong to the title-menu lifecycle only. No additional memory
is allocated, no menu dirty-rectangle policy changes, and no disk I/O occurs.

Workbench, stock 68020/2 MiB/no Fast/default 4 KiB stack:

- `tmp/standalone-release-cvyrlxej`, PLAYERSR: two closes/reopen, profile/vehicle
  preservation and race update 200 pass. Four outgoing pixel/palette pairs are
  identical before holding, after copying, and before incoming publication.
  Reopened Players, returned title and race 200 each match all 64,000 pixels.
- `...-w404mx2c`, TRACKS: selection, two closes/reopen and race entry pass.
  All four retained pixel/palette pairs match, as do all nine captured menu
  publications. Guards require the held view to remain shown while view 0's
  palette is rebuilt and forbid display teardown during a transition.
- First composed Players fixture (`...-h89vgjsv`) failed because two debugger
  breakpoints shared the race-start address; not counted as a workflow pass.
  The helper fixture no longer duplicates that breakpoint. The old handoff
  fixture also now disables its actual breakpoint number, not hard-coded 1.

Both passing test sessions closed. B9 is complete; other disk/lifetime and
release gates remain separate open work.

## 2026-09-30 — reserved trophy staging and retained trophy I/O

Championship results borrow the modal storage lease for the 64,003-byte trophy
image, release it before constructing the results surface, and load through
begin_io/end_io instead of restoring Workbench. Failed loading also closes
the archive, releases the lease and closes the I/O window before returning.
The original fade to black remains; this is not a new blank/loading screen.

`tmp/standalone-release-wbbjz31a`: Workbench, stock PAL 68020/2 MiB/no Fast,
default 4 KiB stack, OPTIONSBC / diag_championship_rectangles passes two-race
results-to-title flow. The strengthened resident assertions forbid teardown
throughout the trophy owner and require active display, I/O service and null
OS ActiView during archive access. All 49 captured publications (including
both trophy bitmaps) match their 64,000 chunky pixels. Emulator closed.
Archive directory allocation remains B12 work.

## 2026-09-30 — reserve the large menu block before cache fragmentation

Move the unchanged 96,546-byte menu reservation to immediately after release
of startup title/sample staging, before the many small resource-cache blocks
and retained track-list catalogue. No capacities or total resident bytes change.
The earlier allocation order failed this block in the maximal-catalogue test
(`...-5fvknh2m`, above); the new order passes with the same catalogue.

- `tmp/standalone-release-8y5wb38r`: maximal 65,528-byte catalogue, Workbench,
  stock 68020/2 MiB/no Fast/default 4 KiB stack, REGCHECK full allocation audit:
  return 0, 200 allocations, zero failures, zero outstanding blocks, no
  workspace ownership conflicts.
- `...-uifdycmj`: repeat maximal-catalogue launch, registration Help and system
  restoration pass (REGCHECKY). Both owned emulator sessions closed normally.

This fixes the reproduced launch reservation failure for these configurations;
it does not establish arbitrary Workbench free-memory headroom or complete B12.

## 2026-09-30 — registration decode staging reservation

Registration BMP decoding now leases 70,000 bytes from the startup-reserved
modal storage overlay. It releases the lease immediately after decoding,
before fading, input or registration Help, and on every failed load/decode.
No new startup bytes are required; archive-directory allocation and display
teardown are still separate B12/B10 work.

- Stock Workbench/2 MiB/no-Fast/4-KiB-stack REGCHECKY passes registration Help
  opening, navigation, closure and restoration in
  `tmp/standalone-release-zs5bawb5`. Before/after Help chunky images are identical.
- Its full allocation audit (`...-nh7zyz4d`) passes cleanup and modal ownership.
- The maximal-catalogue repeat (`...-5fvknh2m`) fails the startup 96,546-byte
  menu reservation, unlike the earlier run that reached registration. It has
  zero outstanding blocks and zero ownership conflicts, but is not a launch
  pass. Startup headroom/allocation layout remains unresolved; do not infer
  robust fit from the earlier single successful reservation.

## 2026-09-30 — retained track-list I/O and reserved transaction scratch

Track-list save/delete and refresh use display-retaining OS-service windows.
All rendering occurs outside them; modal children and borrowed catalogue views
close before scratch acquisition/refresh, and warnings open after release.
Store/refresh take explicit bounded scratch from the startup particle-cache
modal overlay with a separate storage owner. No heap fallback or additional
startup bytes are added. Retained catalogue replacement still allocates.

Workbench-loaded stock 2 MiB/no-Fast/default-4-KiB-stack evidence:

- Before scratch conversion, TRACKSL (`...-24neo_sc`) selected indices 0/1
  correctly but failed the late 65,536-byte save allocation (DOS error 103).
- After conversion, TRACKSL (`tmp/standalone-release-_1p0svk5`) passes real
  save/selection/race entry, with all 16 publications matching every pixel.
- TRACKSD (`...-5g25g08z`) passes name/delete cancellation, confirmed deletion,
  empty-catalogue reopening and race entry; all 26 publications match.
- TRACKSF (`...-nuu6gjbk`) mounts the private data volume read-only: real DOS
  write failure, warning dismissal, requester restoration and race entry pass;
  all 18 publications match. The standalone harness now exposes --read-only.
- Host production-adapter suite passes 324 single/double save faults, refresh
  faults/retention, recovery artifacts and cleanup. Missing/short caller scratch
  is rejected before filesystem work, without changing the retained catalogue.

Maximal-catalogue Workbench check (`...-oh94vx80`) still exits early. The first
TRACKSN audit (`...-9w_8ypd4`) has no scripted exit after race entry and was
closed, not counted as a cleanup pass. REGCHECK audit (`...-rye3zbbz`) identifies
a later 70,000-byte registration-image allocation failure, after both the
65,528-byte catalogue and 96,546-byte menu reservation succeed. It reports
return code 20, 135 allocations, one failure, zero outstanding blocks and zero
workspace conflicts; its expected-normal-exit assertion fails. This remains
B12 work, not a maximal-catalogue pass. All owned emulators are closed.

## 2026-09-30 — retained display through championship save-file operations

The saved-game dialog uses OS-service windows for existence checks, save/load,
delete and catalogue refresh. Each window closes before confirmation, warning,
picker rendering or input resumes. Error cleanup also closes a pending window.
The saved-dialog guard now requires display-owned I/O rather than full teardown;
startup catalogue enumeration remains an inactive-platform operation.

Workbench-loaded stock 2 MiB/no-Fast/default-4-KiB-stack evidence:

- CHAMPEDIT (`tmp/standalone-release-dtsol4tr`): creation, overwrite/delete
  acceptance and cancellation, catalogue refresh counts and clean exit pass.
  No full teardown is allowed while the saved-dialog owner exists.
- CHAMPEDIT capture gate (`...-oefrwh1p`): six I/O windows, three dialog returns.
  All displayed bitmap/palette snapshots compare byte-for-byte unchanged;
  OS ActiView stays null and display ownership remains active during I/O.
- CHAMPSAVF (`...-1d_kmx28`): a directory deliberately obstructs E2E.SSS.new in
  a fresh private fixture. Real save failure, warning, picker reopening,
  cancellation, requester-pointer restoration and normal exit pass. This is
  a transaction-path obstruction test, not a read-only-volume test.

All debug emulators were muted and closed. Hidden Load remains unreachable in
the normal menu; its shared I/O branch was converted but has no renewed native
Load coverage. Remaining B10 boundaries include track-list transactions,
track-info/record-clear operations, screenshot writes, ending/registration
images, setup-save failure recovery and nonfatal race-preparation failures.

## 2026-09-30 — retained display during intermission preview reads

Intermission no longer ends display ownership to load DAT/next-track preview
data. Paula is stopped by the caller; only the two reads run inside an OS
I/O window. Cached resources and all rendering run outside that window.
Retry notices and the completed intermission are built in view 0 while the
unchanged race in view 1 remains visible, then shown at display blanking.
Failure cleanup closes any outstanding I/O window.

Workbench-loaded stock 2 MiB/no-Fast/default-4-KiB-stack evidence:

- OPTIONSTL display guard (`tmp/standalone-release-i59frtuk`): no full teardown
  across intermission; failed Close and retry produce two I/O windows. Both
  retain active ownership/null OS ActiView; each displayed 64,000-byte bitmap
  and 768-byte palette compares identical before/after.
- OPTIONSTI (`...-0xpu4_id`): repeated edits, proper vehicle handoff to the
  next race and restoration pass. All 55 menu publications match every pixel.
- OPTIONSTL behavioural gate (`...-i11e8gql`): failure/retry, rewards-once and
  second race pass. Warning and race return match all pixels; race bitmap
  and retry session snapshots are unchanged.
- `...-201m97d1` exited before debugger connection and is not a test pass.

Each test began with a fresh Classic CFG. All debug emulators were muted and
closed. This does not yet cover intermission save-file I/O or ending images;
those other disk boundaries remain B10 work.

## 2026-09-30 — startup-backed intermission staging and save names

Normal intermission borrows the completed race's idle VGA allocation for DAT,
track and language staging, then reuses the same union for exported save names.
The preview's decoded output and compressed inputs are disjoint. The retained
parent copies labels before the save-name phase; nested dialogs use their
separate particle-cache slot. No startup reservation grows. The only remaining
allocating plain-file-loader callers are explicit construction diagnostics.

Workbench-loaded stock 2 MiB/no-Fast A1200, default 4 KiB stack:

- OPTIONSTI (`tmp/standalone-release-lk376rjj`): repeated intermission edits,
  second race and all 25 complete pixel publications pass.
- CHAMPSAVM (`...-ww9ew0wf`): reserved save-name rejection, warning, retry,
  picker/name acceptance and real save/exit pass; all 25 publications match.
- CHAMPEDIT (`...-95dip7ws`): resave, overwrite/delete acceptance and
  cancellation, catalogue refresh and clean exit pass.
- OPTIONSTL (`...-4o6mspsq`): actual preview-file Close failure, warning,
  retry and next race pass; rewards occur only once. Retry session snapshots
  and retained race bitmap match byte-for-byte. Warning and returned race
  independently decode to all 64,000 expected chunky pixels.
- CHAMPEDIT allocation audit (`...-ck9luqsn`): 166 allocations across startup
  and the whole workflow, zero failures and zero outstanding blocks on exit.
  Persistence allocations remain; this is not a startup-only audit pass.

The initial retry run `...-do_65r2p` failed its pre-intermission playlist
assertion; save runs `...-9cbfrwxj` and `...-hmugipb7` were stopped. Their shared
input CFG had retained Arcade mode from a prior test, whereas these ordinary
key scripts assume Classic's title rows. Fresh Classic fixtures created with
title_return_test_config resolve this; do not count the stopped runs as passes.
All owned debug emulators are closed. B9–B12 remain open as scoped in open-work.

## 2026-09-30 — retain the displayed image during record I/O

Post-race record reads, retries and writes now use the platform's disk-service
window instead of full display teardown. The caller stops Paula first; every
window ends before any menu input, rendering or warning. Cleanup closes an
outstanding window before releasing the records owner.

- `tmp/standalone-release-qrv35cpx`, OPTIONSBC with `diag_records_io.gdb`:
  two race returns and five I/O windows pass on Workbench-loaded stock 2 MiB,
  no Fast RAM, default 4 KiB stack. Independent dump comparisons verify all
  64,000 displayed bytes and 768 palette bytes unchanged across each window;
  both race buffers also restore exactly, with independently decoded planes.
- `...-ik_pangu`, OPTIONSBC with the full standings fixture: read Close failure
  and retry, write failure and retry, two record insertions/returns, standings,
  profile persistence and system restoration pass. Both race return images
  match. The generic first-record saved-file checker is not a pass here: the
  first track did not take the write-recovery path; the write warning occurred
  on the second track. Do not infer saved-byte coverage from that mismatch.
- Full teardown is forbidden for the complete records-owner lifetime. Each
  record write must have active retained display ownership, OS I/O enabled and
  a null OS ActiView. The separate capture fixture avoids exceeding FS-UAE's
  breakpoint capacity; two earlier over-capacity runs never began the test.

All owned emulators were muted and closed. This is a scoped B10 improvement,
not completion: intermission, ending and other disk boundaries remain open.

## 2026-09-30 — startup-owned retained dialog parents

Pause, intermission and track-list parents use a distinct owner-tagged union
alongside the primary menu. The combined startup allocation is 96,546 bytes
(8,588 more than before); the child overlay stays 110,096 bytes. Parent failure
and close paths release the slot, and primary release rejects a live parent.
The menu module now has just one AllocMem at startup and one FreeMem at shutdown.

- Workbench-loaded 2 MiB/default-stack LIVEMENUH (`...-kav4unzr`): pause/Help/
  reopen/resume passes; ten complete pixel publications pass.
- Same configuration OPTIONSTI (`...-4bgbg5ow`): repeated intermission edits
  and second race pass; 55 publications pass.
- 2 MiB without Workbench, maximal catalogue TRACKSN (`...-63gvu8n7`): both
  picker faults, recovery and race pass; 18 publications pass.
- Workbench-loaded OPTIONSTJ (`...-ormdsevw`): intermission constructor
  rollback/retry and two races pass with rewards applied only once.
- Workbench-loaded LIVEMENUH allocation audit (`...-_q7cq3rq`): 139 allocation
  calls, zero failures, zero outstanding blocks and zero ownership conflicts.

All test emulators closed. Catalogue/persistence and non-menu staging remain
B12 work; this does not claim their combined reservation is finished.

## 2026-09-30 — remaining leaf dialogs and icon staging

Track Information, Change Cars and messages use the existing owner-tagged modal
union. Icon decode input uses the spare primary saved-page tail, not its live
64,000-byte parent image. Neither startup reservation grows. Close/failure
restores pixels and font state before releasing the overlaid cache.

Stock 2 MiB A1200 with Workbench/default 4 KiB stack:

- `tmp/standalone-release-fz2lljq6`: TRACKSK passes all five faults,
  dismissal/retry, reopen and race entry; all 30 publications match chunky.
- `tmp/standalone-release-90o35qu6`: OPTIONSTI passes repeated intermission
  car edits and second-race entry; all 55 publications match chunky.

The three retained parent objects and persistence/ending staging remain B12
work. These tests do not claim the combined startup budget is finished.

## 2026-09-30 — reserved picker and name/index payloads

The three picker constructors borrow the existing modal slot, with a separate
aligned payload inside that union. Its 5,698 bytes cover the maximum validated
catalogue offsets and 40 saved filenames. Total modal size remains 110,096.
No picker or payload FreeMem remains; failure hooks still reject acquisition
and preserve their error reports.

- `tmp/standalone-release-wndnhdsu`: PLAYERSK accept/reopen/cancel passes on
  Workbench-loaded 2 MiB/default stack; all 12 publications match chunky.
- `...-r5r89k83`: maximal synthetic catalogue, TRACKSN, both acquisition
  failure boundaries, dismissal/retry and race pass; all 18 publications
  match. This is 2 MiB **without Workbench loaded**, not a Workbench pass.
- `...-3e17051f`: CHAMPSAVE passes real intermission, picker/name/write and
  clean exit with Workbench, stock 2 MiB/default stack.
- Host list dialog and renderer suites pass (172,032 key/state cases and
  88 complete restored rendering cycles, plus initialization/drawing/pulse).

Earlier PLAYERSP runs paired the wrong input script with the PLAYERSK fixture;
they are not regression passes. The fixture now prints its failed values.
The maximal catalogue with Workbench fails startup menu reservation (the user
screenshot); an allocation audit (`...-728xeuoj`) instead fails the 65,528-byte
retained catalogue allocation and continues to a warning. That waiting audit
was closed, not counted as a cleanup pass. Both outcomes remain evidence for
B12's missing combined startup reservation, not permission to ignore it.

## 2026-09-30 — shared startup modal storage

Help, Controllers, name entry and colour picking now share an owner-tagged
union in the existing particle-cache overlay. Target size remains 110,096
bytes: no extra startup memory. Failure, close and parent-destroy paths release
the correct owner and rebuild the cache before resuming simulation. Three more
normal-game allocation sites are removed; the remaining lifetime matrix is in
allocation-inventory.md.

Workbench-loaded stock A1200, default 4 KiB stack:

- PLAYERSNL (`tmp/standalone-release-7jfywbai`): post-paint name failure,
  warning/retry, create/edit/reopen/cancel; 30 full-screen publications pass.
- PLAYERSCL (`...-92w7rh7z`): colour failure/retry and both endpoints,
  acceptance/cancellation/reopen; 44 publications pass.
- OPTIONSC (`...-uhf_bxpi`): capture, defaults, reopening/re-edit;
  38 publications pass.
- LIVEMENUH (`...-vea_gnvr`): Help contents/history/reopen/resume passes;
  ten publications pass and particle-cache/car snapshots match byte-for-byte.
- LIVEMENUH cleanup audit (`...-wn0momk3`) passes with no failed or outstanding
  allocations and no ownership conflicts.

The separate OPTIONSC allocation audit (`...-8s8yxslf`) was interrupted after
its scripted input ended without exiting. It is not a cleanup pass; its
functional/rendering fixture above passed independently. Test emulators closed.

## 2026-09-30 — track-information preparation uses startup VGA storage

Removed the preview arena and compressed DAT/track allocations from
`open_track_info`. Its synchronous preparation uses disjoint ranges of the
idle VGA image, not the current chunky screen or saved parent. The bounded
loader preserves the previous strict file-length, exact-read and Close checks.
Tracks exit rebuilds the title image before using VGA again.

Stock Workbench-loaded 2 MiB/default-4-KiB-stack checks:

- `tmp/standalone-release-0b0h8onh`: all five injected failures, dismissal,
  successful reopen/animation and race entry pass; 30 full-screen publications
  match chunky pixel-for-pixel. This same test previously failed before the
  first injected dialog boundary (`...-08gqyfjb`).
- `tmp/standalone-release-mlnjfd84`: failed disk Close, warning dismissal,
  retry/reopen and race entry pass; 12 full-screen publications match.

This removes preparation allocations only, not the remaining dialog allocation.

## 2026-09-30 — startup-owned nested Help overlay

Help's viewer and parent snapshot now use an exclusive 110,096-byte overlay
inside the already allocated 117,760-byte particle visibility cache. No new
permanent allocation is added and Help no longer makes either of its former
46,096/64,000-byte allocations. On close, failure or parent destruction, its
release callback rebuilds the cache if a race has started; title-start Help
does not read uninitialized terrain. Simulation never runs while modal Help
owns the overlay. Capacity and ownership are checked; there is no heap fallback.

- `tmp/standalone-release-7q2u2u7o`: the previously failing Workbench-loaded
  stock-2-MiB shop fixture passes all 12 construction failure cases, Help,
  purchases/sales, driver switching and race entry. All ten full-screen
  publications match their chunky images.
- `tmp/standalone-release-bc8mawbt`: LIVEMENUH passes pause, Help contents/history,
  close/reopen, resume and normal exit on stock/default-stack configuration.
  Before/after particle visibility, cars and the restored race image compare
  byte-for-byte equal. All ten full-screen publications match their chunky
  images. Workspace ownership conflicts remain zero.
- `tmp/standalone-release-0d0h8lxk`: the LIVEMENUH allocation audit passes
  with 140 allocations, zero failed allocations, zero outstanding allocations
  on exit and zero workspace ownership conflicts.
- `make verify-particle-draw`: full-byte map rebuilds, 524,288 native visibility
  cases, 4,096 single and 256+256 ordered/actor-chain drawing cases pass.

Other nested-dialog and persistence allocations remain on B12; this is not a
claim that all game-owned runtime allocations have been removed.

## 2026-09-30 — reuse primary save-under during construction

The startup menu reservation grows by 1,536 bytes to 87,958 bytes. Before a
parent snapshot exists, its 65,536-byte save-under stages font/image resources
for Help/race surfaces, Players, Options, Tracks and shop. No constructor frees
borrowed bytes. Nested dialogs retain their parent snapshots unchanged.

Workbench-loaded stock/default-stack runs pass Players (`...-dzvokv8z`), Tracks
(`...-3o7ofzlp`) and Options (`...-fpttbhnn`), including 6/9/10 full-screen
chunky/planar publication comparisons respectively. Shop's 12 construction
failure/cleanup cases pass (`...-k1etjlhn`), but its full Help interaction does
not: allocation-return tracing (`...-t6jv_3a3`) identifies the separate 64,000-byte
nested Help save-under allocation failing. Do not count that full workflow as
passed. It remains part of the startup-memory migration; no expectation was
relaxed. The general shop diagnostic now prints which input checkpoint failed.

## 2026-09-30 — remove redundant track-list refresh staging

Refresh now decodes into its own unpublished staging instead of invoking a
loader that allocates another 65,536-byte buffer. The public load API still
preserves caller output on failure; cache replacement remains publish-on-success.
The host suite passes all 361 single/double save faults, malformed/truncated
loads, recovery guards, cache refresh failures and preservation/cleanup checks.

`tmp/standalone-release-ukicjoe_` passes the full CHAMPSAVE picker/name/write/
exit fixture on a Workbench-loaded 2 MiB A1200/default 4 KiB stack, with the
startup menu/track reservation. The earlier `...-7yx9tztp` used an existing
E2E.SSS from a prior fixture, took overwrite behavior, and failed the fixture's
expected new-file catalogue-refresh count; the successful rerun used a fresh
private copy of the manual installation, not altered test expectations.

The preceding same-session DEMOPLR audit (`...-bwsjvboo`) also completed its
second launch: both exits leave 1,891,712 free Chip bytes and a 1,890,752-byte
largest block. Neither original manual data nor unrelated emulators were changed.

After removing redundant staging, `tmp/standalone-release-ij4hjc32` passes
the full DEMOPLR allocation/cleanup audit with no failed allocations, no
outstanding allocations and no workspace ownership conflicts. Directly tracked
allocation high-water is 1,337,672 bytes; executable/OS memory is not included.

## 2026-09-30 — reserve primary menu/track workspace at startup

The common 86,422-byte menu owner is now allocated once before entering the
title/game loop. Track preparation borrows that exclusive slot after shop
destruction for DAT, track, navigation, car and font staging. It fits by static
assertion. Acquisition cannot allocate a fallback block; menu initialization
clears the reused bytes, and shutdown releases the reservation once.

`tmp/standalone-release-i_kmktd6` passes the Workbench-loaded stock-2-MiB
intermission/repeated-edit/second-race fixture and all 55 whole-screen
chunky/planar publication comparisons. The first audited DEMOPLR execution in
`tmp/standalone-release-bwsjvboo` returns zero after both demos and Players,
with 150 allocations, zero outstanding allocations and zero workspace ownership
conflicts. One recoverable 65,536-byte allocation still failed inside
`slicks_amiga_load_track_lists`: the redundant second refresh buffer. This is
not evidence that all runtime allocations have been removed. The same-session
relaunch portion is being checked separately.

## 2026-09-30 — startup-owned intermission preview workspace

Intermission now borrows the dead VGA race image for its 64 KiB preview decode
workspace. No late arena allocation/free remains in this path. The next race
or title rebuilds the VGA image; the saved parent and visible chunky image are
not borrowed. API ownership is explicit and injected failure paths remain.

The normal build and focused intermission menu/preparation/renderer host suites
pass. Workbench-loaded stock PAL A1200/default-4-KiB runs pass:

- `tmp/standalone-release-zw_ncvtf`: nine Change Cars inputs, edited vehicles
  reaching the second race and normal system restoration.
- `tmp/standalone-release-tny86vq2`: same sequence plus 55 menu publications;
  all 64,000 decoded planar pixels match chunky at each publication.
- `tmp/standalone-release-8nxj41zw`: CHAMPSAVE reaches real intermission,
  cancels/reopens the picker, names/saves the three-track championship and
  exits with `NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`. The host launcher was
  accidentally given the shorter, nonexistent success marker and therefore
  reported an assertion after the GDB fixture passed; the saved diagnostic
  log contains the actual expected fixture success marker.

These are automated checks, not claims of manual observation or proof that all
runtime allocations are eliminated. The broader audit and remaining migration
are in [memory-lifetimes.md](memory-lifetimes.md).

## 2026-09-30 — manual release retry, in progress

In the prepared stock PAL A1200 session (68020, 2 MiB Chip, no Fast RAM,
standalone release executable, default 4 KB stack), the user confirmed that
Players opens and returning to the main menu succeeds. This clears the
previously reported exit on that transition in this manual retry.

The user also observed that Escape from Players applies the main-menu palette
immediately, leaving the outgoing image garbled for over half a second before
the main-menu image appears. Recorded as B9; fix deferred until after this
manual session at the user's request. Race/save/exit and WHDLoad checks remain
pending.

The user subsequently reached the post-race results screen (local screenshot
`FS-UAE_Full_260930-2039_00.png`). Workbench was briefly visible before the
results appeared; the exact disk operation has not yet been identified.
Race completion is observed; championship save, normal exit and WHDLoad
checks are still pending. The requested preservation of the current game
display during disk access is tracked as B10.

After dismissing the results, the user reached `ENTER: RETRY / ESC: END MATCH`
(`FS-UAE_Full_260930-2040_00.png`). Escape then reached the trophy/final
standings screen (`FS-UAE_Full_260930-2041_00.png`), again exposing Workbench
briefly first. This is a second observed B10 transition. This match ended
without reaching a between-races save menu; manual championship saving remains
untested.

Correction after inspecting the prompt's source: `ENTER: RETRY / ESC: END MATCH`
is exclusively the intermission failure/retry notice, not a normal match-end
prompt. The earlier guidance to end the match misidentified it. Subsequent
screenshots show 195/195 selected tracks and Classic mode with five laps.
Intermission entry therefore failed; its exact failed operation remains to be
diagnosed (B11). Race completion passed, but intermission did not.

The user then confirmed a clean normal exit to Workbench. Standalone manual
exit passes; championship saving remains blocked by B11, and both WHDLoad
manual passes are still outstanding.

Isolated `OPTIONSTI` reproduction with Workbench loaded and the default 4 KB
stack reaches the same intermission failure (`tmp/standalone-release-j8z09nkk`).
An optimized source-line breakpoint run was inconclusive and stopped.
Exec AllocMem/return breakpoints in `tmp/standalone-release-vcndob3s` identify
the exact failure: the 65,536-byte preview arena in
`slicks_amiga_intermission_open` returns null. The 41,500-byte SLICKS.DAT,
2,011-byte BASICTRK.SS, menu surface and dialog were already allocated.
This is evidence for peak-memory pressure, not another demonstrated leak.
Tests used a separate copy of the installation and did not modify the manual
session's data. The completed manual emulator and diagnostic sessions were closed.

## 2026-09-30 — allocation ownership and default-stack lifecycle

The framework allocated its 24,577-word blitter queue unconditionally in a
startup constructor, even though Slicks builds without `USE_BLITTER_QUEUE`.
Its raw static pointer had no destructor or explicit shutdown free. The old
release audit `tmp/standalone-release-4781m6ij/debug.log` confirms exactly
49,158 requested bytes outstanding after returning to DOS. In the same
Workbench-loaded 2 MiB test, a menu-surface request of 86,422 bytes failed.
That run's attempted debugger-written stack watermark is **invalid**: this
FS-UAE build does not reliably apply target-memory writes. It is not evidence
of stack overflow.

The queue allocation is now conditional on actually using queued blitting.
There is also an explicit final shutdown release for builds that do enable it.
The platform's existing takeover, copper and bitmap implementation is unchanged.
The fix saves roughly 48 KiB during play and avoids leaking it at every exit.

`tools/memory_audit.py` observes real Exec AllocMem/FreeMem calls from before
constructors until return to DOS. It checks ownership and exact free sizes,
including allocations made by C++ new. It uses a host-side debugger driver,
not a tracking allocation on the Amiga, and does not write game memory.
The native DEMOPLR fixture completes two demos naturally, opens/closes Players
through ordinary input events, and exits. The previous debugger-injected input
attempts were discarded, not counted as passing evidence.

Passed local fixtures (`tmp/standalone-release-*`):

- `jsskk5u2`: normal build, Workbench loaded, 2 MiB Chip/no Fast, 4096-byte
  Shell stack, two natural demos then Players; 161 allocations, zero failures,
  zero outstanding at successful exit.
- `ke0l7jnf`: deliberately restricted 1 MiB Chip, genuine allocation failure;
  return code 20, 19 attempts/one failure, zero outstanding allocations.
- `x4vdm7sd`: native STACKCHECK watermark, same demo/Players sequence; 692
  bottom-of-stack bytes remain untouched, zero outstanding allocations.
  One optional 64 KiB request fails and its normal fallback succeeds.
- `ek3ac_fx`: STACKCHECK, DISPMEM's ten display-construction failure boundaries
  followed by demo/exit; 692 untouched stack bytes. Three direct launches in
  the **same** Workbench session all return successfully. After each, Avail
  FLUSH reports exactly 1,891,720 free Chip bytes and a 1,890,792-byte largest
  block. No accumulating leak or fragmentation is observed.
- `vjqrkoq0`: restored normal build, two complete DEMOPLR launches in the same
  Workbench session. The first audit confirms Players stage 4, two natural demo
  returns, zero demo-state errors and zero outstanding allocations. Both
  launches return successfully; free Chip bytes/largest block are the same
  1,891,720/1,890,792 after each. No stack instrumentation is in this binary.

STACKCHECK=1 is diagnostic-only and writes its watermark natively before main;
it neither extends the stack nor allocates a replacement. Mode stamps force
recompilation when toggling it. These are measured exercised paths, not a
mathematical maximum for every possible input. Debuggers and emulators used
for these completed tests were closed. All runs were muted.

Repeat after building/copying the matching executable into a private install:

```
. amiga/env.sh
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMOPLR --allocation-audit --audit-players --marker ALLOCATION_CLEANUP_OK
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DISPMEM --allocation-audit --repeat 3 --marker ALLOCATION_CLEANUP_OK
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMORET --allocation-audit --chip-memory 1024 --expect-failure --marker ALLOCATION_CLEANUP_OK
```

For the watermark, build `make -C amiga STACKCHECK=1`, run with the matching
binary, then restore `make -C amiga STACKCHECK=0` before release packaging.

The clean-tree `make release-check` gate passes at `fe94b50`, including all
host reference comparisons, the 197-file extraction/preservation/rejection
tests, two identical clean stripped builds and the 11-member LH5 audit.
Log: `tmp/release-gate-memory-fix.log`. The normal stripped executable is
455,104 bytes, SHA256
`be8071c8df2f9449759db8e14b7a7dbf6b2e51c93d780b6e7ae7e6264e7dcec7`.
`build/release-check/Slicks-0.1.lha` is 281,331 bytes, SHA256
`8c4b0861ccd678a64e2f6706d7932c68f95b04babe67de03570fdd8b992d3ee8`.
This is a validation candidate, not the final 0.90 release. The prepared
manual-retry executable compares byte-for-byte with the packaged executable.
The exact archive also passes real Intermediate-level Installer tests:
`tmp/installer-script-i4r6c_5n` (Use existing, stock 2 MiB) and
`tmp/installer-script-m6rzumtt` (fresh extraction through RAM-backed T:,
2 MiB Chip plus 4 MiB Fast). Both verify original data, expected binaries,
absence of Play, preserved user files where applicable, and staging cleanup.

## 2026-09-30 — direct executable launch and RAM temporary directory

Removed the Play script from the package and installer. Upgrades also remove
the obsolete installed script. Standalone instructions now run `Slicks` from
the data drawer, without increasing the Shell stack. The existing template icon
already sets `MINUSER=AVERAGE` (Intermediate); a new archive assertion protects
it. The earlier manual fixture incorrectly overrode that setting with NOVICE
on its command line. Installer test launches now use AVERAGE explicitly.

Real Installer tests passed using the updated sources:

- `tmp/installer-script-zoncd5dx`: stock 2 MiB/no Fast, Use existing preserves
  modified tracks and user-file placeholders, removes legacy Play/icons, and
  never asks for ZIP or scratch storage.
- `tmp/installer-script-ms9rlngg`: 2 MiB Chip plus 4 MiB Fast, fresh extraction
  through `T:` assigned to `RAM:T`. All 197 original files match and temporary
  staging is removed. RAM-backed temporary storage is supported, not prohibited;
  on a stock 2 MiB system disk scratch remains the memory-saving recommendation.
- `tmp/installer-direct-candidate/Slicks-0.1.lha`: independent audit passes with
  exactly 11 allowlisted members and no Play script (281,069 bytes).

These packaging checks do **not** close the manual release gate. The user saw
the menu, completed an idle demo, then selecting Players exited and relaunch
failed. Memory cleanup and direct 4 KiB-stack lifecycle verification are still
under investigation; neither success nor a memory-leak diagnosis is claimed.

## 2026-09-30 — WHDLoad-only icon and existing-data installer workflow

At the user's request, the installer now follows the WHDLoad Install Template
and Vette conventions: mandatory WHDLoad path check, standard `Slicks.info`
project icon with `Slave=Slicks.slave` and `PreLoad`, and no optional-launcher
question or standalone icon. The uniconed `Play` script remains usable through
`Execute Play`. Upgrades remove only the old `Play.info` and
`SlicksWHDLoad.info`; data, keys and saves are not deleted.

The default `Use existing` choice skips both ZIP and scratch questions when
the two original files and TRACKS drawer are present. `Reinstall` refreshes
the publisher's files without removing user settings, profiles, championships,
keys or custom tracks. The whole-drawer deletion prompt remains deliberately
absent to protect those files.

The exact candidate `tmp/installer-whd-candidate/Slicks-0.1.lha` passes the
independent 12-member LH5 package/CRC/source/version audit. It is 281,199 bytes,
SHA256 `88bfe7925c6bb1fab2a57755e37055dbfeb2ed535149c413273e0d36f4b7f875`.
The native executable remains byte-identical to the D-1/D-2 candidate; the
change is confined to the installer, documentation and installer tests.

Real Installer checks using this archive pass on 2 MiB Chip/no Fast:

| Case | Local fixture under tmp/ | Result |
| --- | --- | --- |
| Use existing | installer-script-nba46zx_ | ZIP/scratch branches would abort if reached; neither is reached. Modified records and all user-file placeholders survive; old icons disappear. |
| Fresh install | installer-script-c5bs1bdi | All 197 original files, native executable, slave, single WHDLoad icon and staging cleanup match. |
| Reinstall | installer-script-n8yr4d5j | Original track restored; configuration, profile, championship, key placeholder and custom track survive. |
| Missing TRACKS drawer | installer-script-r1krdmnq | Incomplete installation is repaired by extraction, not falsely reused. |
| Missing WHDLoad | installer-script-v8ri8nfn | Prerequisite failure branch reached before installation writes; fatal requester replaced with a marker and quiet exit for unattended testing. |

The user also supplied a screenshot of the unmodified fatal requester from
the deliberate missing-WHDLoad fixture `installer-script-w5jfmvu8`, confirming
the expected visible error. That fixture was closed rather than left waiting
for dismissal. All other test emulators closed automatically. The earlier
manual installer session was stopped because its workflow was superseded;
D-3 still requires a new manual session, now using Execute Play for standalone.
No private key was used, no release tag was made, and `dist/` was not replaced.

## 2026-09-30 — ship gate D-2 passed

The exact stripped D-1 candidate passes all three full-frame display audits
with `SLICKS_LIVE_STATS=0`: F1, CITY and WHACKO each cover 600 updates, with
32/18/5 actors and 2068/1480/1854 marks respectively. The debugger uses the
preserved matching release ELF, not a subsequently rebuilt diagnostic ELF.

After the diagnostic-only allocation repair in 68efb9c, all four RETCHECK
fixtures pass on PAL A1200 with 2 MiB Chip and no Fast RAM:

| Track | Race comparisons | HUD comparisons | Geometry comparisons |
| --- | ---: | ---: | ---: |
| BASIC | 603 | 700 | 0 |
| F1 | 603 | 700 | 575 |
| CITY | 603 | 700 | 603 |
| WHACKO | 603 | 700 | 0 |

Every surface, particle, immutable-map, HUD and geometry mismatch counter is
zero. Zero geometry counts on BASIC/WHACKO are not claimed as geometry coverage;
F1 and CITY exercise that path. Logs: `tmp/release-render-73f9c03/display-*.log`,
`retention-*.log`, `snapshot-host.log` and `retention-remaining.log`.

The normal build was then rebuilt cleanly and stripped. It is byte-identical
to the preserved D-1 candidate, SHA256
`a25d8cc46aa4f64a2fa807ae7a02d2bfb0529731440cd7f44411161aea6888e8`.
Thus the diagnostic repair does not change the already audited release payload.
All automated emulators were muted and closed. D-2 is complete; the actual
manual Installer/Play/gameplay/exit and WHDLoad session is still D-3, not covered
by these automated results. No archive was published and no version tag made.

## 2026-09-30 — retention diagnostic allocation repair

The release-gate RETCHECK run initially reached 700 updates with zero actual
comparisons. Its 129,748-byte contiguous state allocation failed: after the
64,000-byte surface allocation, 134,472 bytes remained free but the largest
block was only 128,840 bytes. Reserving the state earlier instead prevented
startup's menu cache from fitting, so that attempt was rejected.

The optional diagnostic now stores the same mutable prefix in independently
allocated 1 KiB blocks, with an exact-sized final block. It still keeps all
production assets resident and hashes the three excluded immutable maps.
Allocation failure exits the diagnostic rather than silently bypassing its
comparisons; partial allocations are released at cleanup. The gate reports
free and largest memory measurements to make future failures identifiable.

The host snapshot test passes full mutable-state restoration, guards around
each block and all twelve immutable-map mutation cases, both normally and
under address/undefined-behavior sanitizers. On PAL A1200, 2 MiB Chip and no
Fast RAM, BASIC passes 603 race and 700 HUD comparisons with zero surface,
particle, immutable-map or HUD mismatches. Final state matches the original
zero-comparison run exactly. Logs are under `tmp/release-render-73f9c03/`.
Remaining track audits and the restored release binary comparison are recorded
separately when complete; this repair alone does not close D-2.

## 2026-09-30 — ship gate D-1 at 73f9c03

Started `make release-check` with an empty `git status --porcelain` at
`73f9c0367914fb501c4f321f3469466f5234693c`. The command completed with exit 0.
Log: `tmp/release-gate-73f9c03.log`.

All prerequisites passed, including timing/physics/lap-limit/collision,
title/Help/key-repeat/loading/font checks, 56,000 Help partial-redraw keys,
30,000 Tracks partial-redraw keys, 378 original Tracks preparation comparisons
and 168 sequential original redraw comparisons. The installer helper passed
197 exact-original outputs, preservation and corrupt/wrong/truncated ZIP tests.

Two clean default Amiga builds produced byte-identical stripped executables.
The subsequent clean packaging build produced the same stripped game hash:
`a25d8cc46aa4f64a2fa807ae7a02d2bfb0529731440cd7f44411161aea6888e8`.
The scratch archive `build/release-check/Slicks-0.1.lha` is 280,990 bytes,
SHA256 `fb004f4bf78c1d3fff16b8df55a678bc3b7eff68b2e5f7499cad9af6ac1796d1`.
Its independent audit passes all 12 allowlisted LH5 members, decompression,
header/payload CRCs and executable/script/icon identity, including version checks.

This closes D-1 only. D-2 rendering/retention checks and D-3 manual installation
and gameplay remain separate gates. This is a scratch candidate, not publication
or a version tag; `dist/` was not replaced. Debug validation remains muted.

## 2026-09-30 — clean-build installer candidate refresh

A clean default-options Amiga rebuild produces the exact same stripped HUNK
and ELF hashes as the WHDLoad regression below. A separate candidate was
created at `tmp/installer-candidate-O0ydKu/Slicks-0.1.lha` (267,646 bytes),
SHA256 `13b5bda2fa2a046b09186ceac8f922a79f0910a1746eda3b68d755f1fe981270`.
The existing `dist/Slicks-0.1.lha` was neither overwritten nor published.

The independent package audit passes all twelve allowlisted LH5 members,
header/payload CRCs, decompression, HUNK checks and exact script/icon/source
identity. No original assets, private keys or emulator/OS material are included.
The package uses the current production slave, not a diagnostic slave.

Real Amiga Installer tests consume this exact LHA and the unchanged publisher
ZIP on PAL 68020 with 2 MiB Chip and no Fast RAM:

| Workflow | Run under tmp/ | Verified result |
| --- | --- | --- |
| Fresh standalone install | `installer-script-8ozngfgu` | Original data, current executable, icons and staging cleanup match. |
| Existing install, Keep, optional WHDLoad | `installer-script-d4tsohzy` | Modified track and settings/key placeholders survive; both launch paths and icon metadata are installed. |
| Explicit Reinstall | `installer-script-9_ib_5o0` | Original track data replaces the modified track; settings/key placeholders survive; staging is removed. |

Only requester answers are supplied by the test harness; extraction, copying,
filesystem operations and native icon changes execute through the real script.
Placeholder files are deliberately not a real registration key.

The fresh installation is then launched unchanged through Execute Play:
`tmp/standalone-release-e7npf7kc` reaches the active native title. A direct
Options/edit/close/reopen/race run on that same installed executable confirms
the default 4096-byte stack and passes at race entry:
`tmp/standalone-release-lpf7judw`. The latter bypasses Play's explicit larger
stack, rather than claiming the launcher itself uses 4 KiB. These are
checkpoint checks, not normal-exit proof; the separate WHDLoad quit check is
recorded below. All test emulators were muted and closed.

Build/package logs: `tmp/release-refresh-build.log`,
`tmp/release-refresh-package.log`. This candidate is verified packaging for
the present build, not a declaration of complete fidelity or resolution of the
outstanding Load Game entry and other open-work policies. Repeat affected
release gates after further production changes before replacing the published
candidate.

## 2026-09-30 — current WHDLoad regression

The stripped executable built from 1d293e2 passes fresh isolated WHDLoad
checks after the menu/cache/title changes. The production, race-test and
exit-test slaves were rebuilt; the latter two supply diagnostic arguments
only and are not release payloads. The original publisher ZIP supplies data
inside each private installation. No registration key is used.

| Workflow | Local run directory under tmp/ | Result |
| --- | --- | --- |
| Production startup, PRELOAD | `whdload-test-fqrar8aj` | Reads original data and reaches the automatic demo before the timed stop. |
| Race, PRELOAD | `whdload-test-ivto1r7s` | Reads original archive/DAT and reaches racing before the timed stop. |
| Normal REGCHECK exit | `whdload-test-qo7ialx2` | WHDLoad reports Return OK; host completion marker passes. |
| Race, no PRELOAD | `whdload-test-yfidkue9` | Live reads of original archive/DAT pass and execution reaches the intentional timed stop. |

Actual AGA copper/bitmap decoding from the startup dump shows the title and
a DEMO-labelled track/HUD; the race dump shows BASIC with updated timers and
effects. This is inspection of native output, not a DOS-frame substitute or
pixel-fidelity oracle. Timed runs intentionally end in WHDLoad's DEBUG dump;
only the separate quit case proves normal return.

Configuration: PAL A1200, 68020, 2 MiB Chip plus 4 MiB Fast, A600 Kickstart
40.063 with matching RTB, locally installed WHDLoad. This retains the existing
WHDLoad memory requirement; it does not claim no-Fast standalone memory limits
or a 4 KiB WHDLoad execution stack. Each run used muted host audio and closed
its emulator. No source/game/ROM/key/dump material was added to Git.

Executable: `tmp/whdload-current-JKq1eh/Slicks`, SHA256
`5f4be6edb92f9ac488c6ae130c8d8983d7a90d3903fe9d3a51057b52a3f79c97`.
Companion ELF SHA256:
`19503a7fd07e3790aed93d768fcab61b03c017de29b5136bc67a3a4ee2ec8f7d`.
Slave build log: `tmp/whdload-current-build.log`. Reproduction uses the
documented `tools/test_whdload.py` modes, `--exe` pointing at that stripped
binary, `--ticks 5000 --seconds 180`, and `--no-preload` for the final case.

This renews the listed WHDLoad workflows for this build. Installer/archive
refresh, unresolved saved-game entry validation and remaining fidelity work
are still open; the existing distribution archive was not replaced or uploaded.

## 2026-09-29 — startup display allocation cleanup

The explicit `DISPMEM` diagnostic tests all ten allocation sites in
`slicks_amiga_platform_create`: bitmap data, bitmap descriptor, copper storage,
Bitmap wrapper and CopperList wrapper for each of the two startup views.
Each case supplies a null allocation at exactly one site; other allocations
and the ordinary failure cleanup execute normally. Wrapper failures skip
construction, matching a null `new` result under the build's `-fcheck-new`.

For each case the target requires failure, consumption of the selected fault,
no active display, and null owning pointers after the internal cleanup.
It calls destroy again, as the outer cleanup may do, and requires total
`AvailMem(MEMF_ANY)` to equal the pre-case value. Scheduling is forbidden
only around each allocation/free check to avoid competing task allocations;
interrupts remain enabled. The checks occur before the real display is
created. Normal launches never enable these faults.

The current stripped build passes all ten cases with a confirmed 4096-byte
entry stack on a stock-speed PAL 68020, 2 MiB Chip and no Fast RAM. It then
creates the real display, enters a demo, pixel-checks both data views,
restores configuration/playlist and exits with system-restoration mask 31.
Evidence: `tmp/standalone-release-lfe6w95v/debug.log` contains
`DISPLAY_ALLOCATION_CHECKS 10` and `DEMO_LIFECYCLE_EXIT_OK`.
Build log: `tmp/display-allocation-build.log`. The run was muted and its
emulator closed. No cleanup defect was found.

This is controlled allocation-failure coverage, not deliberate system-wide
memory exhaustion, a stack high-water measurement, or validation of unrelated
startup resource failures. It does not renew every release workflow for this
binary.

## 2026-09-29 — current title/menu default-stack regression

Two further workflows pass on the same stripped binary and hardware settings
below, both with a confirmed 4096-byte entry stack and normal system restoration:

- Options-owned Help navigation, history return, close and reopen:
  `tmp/standalone-release-cdyjoeo9`. Five ready checkpoints and two closes pass;
  the complete 64,000-byte saved background and final restored chunky image
  compare identically.
- Championship save from a genuine first intermission, including picker
  cancellation/re-entry, name acceptance, catalogue refresh and exit:
  `tmp/standalone-release-z_axl9an`. The shared resident-display assertions
  are expanded by the standalone harness and execute in this run.

The subsequent `CHAMPLOAD` run (`tmp/standalone-release-8y29uof8`) is **not a
pass**. Inspection showed its startup input still sends four Down keys and
Enter to the obsolete title Load Game row; it never reached the saved-game
picker or resume checkpoint. The debugger was stopped and the harness closed
its emulator. Do not rerun this fixture as a release gate until its original
entry route is resolved. No before/after championship-state comparison or
fresh-process resume is established by this batch, and no invented menu row
was reinstated. The private fixture contains the test save for later checks.

2026-09-30, decision D1 (keep Load hidden, as the original does): the
`CHAMPLOAD`, `CHAMPLOADW` and `CHAMPFAIL` fixtures and their debugger scripts
were removed, together with the uncommitted dynamic Load-storage draft
(archived locally as `tmp/load-storage-draft-20260930.patch`). The
unreachable Load handler itself stays, mirroring the original's. After the
removal, `CHAMPSAVE` then `CHAMPEDIT` in one shared run directory passed
(`NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`,
`NATIVE_CHAMPIONSHIP_RESAVE_OVERWRITE_DELETE_CANCEL_OK`; `tmp/b4-{save,edit}.log`).

Current-build full-frame racing audits also pass 600 updates each on F1
(`tmp/standalone-release-cl_xj10f`, 32 actors, 2,068 marks), CITY
(`tmp/standalone-release-o6qr7np5`, 18 actors, 1,480 marks), and WHACKO
(`tmp/standalone-release-whrmplu7`, 5 actors, 1,854 marks). All confirm
the 4 KiB entry stack and live statistics disabled, using the stripped binary
and ELF hashes below. These compare incremental bitplanes against the full
rendering reference; they are not performance measurements or normal-exit
tests. The harness closes each emulator after the successful checkpoint.

The current ELF was converted directly to a stripped HUNK and installed in
a fresh private original-data directory, without overwriting `dist/` or an
existing release candidate. This includes the prepared-title background fix
and tight registered-owner bounds. Executable SHA256:
`9e78a66e4dbedaa31f9f57795075071b8fad0d6d606df25a46d37bfe63784f8f`.
Companion ELF SHA256:
`f7f021b4fcc69ce11eb8d49cabc997eaae231d77c2a4612037f4872b4e48f817`.

All three runs confirm a 4096-byte task stack at entry, with no Stack command,
on PAL A1200/68020 real speed, 2 MiB Chip and zero Fast RAM:

- Normal active title: `tmp/standalone-release-xzvybrv3`.
- Options edit, close, reopen and race handoff:
  `tmp/standalone-release-pjvfrrlc`.
- Unmodified demo entry, both track-data views, restoration of configuration
  and playlist, zero setup saves, and normal system-restoring exit:
  `tmp/standalone-release-33y84fv3`.

The installation is `tmp/release-current-PH0gjX`; it contains copied original
data, not private registration material. Runs were muted and the harness
closed each owned emulator. Title and Options fixtures stop at their stated
checkpoints; only the demo fixture proves normal exit here. These are bounded
stack/memory regressions, not exhaustive high-water measurements, renewed
WHDLoad/installer validation, or completion of the remaining release gates.

## 2026-09-28 — display-end publication pacing

Supersedes the VBlank-at-loop-entry limiter below. Simulation and chunky
rendering now run immediately after the preceding publication. Once ready,
the main race path waits for a **fresh** row `$100` display-end edge, then
updates audio/palette and performs C2P without another synchronization wait.
If preparation finishes during the lower border, it waits for the next edge
rather than publishing late or twice in one refresh. Missed opportunities
do not accumulate catch-up updates. Existing menu and modal timing is unchanged.
The original level-sensitive `wait_display_blank` remains for those callers;
`wait_display_end` is a separate edge-sensitive publication wait.

`diag_display_end_limit.gdb` replaces `diag_vblank_limit.gdb`: it checks
publication timing, not simulation-start VBlank counters. On 2 MiB Chip,
no-Fast, confirmed 4 KiB-stack native runs:

- Accelerated 68040: 120 publications spanning 119 VBlank intervals, all at
  row 256 (`tmp/standalone-release-ed26g3lg`).
- Stock-speed 68020: 120 publications spanning 121 VBlank intervals, at
  rows 256–257 (`tmp/standalone-release-11xv4vz_`).

Repeat using `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack
--args NATURALQB --checks amiga/diag_display_end_limit.gdb
--marker DISPLAY_END_LIMIT_OK`, adding `--cpu 68040 --cpu-speed max` for the
accelerated case. Use a disposable installation for the test's game saves.

The required stock-speed, live-statistics-off full-frame display audits pass
600 updates each: F1 (`tmp/standalone-release-otwrfnx1`), CITY
(`tmp/standalone-release-5sxrw5qw`) and WHACKO
(`tmp/standalone-release-kjjbcn4o`). All sessions are muted and closed on exit.

These breakpoint-observed timing checks establish phase and rate, not an
uninterrupted performance benchmark or proof that C2P always fits inside the
blanking window. The single-buffered design and outstanding worst-frame
performance target are unchanged. The publication wait remains excluded from
measured CPU work; end-to-end cadence includes it.

## 2026-09-28 — gameplay refresh-rate limiter

The main loop now permits at most one iteration per new vertical-blank count
while racing. The existing display-blank wait only protects visible DMA writes:
it returns immediately throughout the lower border and therefore could permit
multiple updates in one refresh on a fast CPU. Menus retain their unconditional
VBlank wait. Updates that already cross a VBlank incur no additional wait;
missed refreshes do not accumulate catch-up updates. The limiter is outside
the measured work region. Simulation and rendering code are unchanged.

`diag_vblank_limit.gdb` observes 120 consecutive real simulation updates and
rejects any repeated VBlank counter. Muted A1200 tests with 2 MiB Chip, no Fast
RAM and the confirmed default 4 KiB task stack passed:

- 68040, maximum CPU speed: 120 updates spanning 120 VBlanks,
  `tmp/standalone-release-77armh10`.
- 68020, real CPU speed: 120 updates spanning 122 VBlanks,
  `tmp/standalone-release-fo5cp1dn`.
- Options edit/reopen/race entry (`tmp/standalone-release-9rqn9w4c`) and
  championship save/normal system-restoring exit
  (`tmp/standalone-release-nz430lqg`) also pass on the stock configuration.

These short pacing checks include countdown/startup; they do not establish
that all gameplay now meets the outstanding 20 ms performance target.
Repeat with `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack
--args NATURALQB --checks amiga/diag_vblank_limit.gdb --marker VBLANK_LIMIT_OK`
and optionally `--cpu 68040 --cpu-speed max`.

## 2026-09-28 — default 4 KiB stack verification

The unchanged stripped release executable was launched directly, bypassing
Play's conservative `Stack 16384`. No Stack command was used. Read-only GDB
checks at main entry confirmed `tc_SPUpper - tc_SPLower == 4096` in every run.
Configuration: PAL A1200, 2 MiB Chip, no Fast RAM; debug audio muted.

Passed paths and local-only evidence directories (`tmp/standalone-release-*`):

- Normal title/startup: `f8z8wiak`.
- Options edit, close, reopen and entry into racing: `yn4541t_`.
- Championship menu save and normal system-restoring exit: `49dsjbiw`.
- Fresh-process championship reload/resume, advancing the race and normal
  exit: `l90qfbku`. Saved/restored points, cash, inventory and vehicle dumps
  also compare byte-for-byte.
- Help-menu navigation, history, close/reopen and system-restoring exit:
  `xzeu7a54`.
- F1 racing through 600 updates with the full-frame dirty-sprite audit:
  `er198zfr` (`NATURALO1Q`, `diag_dirty_sprites.gdb`).

The executable SHA256 is
`fc6b9d7db5ee6253ce716778a6d9d04815b7ed107c51dc7cdcdd1a4637e60b24`,
identical to `build/release/Slicks`. No game rebuild or code change was needed.
This verifies the listed workflows, not an exhaustive maximum-stack bound.
An attempted debugger-written watermark failed its immediate write/readback
check and was discarded; it is not evidence of a game stack overflow or a
valid high-water measurement. A repeated save fixture with an already existing
test save and an incorrect help launch mode were also discarded and rerun
with the correct clean fixture/mode.

Repeat using `tools/test_standalone_release.py PRIVATE_INSTALL --default-stack`;
add `--args OPTIONS --checks amiga/diag_options.gdb
--marker OPTIONS_ENTRY_EDIT_RETURN_REOPEN_RACE_OK` for the options path.
Use a disposable installation: scripted checks really write saves/records.
The harness isolates debugger dumps and closes its own emulator on success
and failure. The existing package/launchers are unchanged by this verification.

## 2026-09-28 — installer release 0.1

The distribution is now `dist/Slicks-0.1.lha`, not the historical developer ZIP
below. Instructions/build details are in [install-original-data.md](install-original-data.md)
and [whdload.md](whdload.md). This is packaging/validation, not release publication.

- Downloaded the exact publisher Slix151.zip specified by the user. Native
  helper extraction matches an independent ZIP-library reader for both data
  files and all 195 tracks. Both core files also match development originals.
- Host rejection tests cover missing, empty, truncated, extended and altered
  archives and an existing destination containing settings/key placeholders.
  No settings/key are replaced, selected or shipped.
- Native helper: PAL A1200, 2 MiB Chip, zero Fast RAM, 4 KiB stack. All 197
  outputs match independently; 1,246 bytes of stack remain unused. Partial
  staging is cleaned. Actual user-key bytes are not test inputs.
- Real Amiga Installer 43.3: fresh standalone installation directly from the
  LHA candidate plus original ZIP passes on the same 2 MiB/no-Fast machine.
  Only requester answers are supplied deterministically; helper execution,
  filesystem operations, copying and native icon updates run unchanged.
  Existing-install Keep with optional WHDLoad preserves settings/key and
  produces both launch icons. Native project-icon default tools/tooltypes
  are checked (IconX and WHDLoad/SLAVE/PRELOAD).
  Explicit Reinstall replaces a deliberately modified original track while
  retaining settings and key placeholders; staging is removed after success.
- The installed, unchanged Play script runs SetPatch, sets the stack, enters
  data/ and reaches the native active title display on a 2 MiB/no-Fast A1200.
  Its executable is the stripped release executable, not a test replacement.
- WHDLoad 19.2.6941/A600 Kickstart 40.063: production-slave normal startup;
  real racing with PRELOAD on 4 MiB Fast RAM; live reads with PRELOAD disabled;
  normal REGCHECK exit returns OK. Diagnostic-only slave arguments select
  racing/exit without altering the production game. Actual AGA bitplanes and
  palette decoded from dumps show the native title and race/HUD/effects.
  WHDLoad is not claimed to work without Fast RAM.
- A clean default-options Amiga rebuild has a byte-identical stripped HUNK
  payload to the tested release candidate. Only discarded debug-symbol
  padding differed in the unstripped executable.
- Independent Lhasa extraction verifies every member against source, both
  header and payload CRCs, exact 12-member allowlist, LH5 format, HUNK binaries
  and reference icons. No private key, original archive/data, DOS program,
  ROM, RTB, WHDLoad binary, host configuration, dump or screenshot is included.
- All owned debug runs are muted and terminate their own emulator. Existing
  run.sh remains audible. The source tree contains no original or private files.

Limits: PAL stock-A1200 standalone and the stated emulator WHDLoad configuration
were tested, not all accelerators/ROMs/filesystems. Hardware joystick checks
remain deferred by the user. Gameplay still does not meet the 50 FPS target.
Registration's interactive exit-help/optional missing-image checks remain on
the open list; packaging does not reclassify them as completed.

## 2026-09-26

- Runtime dependencies: AmigaOS DOS/utility/keymap libraries version 37 and
  graphics.library version 39, AGA, the supplied original archive/DAT/tracks,
  and SetPatch from the user's AmigaOS installation. No PC executable, host
  emulator, SDL or external audio mixer is needed by the native binary.
- Build dependencies remain the documented shared Amiga GCC/vasm/elf2hunk
  toolchain and generated inputs derived locally from the user's executable.
  FS-UAE and the shared process helper are development-launcher dependencies,
  not dependencies distributed inside the native release.
- Runtime memory: all six mode-transition tests passed with exactly 2 MiB chip
  and no Fast RAM. A fresh current-build race entry also passed; the target
  compiler reports a 202,702-byte race structure. The native shop saved two
  screenshots with an additional temporary 65,078-byte allocation on the same
  configuration. These checks establish tested configurations fitting, not a
  claim that arbitrary Workbench memory pressure or every asset combination fits.
- Fresh BASIC GO preparation measured 142 OS ticks (2.84 seconds), excluding
  the starting-light countdown. Evidence: `tmp/release-startup.log`, fixture
  `amiga/diag_release_startup.gdb`; no per-frame debugger stops were used.
- Launcher defaults retain PAL A1200, 2 MiB chip and no Fast RAM. `run.sh`
  keeps audio enabled; scripted debugging uses dummy host audio and closes its
  owned emulator. `run.sh` now consistently honors `FSUAE_RUN` for both the
  mounted directories and process ownership instead of splitting those paths.
- Packaging uses an exact four-file allowlist: native HUNK executable, README,
  credits and a SHA-256/source-revision manifest. It checks HUNK headers and
  archive membership/content, refuses replacement of an existing archive,
  and never traverses the original data or generated reference directories.
  The source tree tracks no files under `ref`, `tmp`, `src/gen`, `amiga/out`
  or emulator run directories. Runtime data and OS/ROM files must be supplied
  separately; no license for those assets is implied.

At that revision a local archive was built with `make release-package`. A fresh output path
used `RELEASE_ARCHIVE=build/release/<name>.zip`. Those instructions are historical;
use the current installer build instructions above. No upload/push is implicit.

The verified local package is `build/release/slicks-749bb11.zip`, built from a
clean working tree. Its executable is 408,344 bytes; declared HUNK allocations
total 389,316 bytes (excluding runtime allocations and OS memory). All four
archive entries were reread and compared to their source content. No release
was uploaded. Integration/release open items 1–3 are now complete within the
native-port scope; general translator expansion remains explicitly deferred.
