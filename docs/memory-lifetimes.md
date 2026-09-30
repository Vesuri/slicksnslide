# Runtime memory ownership

## 2026-09-30 audit and target

The requested contract is startup reservation of all game-owned memory needed
for supported gameplay and menus, followed by bounded workspace reuse. A private
general-purpose heap alone would not prove this: it could still fragment or run
out. Reserve by simultaneous lifetime and reject unsupported catalogue sizes
before gameplay. Disk/OS operations can still fail independently and must retain
their recoverable error handling; startup reservation cannot guarantee disk I/O.

The current implementation does **not** meet that contract yet.

The complete direct-allocation census and finite migration groups are in
[allocation-inventory.md](allocation-inventory.md). Complete its nesting/phase
matrix before further one-off conversions; runtime failures are validation
evidence, not the allocation discovery process.

### Measured baseline

Target sizes from the built 68020 ELF, not host ABI sizes:

| Storage | Bytes | Lifetime |
| --- | ---: | --- |
| VGA logical image | 262,144 | Startup to exit |
| Chunky image/decode workspace including C2P lookahead | 65,568 | Startup to exit |
| Prepared title background including lookahead | 64,034 | Startup to exit |
| Two eight-plane display bitmaps | 128,000 | Startup to exit |
| Race runtime, including maps/assets/render caches | 369,104 | Startup to exit |
| Common menu owner / track-load staging and retained parent | 96,546 | Startup-reserved exclusive workspace |
| Help viewer | 46,096 | Modal overlay in startup particle cache |
| List/profile picker | 35,132 | Nested modal lifetime |
| Intermission dialog | 4,228 | Between races |
| Track navigation decode staging | 3,018 | Track preparation |

The first five total 888,850 bytes, excluding code/data/BSS, audio, cached menu
resources, fonts, catalogues, copper lists, library allocations and AmigaOS.
The older DEMOPLR allocation audit (`tmp/standalone-release-vjqrkoq0/debug.log`)
peaks at 1,337,664 bytes of directly tracked game allocations and ends at zero.
That is one tested path, not the program's worst-case total RAM requirement;
it excludes the loader's executable segments and OS-owned memory.

The common menu owner includes a 65,536-byte saved-page/decode buffer and three 6,000-byte
font arrays. These dominate small menus too. Nested Help's 64,000-byte save-under
now shares the particle-cache modal overlay. Never sum all dialog sizes as though all were live
together, but never assume nested dialogs can overwrite their parent.

The race allocation includes a 117,760-byte particle visibility lookup,
two 60,800-byte material/surface maps, 19,200 bytes of track drawing packets,
14,996 bytes of car-render cache and 8,704 bytes of track visibility cache.
These are not all redundant images: several are deliberate performance caches.
Changing their layout/removing them needs speed and fidelity measurements, not
an assumption that every large buffer is waste.

### Confirmed avoidable peak

Intermission previously allocated a further 65,536-byte DAT preview arena.
On Workbench-loaded stock 2 MiB this failed after successfully allocating the
41,500-byte DAT, 2,011-byte next track, menu and dialog. Preview now borrows
the first 64 KiB of the startup-owned VGA image: the completed race cannot
resume, the parent/retry display is retained in chunky, and next-race or title
preparation reconstructs VGA contents. No displayed pixels or live race state
are used as preview output. The arena API explicitly borrows rather than owns.

### Remaining allocation families and reservation design

1. **Primary menu owner (implemented):** Players, Tracks, Options, Help, pause, shop, records,
   intermission and endings use the same large owner type. Prove exclusive
   primary ownership: one slot is now reserved at startup and cleared on each
   menu acquisition. Nested children still need their own slots. Busy/oversize
   acquisition fails without an AllocMem fallback and increments an ownership
   diagnostic. The reservation is freed once at shutdown.
2. **Menu construction resources (implemented):** Help/race fonts, Players,
   Options, Tracks and shop construction borrow the owner's save-under before
   its parent snapshot becomes live. Extending that reservation by 1,536 bytes
   removes 8/32/64 KiB transient allocations and accommodates indexed image
   headers. Nested icon loading uses 512 bytes of the reserved tail beyond the
   64,000-byte live parent snapshot.
3. **Track preparation (implemented):** 65,536-byte DAT, 8,192-byte track, 3,018-byte navigation,
   128-byte car and 2,048-byte font staging. Chunky already supplies decode
   workspace. Preparation now shares the inactive primary menu slot only after
   shop destruction, with explicit acquire/release ownership and a compile-time
   capacity check. No five staging allocations remain in `prepare_race`.
   Tracks' information preview now uses disjoint spans of idle VGA storage
   (65,536-byte decode arena, 65,536-byte DAT staging, 8,192-byte track staging).
   Its chunky parent and save-under remain untouched; leaving Tracks rebuilds
   the title VGA image. Its information dialog uses the shared modal overlay.
4. **Nested dialogs:** Help now borrows 110,096 bytes from the startup-owned
   117,760-byte particle visibility cache. The viewer and 64,000-byte parent
   snapshot have exclusive modal ownership. Racing is stopped during Help;
   release reconstructs the cache from unchanged terrain maps before any race
   update can resume. At initial title startup `race->started` is explicitly
   zero; no uninitialized terrain is read. Binding capacity is checked at
   compile time and startup, and no heap fallback exists. Controllers and
   name/colour dialogs now share the same exclusive overlay, without growing
   it. Pickers and their bounded 5,698-byte name/index payload also share it.
   Track Information, messages and Change Cars also share that child slot.
   Pause, intermission and track-list parents share a separate 8,588-byte union
   appended to the primary startup reservation. Its owner tag remains live
   across child dialogs and must be released before the primary owner. All
   dialog acquisition is now bounded reuse: amiga_player_menu.c contains only
   its startup AllocMem and matching shutdown FreeMem.
5. **Persistence/catalogues:** track-list store and refresh now borrow 65,536
   bytes from the owner-tagged modal overlay, after picker/name/message children
   close. Primary/track-list parents and the immutable catalogue are separate.
   Refresh reads/validates unpublished scratch directly, then still allocates
   an exact-size retained copy before releasing the previous copy. That retained
   replacement is not yet startup-only. Both APIs reject missing/short scratch
   before filesystem work. Release rebuilds the particle cache if a race exists.
   Reserve catalogue/transaction capacity at launch; bound dynamic name tables,
   profile editing, save/load and screenshot encoding without losing atomic
   new/backup file replacement or original data support.
   Record input and transactional writes now reuse an 8,192-byte modal storage
   lease. Parsing copies all table values before release; no pointer into the
   source file survives. Post-race display owns only the primary surface and
   inline icons/fonts, not a modal child, so saves can borrow the modal slot.
   Retry/skip notices run after releasing it. Options closes the confirmation
   message before acquiring one lease for the whole Clear Records loop and
   releases it before the result/path notices. All error cleanup releases live
   leases; no track-record adapter allocation or heap fallback remains.
   Championship save/load also use explicit modal scratch after filename,
   picker and confirmation children close. Saving requests the exact encoded
   length (up to 80,218 bytes, compile-time checked against the reservation).
   Input names remain in the separate completed-race VGA workspace, with the
   intermission parent still live. Scratch is released before success/failure
   notices. Hidden Load's bounded outputs are separate from its scratch and
   are published only after a complete validated read; it remains hidden.
6. **Registration/ending screens:** Registration's 70,000-byte decode input
   now uses the modal storage lease, released after decode and before Help.
   Chunky/palette output does not alias that lease. Failed loads/decodes also
   release it. Trophy's 64,003-byte staging uses the same lease, released
   before creating the results surface; its file access retains the faded
   game display. Archive directories remain to reserve.
7. **Startup-only ownership:** bitmaps/copper, PCM banks, compressed menu cache,
   fonts and main images can retain their existing startup allocation/exit-free
   pattern. Audit framework and OS calls separately, rather than routing Chip
   DMA data into a generic workspace.

### Intermission staging layout

Exit setup saving borrows 5,771 bytes of modal storage, separate from live
configuration/profiles, and releases it before any warning or registration
presentation. The normal exit action comes from the title; demo persistence
diagnostics may save while a race exists but have no active modal child.
Both failure/retry and failure/cancel/reopen/save have native ownership and
disk round-trip coverage. Insufficient caller scratch fails before disk I/O.

Shop capture borrows 65,078 bytes from modal storage only on the normal shop
input path (no Help/message child). Its primary parent and chunky source do
not alias the lease. The lease is released before opening a write-failure
warning; storage rejects insufficient scratch before touching disk. No new
startup bytes or capture-time allocations are required.

The completed race's VGA allocation now hosts a 141,312-byte union. During
construction its spans are decode output [0,65536), DAT input [65536,131072),
track input [131072,139264), and decoded language [139264,141312).
The bounded file loader rejects empty/oversized inputs, short reads and failed
Close operations before preview construction. Failure retries reuse the spans.
The renderer copies all labels/names into the retained parent before returning.
After that return the same union holds up to 10,000 exported eight-byte track
names (80,000 bytes) throughout the nested save workflow. Transaction encoding
is still separate; it must not overwrite those input names.

Neither chunky, either displayed bitmap, the primary saved parent, nor the
particle-cache child slot aliases this union. Next race/title preparation
rebuilds VGA; the completed race cannot resume. No startup allocation grows.
The old allocating plain-file helper is now used only by the explicit
intermission construction diagnostic, not by normal game intermission.

### Validation requirements

Reserve the 96,546-byte menu/parent block after startup title/sample staging
is freed and before allocating the many small menu-cache resources. Reserving
it last reproduced the reported startup error with a maximal catalogue;
reserving it first passes without increasing resident memory.

- Workbench loaded, stock PAL 68020, 2 MiB Chip, no Fast, default 4 KiB stack.
- Track actual Exec allocation/free calls through menus, demo/races, intermission,
  saving and shutdown; distinguish OS allocations from game-owned ones.
- Test busy-slot rejection, startup failure and partial initialization cleanup.
- Repeat transitions and restart in the same OS session; no outstanding memory.
- Compare whole chunky/planar publications and retained parent screens; use
  emulator output for palette timing and Workbench flashes.
- Keep extended diagnostic-only allocations out of the normal-game budget.
