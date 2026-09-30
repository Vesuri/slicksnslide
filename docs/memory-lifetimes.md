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
| Common menu owner | 86,422 | Currently allocated for each menu |
| Help viewer | 46,096 | Nested modal lifetime |
| List/profile picker | 35,132 | Nested modal lifetime |
| Intermission dialog | 4,228 | Between races |
| Track navigation decode staging | 3,018 | Track preparation |

The first five total 888,850 bytes, excluding code/data/BSS, audio, cached menu
resources, fonts, catalogues, copper lists, library allocations and AmigaOS.
The older DEMOPLR allocation audit (`tmp/standalone-release-vjqrkoq0/debug.log`)
peaks at 1,337,664 bytes of directly tracked game allocations and ends at zero.
That is one tested path, not the program's worst-case total RAM requirement;
it excludes the loader's executable segments and OS-owned memory.

The common menu owner includes a 64,000-byte saved page and three 6,000-byte
font arrays. These dominate small menus too. Nested Help can add a separate
64,000-byte save-under. Never sum all dialog sizes as though all were live
together, but never assume nested dialogs can overwrite their parent.

### Confirmed avoidable peak

Intermission previously allocated a further 65,536-byte DAT preview arena.
On Workbench-loaded stock 2 MiB this failed after successfully allocating the
41,500-byte DAT, 2,011-byte next track, menu and dialog. Preview now borrows
the first 64 KiB of the startup-owned VGA image: the completed race cannot
resume, the parent/retry display is retained in chunky, and next-race or title
preparation reconstructs VGA contents. No displayed pixels or live race state
are used as preview output. The arena API explicitly borrows rather than owns.

### Remaining allocation families and reservation design

1. **Primary menu owner:** Players, Tracks, Options, Help, pause, shop, records,
   intermission and endings use the same large owner type. Prove exclusive
   primary ownership, reserve one slot at startup, and zero/reinitialize on
   acquisition. Nested children need their own slots. Never silently fall back
   to AllocMem if a slot is unexpectedly busy.
2. **Menu construction resources:** 8/32/64 KiB temporary decode allocations.
   Reuse the owner's save-under before its parent snapshot becomes live where
   safe; otherwise use a shared scratch reservation. Track background input is
   64,003 bytes, so a 64,000-byte save-under is insufficient as-is.
3. **Track preparation:** 65,536-byte DAT, 8,192-byte track, 3,018-byte navigation,
   128-byte car and 2,048-byte font staging. Chunky already supplies decode
   workspace. Share preparation storage with an inactive primary menu slot only
   after shop destruction, with explicit acquire/release ownership.
4. **Nested dialogs:** fixed Help, picker, controllers, name/colour, messages,
   Change Cars and intermission storage. Work out legal nesting before defining
   unions; preserve parent save-under, labels and font state across child exit.
5. **Persistence/catalogues:** track-list refresh allocates 65,536 bytes, then
   its loader allocates another 65,536, then it allocates an exact-size retained
   copy before releasing the previous copy. Eliminate redundant staging first.
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
