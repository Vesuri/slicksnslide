# Menu-resident asset inventory

Inventory date: 2026-09-28; implementation started 2026-09-29. The archive cache
and several menu owners are implemented; catalogue/lifecycle migration is not
complete. Evidence is in [menu-cache-verification.md](menu-cache-verification.md).
Goal: navigating menus must neither read disk nor restore the AmigaOS
display. Explicit save/load operations, individual track loading and exit/end
presentations may perform disk I/O.

## Load once at startup and retain

The following are named resources inside `SLICKS.000`, not separate filesystem
files. Sizes are encoded archive payload bytes. They are **not** decoded RAM
requirements or necessarily additional allocations: several assets already have
resident decoded representations which should be reused instead of duplicated.

| Consumer | Resources | Payload bytes |
| --- | --- | ---: |
| Title and original status indicators | `mainmenu.@I`, `partII`, `val1.@I`, `val2.@I`, `pel_on.@I`, `pel_ei.@I`, `pel_t.@I` | 64,968 |
| Shared fonts | `kirj.@f`, `pieni.@f`, `iso.@f` | 8,786 |
| Player setup and shared vehicle icons | `players.bmp`, `computer.@16`, `carimage16`, `auto01.@16` through `auto09.@16` | 28,096 |
| Track selector, preview palette and records | `trckmenu.@I`, `trckmenu.@p`, `peli.@p`, `top10cc.@16` | 65,619 |
| Help and language text | `HELP.TXT`, `lang1.txt` through `lang8.txt` | 15,568 |
| Controller setup | `ohj_key.@I`, `ohj_joy.@I`, `ohj_lptc.@I`, `keys_m1.@16` through `keys_m5.@16` | 1,441 |
| Shop | `tuning.@I`, `tuning.@p`, `vir00.@16` through `vir12.@16` | 47,201 |
| Intermission | `clock.@16` | 106 |
| **Total: 59 resources, counted once** | | **231,785 (226.35 KiB)** |

The five small title status resources are decoded at startup and used by the
live original status renderer. Their role/count/badge bindings and native
transitions are verified in `fidelity-audit.md` (F02).
Title/Arcade and pause/intermission table loads now honor positive saved
language IDs 1–8. Retaining all eight languages costs only 2,464 bytes
altogether. Original startup chooser/default handling and remaining label
callers are still under audit; this is not complete localization.

Options, name entry, colour picking and confirmation dialogs need shared
fonts/palettes and existing state, not another dedicated background file.
Shop, records and intermission share the same car icons; do not load them again.

### Startup state outside the archive

- `SLICKS.CFG` and `SLICKS.PLR`: retain parsed configuration/profile working
  state, as already done. Editing happens in RAM; persistence is explicit I/O.
- `SLICKS.REK`: validate at startup and keep the resulting registration state.
  Do not retain another copy of the private key merely for navigation.
- `SLICKS.TRK`: read the track-list catalogue at startup, retaining parsed
  entries and exact-sized source storage borrowed by its editor. The chooser
  does not reread it. Explicit save/delete boundaries refresh the snapshot
  after closing borrowed views, retaining transactional recovery checks.
- Track-name catalogue: retain startup discovery results for list browsing.
- Saved championships (`*.SSS`): discover names at startup and retain the
  chooser catalogue. Opening/reopening the chooser uses the startup snapshot;
  explicit save/delete/load boundaries refresh it with the OS available.
  Preserve the existing 40-entry limit and overflow/error reporting; do not
  silently truncate. Read the selected save only on confirmed load.
- Keyboard layout: the startup snapshot is implemented (8 qualifier variants
  by 128 raw keys, 1,024 bytes). Constructors share the immutable mapping;
  modifier/input state remains local to each dialog.

Successful saves/deletes must update the in-memory catalogues. Failed operations
must leave the previous catalogue/state usable and retain existing `.new`/`.bak`
recovery rules. External filesystem changes require an explicit refresh, not
hidden disk reads during ordinary navigation.

## Keep on demand, or discard after startup

| Resource | Recommended lifetime |
| --- | --- |
| `end1.bmp`, `end2.bmp` (65,078 bytes each) | Load only the selected exit presentation. |
| External `webf_ord.bmp` | Load on explicit order-form request, if supplied. It is absent from the supplied archive. |
| `sskuppi.@I` + `sskuppi.@p` (64,771 bytes) | Load for championship-end presentation. If that transition must also be disk-free, add these to the resident set: 61 resources, 296,556 bytes total. |
| `loading.bmp` (11,900 bytes) | Startup/expired-trial presentation; release when no longer needed. |
| Individual `TRACKS/*.SS` | Load selected track for a race or an explicit track-info/preview request. Merely moving through the track list must not read tracks. |
| `SLICKS.DAT` (41,500 bytes) | Track graphics for rendering/previews; load with the selected track, or retain after first use if the measured budget permits. Preview is a track-loading exception, not ordinary list browsing. |
| Gameplay sprites, vehicle data, masks, weapons/smoke/explosions | Race setup dependencies, not menu-navigation dependencies. Preserve whatever gameplay lifetime is required. |
| `samples.dat` (131,691 bytes), `intermed.wav` (18,852 bytes) | Already loaded into runtime audio allocations at startup. Keep playback data; do not additionally cache their source payloads. |

The current menu paths do not load `mainmenu.wav`, `slix_sw.bin`, `slix_reg.bin`,
`autoinfo.@p` or the large `car1`…`car10` images. Excluding them from this cache
does not prove they are unused by every original-game feature.

## RAM and ownership constraints

Arcade title presentation now retains its own decoded `pieni.@f` allocation
(8,192-byte capacity) and a 2,048-byte decoded language table at startup.
These are additional to the encoded cache. No modal/font/language reads occur
while drawing this title. Release memory gates must be refreshed for this
ownership change; the historical measurements below do not include it.

Do not cache the whole 642,007-byte archive. The current release measurements
leave 345,872 bytes of Chip RAM free at title/race, 259,640 in options and
149,688 in help; the largest free block in help is only 99,352 bytes. These are
individual checkpoints, not an exhaustive minimum-free-memory test.
The 226 KiB inventory cannot simply be added on top of every current allocation.

- Reuse the resident title background and fonts. The 64,003-byte encoded title
  staging buffer is unnecessary in normal gameplay after decoding (diagnostic
  audits are a separate lifetime).
- Retain encoded backgrounds where useful, decoding into a shared menu
  workspace rather than keeping every 64,000-byte decoded screen.
- Share immutable font data, but preserve per-surface mutable font colours.
  Indexed sprite conversion must respect the active palette; globally caching
  one palette-specific decoding can introduce wrong colours.
- Preallocate/reuse modal storage. The current common menu object is 86,226
  bytes and help viewer 109,950 bytes, before other allocations. Sharing help
  text/index storage and saved-screen storage needs explicit ownership.
- Budget decoded resources, archive metadata, alignment, saved screens and
  transient track/load buffers, not just source byte counts. Verify on 2 MiB
  Chip RAM with no Fast RAM and the default 4 KiB process stack.

## Implementation and acceptance boundaries

An in-memory resource provider must cover every menu resource request, including
archive opening/lookup: a cache miss during ordinary navigation must report a
bug rather than silently fall back to disk. Loading resources is only half the
change. Existing menu open/close paths also call platform end/begin around
allocation, destruction and redraw even when no file is read. Remove those
handoffs for RAM-only transitions once workspace ownership is safe. Do not
simply enable multitasking while custom interrupts/display/audio remain owned.

Trace ordinary navigation across title, players/Add/Edit/name/colour, options,
controllers, help, track lists, saved-game chooser, pause, shop and intermission.
Verify no archive/file reads, directory enumeration, keymap-library work or
platform teardown occurs for navigation. Explicit save/load/refresh and track
preview operations remain identifiable exceptions. Compare rendered output and
cancellation/repeated editing, exercise save failures/recovery, and measure the
worst modal/transient memory usage before accepting the cache.

## Inventory evidence

Resource names and sizes were read from the supplied archive directory; no
original payload is included here. Call paths audited:

- `src/platform/amiga/amiga_player_menu.c`: menu constructors, fonts/icons,
  controller graphics, help/languages, keymap preparation and track-list picker.
- `src/platform/amiga/amiga_shop.c`: shop background, palette and sprite loader.
- `src/platform/amiga/slicks_diag.c`: title, menu handoffs, pause/intermission,
  track preview, championship results, startup audio and exit presentation.
- `src/platform/amiga/amiga_saved_files.c` and `amiga_setup_storage.c`:
  directory discovery and persistent state.

The older memory figures above are pre-cache checkpoints. Current target
measurements and remaining integration boundaries are recorded in the separate
verification document and `open-work.md`; they are not whole-game peak bounds.
