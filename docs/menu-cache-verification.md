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
