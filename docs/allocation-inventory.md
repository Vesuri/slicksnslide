# Static allocation inventory

Source baseline: 8b225ac, 2026-09-30. This is a source census and migration
design, not a claim that startup-only ownership is implemented. Runtime tests
validate the inventory; they must not be the mechanism for discovering it.

## Census

Search all `src/`, including framework code and headers, for `Alloc*`, C heap
calls, `new`, and allocation callbacks. Follow wrappers and compare the Amiga
Makefile/link map so unused framework helpers are not charged to the game.
There are **62 direct AllocMem call sites**, distributed below. Counts are
source expressions, not allocation counts, live blocks, or byte totals.

| File under src/platform/amiga | Sites | Coverage |
| --- | ---: | --- |
| amiga_player_menu.c | 16 | Primary owner (1); pause, icons, track info, intermission, Change Cars, controllers, message, name, colour (9); three pickers and two name/index buffers (5); track-list dialog (1) |
| slicks_diag.c | 23 | Detailed below; includes startup, runtime and diagnostics |
| amiga_setup_storage.c | 11 | All persistence/transaction buffers listed below |
| amiga_audio.c | 4 | PCM allocation loops and silence buffer |
| amiga_platform.cpp | 3 | Display data, BitMap metadata and copper storage |
| resource_archive.c | 4 | Archive directory, cache owner, entries and encoded resources |
| framework_runtime.cpp | 1 | C++ new/new[] allocator, including its size header |

No C malloc/calloc/realloc calls were found in the production source scan.
Actor/particle slot allocation is fixed-pool indexing, not heap allocation.

### slicks_diag.c: all 23 sites

| Owner/function | Sites | Size/lifetime and disposition |
| --- | ---: | --- |
| PC sampler | 1 | Diagnostic sample capacities × 8; exclude from release requirement |
| allocate_track_storage | 1 | Wrapper: discovery name-table growth and startup playlist; reserve catalogue-dependent capacity before menus |
| discover_tracks sort | 1 | 2 × track count; startup temporary, safe to release before play |
| test_pause_children | 1 | 64,000; diagnostic-only image comparison |
| load_plain_allocated | 1 | Actual file length; remaining callers are intermission and its diagnostic; replace with bounded borrowed spans |
| prepare_race shadow | 2 | 57,344 + 64,000; diagnostic-only |
| run_intermission language | 1 | 2,048; construction staging, does not need independent ownership |
| run_intermission saved_tracks | 1 | 8 × track count; must coexist with save encoding, intermission parent and filename dialog |
| run_record_results | 1 | Baseline site removed: 8,192-byte modal storage lease; parsed record values are copied, so release precedes warnings/table creation |
| registration_screen | 1 | Baseline site removed: 70,000-byte modal storage lease, released before registration Help and on failure; no runtime image allocation |
| run_championship_results | 1 | Baseline site removed: 64,003-byte modal storage lease, released before results surface creation and on failure |
| main title asset/frame | 2 | 64,003 and TITLE_FRAME_ALLOCATION_BYTES; startup source and persistent title |
| main fonts | 1 | Three size-dependent allocations from one expression; startup/resident |
| main logical/chunky/race | 3 | 262,144; CHUNKY_ALLOCATION_BYTES; sizeof race; startup/resident |
| main sample_resource | 1 | 131,691; startup decode input, freed before play |
| demo_expected_playlist | 1 | 2 × playlist capacity; explicit demo diagnostic snapshot |
| retention check | 3 | Saved chunky, snapshot owner and prefix allocations; diagnostic-only |

### Persistence: all 11 sites

| Operation | Sites | Required coexistence / replacement |
| --- | ---: | --- |
| store_capture | 1 | Baseline site removed: explicit 65,078-byte caller scratch from modal storage; source chunky remains live and separate |
| store_setup | 1 | Baseline site removed: explicit 5,771-byte caller scratch from modal storage; live profiles/configuration retained during atomic replacement |
| load_track_lists | 1 | Up to 65,536; preserve caller output on failure |
| track_list_cache_refresh | 2 | 65,536 staging plus retained next.size; old cache must remain valid until successful validation/publication |
| store_track_lists | 1 | 65,536 output alongside immutable old catalogue and selection |
| load_saved_game | 1 | 6 + 8 × capacity + 212; inspect production reachability separately from hidden Load UI |
| store_saved_game | 1 | Encoded game size; input track-name array must remain live throughout encoding/write |
| store_track_records | 1 | Baseline site removed: explicit 8,192-byte caller scratch, leased from modal storage for post-race saves and confirmed Clear Records; unchanged new/backup transaction |
| load_setup | 2 | Profile bytes plus candidate SlicksPlayerProfiles; transactional startup load, not live-profile overwrite |

### Wrappers, framework and OS

- `operator new[]` calls `operator new`; delete/delete[] return the exact
  header-recorded allocation. Startup platform creation constructs two Bitmap
  and two CopperList wrappers. The hardware blitter queue is another C++
  startup allocation with explicit shutdown cleanup.
- Framework Palette24Bit has two array allocations; Bitmap/CopperList static
  allocation helpers use Util's memory pool. Util, Sprite and Palette also
  contain allocation helpers in the imported tree. Verify retained symbols in
  the link map before assigning these to the live budget; source presence alone
  is not evidence of a runtime allocation. Util is not a normal Makefile object.
- Resource cache creation allocates one owner, entry array and one block per
  cached resource at startup. `resource_archive_open` allocates a directory;
  trace every remaining disk-backed caller before calling this startup-only.
- PCM resource conversion allocates Chip RAM at startup; effects playback must
  keep borrowing those samples rather than allocate per sound.
- `AllocDosObject(DOS_FIB)` in discovery is explicit OS-object ownership;
  `FreeDosObject` closes it. Open/Close, Lock/UnLock, library opening and DOS
  handlers can allocate internally. Game startup reservation cannot eliminate
  OS or disk failures; report these separately from game workspace ownership.

## Finite implementation groups

1. **Dialog slots:** map every open/close/failure/destroy edge, then reserve
   typed slots or proven-exclusive unions. Include pause (with Help/Change Cars
   children), intermission (with picker/name children), Controllers, track info,
   message, name, colour, profile picker and track-list state. Do not union all
   these merely because each is modal: a modal can have another modal as child.
2. **Picker variable payloads:** saved filenames need at most 40 × 9 bytes;
   track-list title offsets need 2 × floor((65,536−8)/23). Profile picker borrows
   the live profile data. Reserve payload separately from retained catalogue.
3. **Persistence transaction workspace and catalogue:** reserve encoding/read
   scratch and retained catalogue separately. Publish only after validation;
   no overwrite of old catalogue on failed refresh. Preserve all .new/.bak
   rules. Capacity must cover the existing supported limits or fail at launch,
   not silently reduce functionality.
4. **Intermission/records/ending staging:** remove the remaining allocations
   listed above, using explicit phase-owned spans. Track-information staging
   is already converted. Include registration and screenshot paths even when
   ordinary race tests never exercise them.
5. **Startup/diagnostic/OS boundary audit:** establish a release startup-complete
   marker; reject normal-game direct heap allocations after that marker in the
   audit. Keep diagnostic exceptions named. Check archive reopen callers and
   linked framework constructors. Verify shutdown returns every startup block.

## Lifetime constraints established from source

- Primary menu owns its saved page, fonts and icon pixels throughout all child
  dialogs. Only bytes beyond the 64,000-pixel snapshot can be staging while
  the parent snapshot is live; constructor staging has a shorter lifetime.
- Pause remains alive while Help is open. Help currently borrows particle
  visibility and restores it on exit. Another child must not independently use
  that same cache while Help owns it, even if racing is stopped.
- Track-list state and its immutable catalogue remain live beneath its picker;
  picker offsets refer into that catalogue. Cache publication cannot invalidate
  those pointers. A new catalogue cannot share storage with its old source
  while validation/encoding is in progress.
- Saved-file picker and filename dialog explicitly reject coexistence. Colour
  rejects an active name dialog. Proving a union still requires checking every
  entry edge (including direct open_at callers), not just those two guards.
- Intermission retains the parent display while saving. Its track-name input
  and encoded output must be separate, and failure must leave the dialog usable.
- Tracks preview can overwrite VGA because its current image and parent are
  chunky-owned and its exits rebuild the title. This does **not** grant the
  same permission to arbitrary pause or screenshot operations during a race.

The child matrix below establishes the first shared modal slot. The exact
combined peak layout for retained parents, catalogues and transactions remains
design work, not discovery of new direct allocation sites. Validate each layout
bound on the target ABI, then run failure, publication, repeated-transition and
cleanup tests.

## Child ownership matrix

The retained primary menu (87,958 bytes) is outside all child storage below.
Rows describe the actual input dispatch: it services the active child before
allowing another menu action. Error/exit destruction must release that child.

| Screen / retained parent | Sequential children | Additional simultaneous storage |
| --- | --- | --- |
| Players / inline profile editor | Help, profile picker, name, colour | Live profiles; editor state in primary owner |
| Options | Help, Controllers, confirmation/message | Live configuration |
| Tracks | Help, track information, track-list workflow | Catalogue stays resident |
| Track-list workflow (34 bytes) | Picker → name or delete message | Catalogue and playlist; picker closes in track_lists_choice before next child |
| Pause (8,588 bytes) | Help, Controllers, embedded speed dialog | Race/maps stay live; simulation stopped |
| Shop | Help | Shop state/inventory stays live |
| Intermission (4,228 bytes) | Change Cars, saved-game workflow | Race results, playlist and exported track names |
| Saved-game workflow | Picker → name → confirmation/message | Encoded transaction output must not alias track names |

Confirmed target sizes for child layout: Controllers 34,952, name 16,822,
colour 2,558, picker 35,132, Change Cars 10,098, message 8,330 bytes.
Help including its parent snapshot is 110,096 bytes and dominates the union.

Help/Controllers/name/colour now share that 110,096-byte modal overlay, inside
the existing 117,760-byte particle visibility cache. An owner tag rejects
cross-type acquisition/release; each acquired object is cleared. All close,
failed-open and parent-destroy edges restore the particle cache before resume.
No additional startup bytes are needed. The existing diagnostic allocation
failure hooks now exercise acquisition failure, without introducing a heap
fallback. Other children and retained parents are not converted yet.

The three picker constructors and their filename/index payloads now also use
this slot. The aligned payload is 5,698 bytes, covering the maximum validated
track-list title offsets and all 40 saved filenames. The union remains 110,096
bytes. Picker release does not free the borrowed payload. Track-list state and
the retained catalogue remain separate; choice closes the picker before name
entry or confirmation. Five further allocation expressions are removed.

Track Information, Change Cars and generic messages also use this slot.
Their normal call paths close a failed/finished child before opening a message;
the owner tag rejects accidental nesting instead of overwriting it. Track
information restores painted bounds and font colours before releasing storage.
Change Cars retains its separate intermission parent. Icon staging uses bytes
64,000–64,511 of the primary saved-page reservation, beyond the live snapshot.
These remove four further allocation expressions without enlarging either slot.
The pause, intermission and track-list parent objects now share a separate
8,588-byte startup union after the primary menu, raising that reservation from
87,958 to 96,546 bytes. Parents do not coexist with each other, but do coexist
with the child union. Their acquisition, failure, release and shutdown edges
are owner-tagged; releasing a primary owner with a live parent is rejected.
amiga_player_menu.c now contains only the startup allocation and shutdown free.

Maximum-catalogue + Workbench tests exposed two startup outcomes: menu-owner
reservation can fail, or catalogue retention can fail first and leave a later
recoverable warning. This is not the final B12 all-or-nothing contract. Reserve
the combined required budget before publishing any live menu; distinguish
memory reservation failure from recoverable filesystem errors.

Normal intermission no longer allocates DAT, track, language or exported
playlist names. The phase-exclusive union in idle VGA is 141,312 bytes, with
disjoint input/output/label spans during preview and an 80,000-byte name span
after the renderer copies its labels. See memory-lifetimes.md for offsets and
proof. The allocating plain-file helper remains only in the explicit
intermission construction diagnostic. Persistence encoding stays separate
and remains open work; it must coexist with those exported names.

Track-list store/refresh staging now borrows the modal overlay under a distinct
storage-owner tag. Both adapters take explicit caller-owned 65,536-byte scratch;
their staging AllocMem/FreeMem expressions are removed. Picker/name/confirmation
children close before acquisition, and any new warning opens only after refresh
releases the slot. Startup refresh uses the same already-bound overlay. The
retained catalogue's exact-size replacement allocation is still open B12 work,
as are other persistence operations. No additional startup bytes are reserved.

This matrix separates child payloads from longer-lived parent, catalogue and
transaction data. The remaining work is placing those parents/payloads and
transactions into bounded spans and proving the combined peak, not treating
all modal objects as mutually exclusive.
