# Runtime memory ownership

## 2026-09-30 audit and target

The requested contract is startup reservation of all game-owned memory needed
for supported gameplay and menus, followed by bounded workspace reuse. A private
general-purpose heap alone would not prove this: it could still fragment or run
out. Reserve by simultaneous lifetime and reject unsupported catalogue sizes
before gameplay. Disk/OS operations can still fail independently and must retain
their recoverable error handling; startup reservation cannot guarantee disk I/O.

The current implementation does **not** meet that contract yet.

### Measured baseline

Target sizes from the built 68020 ELF, not host ABI sizes:

| Storage | Bytes | Lifetime |
| --- | ---: | --- |
| VGA logical image | 262,144 | Startup to exit |
| Chunky image/decode workspace including C2P lookahead | 65,568 | Startup to exit |
| Prepared title background including lookahead | 64,034 | Startup to exit |
| Two eight-plane display bitmaps | 128,000 | Startup to exit |
| Race runtime, including maps/assets/render caches | 369,104 | Startup to exit |
| Common menu owner / track-load staging | 87,958 | Startup-reserved exclusive workspace |
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
   headers. Nested icon loading still has a separate 512-byte temporary.
3. **Track preparation (implemented):** 65,536-byte DAT, 8,192-byte track, 3,018-byte navigation,
   128-byte car and 2,048-byte font staging. Chunky already supplies decode
   workspace. Preparation now shares the inactive primary menu slot only after
   shop destruction, with explicit acquire/release ownership and a compile-time
   capacity check. No five staging allocations remain in `prepare_race`.
   Tracks' information preview now uses disjoint spans of idle VGA storage
   (65,536-byte decode arena, 65,536-byte DAT staging, 8,192-byte track staging).
   Its chunky parent and save-under remain untouched; leaving Tracks rebuilds
   the title VGA image. The small information dialog still allocates separately.
4. **Nested dialogs:** Help now borrows 110,096 bytes from the startup-owned
   117,760-byte particle visibility cache. The viewer and 64,000-byte parent
   snapshot have exclusive modal ownership. Racing is stopped during Help;
   release reconstructs the cache from unchanged terrain maps before any race
   update can resume. At initial title startup `race->started` is explicitly
   zero; no uninitialized terrain is read. Binding capacity is checked at
   compile time and startup, and no heap fallback exists. Picker, controllers,
   name/colour, messages, Change Cars and intermission storage still allocate
   dynamically. Work out legal nesting before defining
   unions; preserve parent save-under, labels and font state across child exit.
5. **Persistence/catalogues:** track-list refresh formerly allocated 65,536
   bytes and its loader another 65,536. The redundant loader buffer is now
   eliminated: refresh reads/validates its unpublished staging directly, then
   allocates an exact-size retained copy before releasing the previous copy.
   Reserve catalogue/transaction capacity at launch; bound dynamic name tables,
   profile editing, save/load and screenshot encoding without losing atomic
   new/backup file replacement or original data support.
6. **Registration/ending screens:** 64,003/70,000-byte image resource buffers,
   plus a menu surface. Reuse preparation scratch while preserving whichever
   screen is actually displayed during I/O.
7. **Startup-only ownership:** bitmaps/copper, PCM banks, compressed menu cache,
   fonts and main images can retain their existing startup allocation/exit-free
   pattern. Audit framework and OS calls separately, rather than routing Chip
   DMA data into a generic workspace.

### Validation requirements

- Workbench loaded, stock PAL 68020, 2 MiB Chip, no Fast, default 4 KiB stack.
- Track actual Exec allocation/free calls through menus, demo/races, intermission,
  saving and shutdown; distinguish OS allocations from game-owned ones.
- Test busy-slot rejection, startup failure and partial initialization cleanup.
- Repeat transitions and restart in the same OS session; no outstanding memory.
- Compare whole chunky/planar publications and retained parent screens; use
  emulator output for palette timing and Workbench flashes.
- Keep extended diagnostic-only allocations out of the normal-game budget.
