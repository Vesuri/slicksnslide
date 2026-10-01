# Memory

The port targets a PAL A1200 with 2 MiB Chip RAM, no Fast RAM and the
default 4 KiB Shell stack, loaded from Workbench. WHDLoad adds no expansion
memory (`FASTMEMSIZE = 0`), so the game runs in the same 2 MiB Chip-only
space there as it does standalone (see [whdload.md](whdload.md)).

## Goal

Navigating menus never reads the disk and never restores the AmigaOS display.
Everything a menu needs is resident from startup: the decoded title image,
fonts, language tables, the encoded menu resource cache, the track-name and
track-list catalogues, the saved-championship catalogue and the keymap
snapshot. Disk access happens only at explicit boundaries: race track loading,
the Tracks information preview, intermission next-track preview, save/load/
delete, track-list save, registration, trophy and exit presentations.

All game-owned memory is reserved once at startup. After the startup-complete
checkpoint (`slicks_diag_startup_complete`) the normal game makes no direct
Exec allocation; every later lifetime borrows a reserved span. A private heap
would not give this guarantee, so storage is reserved by simultaneous lifetime
and each borrower is owner-tagged. OS internals (DOS handles, locks,
libraries) still allocate on their own; their failures stay recoverable I/O
errors and are not guaranteed away.

## Startup reservation contract

`main` in `src/platform/amiga/slicks_diag.c` reserves in this order. Order
matters: the large menu block must be taken while the released startup
scratch is still contiguous, before the menu cache's many small blocks.

| Storage | Bytes | Memory | Notes |
| --- | ---: | --- | --- |
| Two display bitmaps | 2 × 64,000 | Chip | Eight planes, 320×200 (`amiga_platform.cpp`) |
| Two copper lists | 2 × 2,232 | Chip | 558 longs each |
| BitMap headers, C++ wrappers | small | any | Bitmap/CopperList wrappers via `framework_runtime.cpp` |
| SLICKS.000 directory | 19 × entries | any | First open, adopted as a retained reservation |
| Title background frame | 64,034 | any | Prepared title plus 32-byte C2P lookahead |
| Title fonts (`iso`, `kirj`, `pieni`) | ≤ 3 × 8,192 | any | Exact decoded sizes |
| VGA logical image | 262,144 | any | Also idle-phase staging, see below |
| Chunky image / decode workspace | 65,568 | any | 64 KiB plus C2P lookahead |
| Race runtime | 369,104 | any | Maps, assets, render caches; modal overlay |
| Track-name catalogue | 12 × capacity | any | Grows 256 → … → 10,000 during discovery only |
| Track playlist | 2 × (tracks + 2) | any | Only when more than 256 tracks |
| PCM sample banks, silence | data-dependent | Chip | Converted from `samples.dat`, `intermed.wav` |
| Menu workspace | 96,546 | any | Primary menu owner + parent union |
| Track-list catalogue | 65,536 | any | Full-capacity SLICKS.TRK snapshot |
| Menu resource cache | data-dependent | any | Owner, 57 entries, one block per resource |

Startup-only scratch is freed before the startup-complete checkpoint: the
64,003-byte title asset staging (title, status icons, fonts, language), the
131,691-byte `samples.dat` decode input, discovery's sort order, and the
setup loader's profile buffers. The title asset is freed immediately before
the menu workspace is reserved.

Bytes marked fixed come from code (`sizeof` on the 68020 target, defines or
`_Static_assert`s), not host ABI sizes. The total resident amount depends on
the track count and archive contents; measure it rather than summing this
table (see *Checking headroom*).

### Menu resource cache

`slicks_resource_cache_create` (`resource_archive.c`) loads the 57 names in
`menu_resources.h`, largest first. Each resource is read in one Read, staged in
the 64 KiB track-list storage, which has not yet been loaded at that point.
The staged bytes are packed when that is smaller and copied into an exact-size
block. `mainmenu.@I` and `partII` are not cached: their decoded forms already
have permanent owners. After the cache is built, SLICKS.TRK is loaded into the
same storage. Archive opens after startup reuse the retained directory
reservation and never reread or reallocate it.

## Reservation families

**Startup to exit.** Display bitmaps, copper lists, VGA image, chunky image,
title frame, fonts, race runtime, PCM banks, catalogues, menu cache, archive
directory and menu workspace. Everything is freed once at shutdown.

**Primary menu slot** (`struct SlicksAmigaPlayerMenu`, 87,958 bytes inside the
96,546-byte menu workspace). Players, Tracks, Options, Help, pause, shop,
records, intermission and endings all acquire this one exclusive slot; it is
cleared on acquisition. It holds a 65,536-byte saved page, the fonts and icon
pixels. Bytes 64,000–64,511 of the saved page, beyond the live 64,000-pixel
parent snapshot, stage nested icon decodes. Constructors may borrow the whole
saved page before the snapshot becomes live.

**Parent union** (8,588 bytes, same block). Pause, intermission and the
track-list workflow never coexist with each other, but do coexist with a
child. The parent tag must be released before the primary slot.

**Modal overlay** (`struct SlicksAmigaHelpWorkspace`, 110,096 bytes). Bound at
startup over the race runtime's 117,760-byte particle visibility cache, which
is derived from the immutable terrain maps. One child at a time: Help with
its 64,000-byte save-under, Controllers, name, colour, pickers with their
filename/title-offset payload, Track Information, Change Cars, messages, and
storage leases. Racing is stopped while it is borrowed; release rebuilds the
particle cache before any race update. At first title startup no race exists
and nothing is rebuilt.

**Storage leases** (same overlay, `MODAL_STORAGE`, taken only after all
children close and released before any warning opens):

| Operation | Bytes |
| --- | ---: |
| Setup save (CFG + PLR) | 5,771 |
| Track records read/write, Clear Records | 8,192 |
| Track-list refresh or save | 65,536 |
| Championship save (exact encoded size) | ≤ 80,218 |
| Championship load | 2,266 |
| Screenshot capture | 65,078 |
| Trophy image staging | 64,003 |
| Registration image input | 70,000 |

**Race preparation.** `prepare_race` borrows the primary slot after the shop
is destroyed: DAT 65,536, track 8,192, navigation, car 128 and font 2,048
bytes, decoding into the chunky workspace. A `_Static_assert` checks the fit.

**Tracks information preview.** The title VGA image is idle while Tracks owns
chunky and its saved parent: decode arena [0, 64 KiB), DAT [64 KiB, 128 KiB),
track [128 KiB, 136 KiB). Leaving Tracks rebuilds the title VGA image.

## Intermission staging layout

A completed race never resumes, so intermission borrows its 262,144-byte VGA
image as `union IntermissionWorkspace` (141,312 bytes). During preview
construction:

| Span | Use |
| --- | --- |
| [0, 65,536) | Decode output |
| [65,536, 131,072) | SLICKS.DAT input |
| [131,072, 139,264) | Next track input |
| [139,264, 141,312) | Decoded language table |

Once the renderer has copied labels and names into the retained parent, the
same union holds up to 10,000 exported eight-byte track names (80,000 bytes)
for the save workflow. The encoded championship goes to a storage lease in
the modal overlay, so it never overwrites those names. Chunky, both displayed
bitmaps, the primary saved page and the modal overlay never alias this union.
The next race or the title rebuilds VGA.

## Deliberate caches

Several large race buffers exist for speed, not by accident. Changing or
removing one needs speed and fidelity measurements.

- Particle visibility lookup, 117,760 bytes: rebuilt from the terrain maps;
  also serves as the modal overlay.
- Material and surface maps, 2 × 60,800 bytes: built before racing, read-only
  during it.
- Track draw packets 19,200, car render cache 14,996 and track sprite
  visibility 8,704 bytes: derived only from immutable sprites and maps.
- Stationary track-object cache: a 100-byte validity array beside the
  permanent handle array, inside the shadow-checked working state. It skips
  material sampling and actor reconfiguration for objects that stayed
  stationary since a fully processed stationary pass
  ([rendering.md](rendering.md)).
- Prepared title frame, decoded fonts and the keymap snapshot (1,024 bytes):
  keep title drawing and menu input free of disk, library and decode work.

Do not cache the whole SLICKS.000 archive; it does not fit beside these.

## When memory runs out

Startup reservations are all-or-nothing. A failed allocation jumps to the
common cleanup, frees every block already taken and returns 20 before the
startup-complete checkpoint, so no menu ever runs partially reserved. The
three likeliest failures print a reason: `insufficient memory for menu and
track workspace`, `insufficient memory for track-list storage`, and `no
tracks found or insufficient memory for the track catalogue`; a large
catalogue's playlist failure prints `insufficient memory for the track
selection`.

After startup there is no heap fallback. A busy or oversized acquisition of
the menu, parent or modal slot returns null and increments
`g_slicks_menu_workspace_conflicts`. The caller treats that as an error.
Storage adapters reject missing or short scratch with `ERROR_NO_FREE_STORE`
before touching the disk. A failed track-list refresh keeps the old catalogue
bytes but records the failure, so stale data is never browsed silently.

## Checking headroom

The allocation audit drives Exec AllocMem/FreeMem breakpoints from the host
(`tools/memory_audit.py`) through the standalone release harness:

```sh
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMOPLR --allocation-audit
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args PLAYERSV --allocation-audit --startup-only
python3 tools/test_standalone_release.py PRIVATE_INSTALL --workbench --args DEMORET --allocation-audit --chip-memory 1024 --expect-failure --marker ALLOCATION_CLEANUP_OK
```

- `--allocation-audit` logs every game-owned allocation and free and checks
  the default stack. It fails on outstanding blocks or workspace conflicts at
  exit. `--repeat N` relaunches in the same OS session.
- `--startup-only` additionally reports `STARTUP_RESERVATIONS_READY blocks=
  bytes=` (the resident startup total) at the checkpoint and fails on any
  game-owned allocation after it. OS-internal allocations are excluded by
  caller address.
- `--chip-memory 1024 --expect-failure` proves that a failed reservation is
  cleaned up.
- `amiga/diag_menu_cache.gdb` prints the resident cache size
  (`g_slicks_menu_cache_bytes`). `diag_catalogue_memory.gdb` samples free and
  largest-block memory around race preparation with a maximal catalogue.

Diagnostic-only allocations (PC sampler, shadow and retention comparisons,
pause and intermission construction fixtures, the demo playlist snapshot)
are outside the normal-game contract and never run in ordinary play.
