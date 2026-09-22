# First Amiga execution skeleton

The repository now builds a bootable Amiga HUNK and runs it in FS-UAE as an
A1200 with a 68020, exactly 2 MiB of chip memory, and no fast memory. This is a
platform diagnostic rather than a C transliteration of the DOS program. The
game operations under test are native 68020 routines; a small C/C++ platform
coordinator connects them to the shared dA JoRMaS Amiga framework.

## What the diagnostic proves

`SlicksDiag` allocates the 256 KiB logical VGA store, runs the native equivalent
of the observed mode-zero setup with its 400-pixel virtual width, opens the
original `SLICKS.000` archive through AmigaDOS, and resolves `mainmenu.@I` and
`partII` by their native archive directory entries. A hand-written 68020 pass
converts the original 320 by 200 chunky image resource into the game's
four-bank layout; the translated title caller then blits it to both original
VGA pages and draws the native menu overlays. The platform layer converts the
visible page to eight Amiga bitplanes and displays it as a native 320 by 200
AGA playfield.
The conversion is native 68020 assembly: a small staging pass interleaves the
visible pixels from the four VGA banks into a 64,000-byte chunky buffer, then
Mikael Kalms' Public Domain `c2p1x1_8_c5_bm` CPU5 routine writes directly to
the plane pointers in a 320-byte-row interleaved `BitMap` adapter. The initial scene uses
that full staging pass once. Live race writes mirror changed pixels into the
chunky surface, allowing later frames to call Kalms directly without another
64,000-pixel VGA deinterleave. The target-side GDB
check stops at a named post-display marker and verifies logical checksum
`93c8bea6` and planar display checksum `29592c57` after the currently translated
title menu overlays are drawn.

There is no Intuition screen or window in the active display path. Before
takeover, Slicks loads both the title and race resources and prepares two
chip-memory views. The DanceDiverse3 framework's `Bitmap` describes each
eight-plane interleaved allocation; `CopperList::showBitmap` emits its eight
plane pointers, `CopperList::setPlayfield` emits the matching 280-byte
`BPL1MOD`/`BPL2MOD`, and `setPalette24Bit` emits the AGA palette banks. The
200-line display window spans raster rows `$38..$ff`; its exclusive `$100`
stop matches the ninth vertical-stop bit emitted in `DIWHIGH`. The
platform then takes over copper DMA and the vertical-blank vector directly.
It restores the original interrupt vector, DMA/interrupt masks, copper list,
and OS view on exit.

The VGA representation is the unchained 256-colour layout used by the game:

```text
plane  = x & 3
offset = screen_base + y * 100 + (x >> 2)
pixel  = vga_plane[plane][offset]       # one complete 8-bit colour index
```

Those are four byte-interleaved VGA banks, not four Amiga bitplanes. An Amiga
bitplane stores one bit of every pixel. The diagnostic makes that boundary
explicit by converting the 8-bit indices in the logical VGA store to eight
real AGA bitplanes once per displayed frame. Keeping the conversion behind this
boundary allows translated drawing code to preserve the DOS representation
without paying for a C pixel-by-pixel conversion.

## Commands

From the repository root:

```sh
make amiga          # build amiga/out/SlicksDiag.exe
make amiga-run      # show the diagnostic; mouse button or Escape exits
make amiga-debug    # open the target under the M68k GDB stub
make amiga-check    # boot it and verify the displayed-frame marker/checksum
make amiga-race-check # enter BASIC.SS and verify its asset-built race frame
make amiga-track-check # run BASICTRK.SS through the generalized race path
make amiga-lap-check  # prove a complete checkpoint/lap wrap
make amiga-restore-check # prove readable OS state is restored on exit
```

`amiga/env.sh` selects the shared toolchain, FS-UAE, and default Kickstart path,
following the conventions of the existing ports. Generated objects, emulator
state, mounted scratch disks, maps, and debugger files remain ignored.

An AmigaOS `SetPatch` binary must be present at `tmp/SetPatch`. It remains
ignored and is copied to the generated boot volume as `C:SetPatch`; both launch
paths run it before taking over the eight-bitplane AGA display.

## Current boundary

This milestone proves the complete host-build-to-target-display path, the
native graphics ABI, and application-level control-flow translations including
an observed BASIC-path caller using original game data on the actual emulated
target configuration. The translated `195F0h..19639h` block submits the title
image to both original VGA page bases through the native blitter. The observed
`196ECh..19718h` redraw block then restores its fixed `100 by 97` crop through
the native sub-rectangle blitter. That redraw now executes as part of the
translated `19653h..19718h` UI prefix, including its three palette searches and
animation-counter update. No decoded title framebuffer or call-time sprite is
linked into the executable: the original archive remains on the mounted Amiga
volume and is parsed at runtime.

The native Return path now leaves the title and constructs the first genuine
`BASIC.SS` course frame. It reads `SLICKS.DAT` and `TRACKS/BASIC.SS` from the
mounted original files, decodes 110 compressed images, applies their four
orientations to 233 track records, installs `peli.@p`, and presents the result.
The strict race gate reports logical checksum `815c70ca` and display checksum
`024f572b`. No captured DOS frame is linked or loaded.

The diagnostic now runs a repeating four-car race/update loop after the title.
During live racing, each painter records its touched row interval in a fixed
list. Overlapping intervals are merged and only those full-width spans are
passed to Kalms; no shadow framebuffer or 200-row flag array is used. The
200-frame BASIC gate converts 3,418 row-widths rather than 40,000 while
retaining the exact prior display checksum.
Its pre-race sequence displays the four original `lahto*.@I` start-light
frames at `(164,31)` using the cadence measured from the DOS capture, then
restores the underlying course before cars move.
The same loader and runtime also run the original `BASICTRK.SS` editor/template
course: all 262 scene objects and 25 navigation regions are consumed, and its
zero-valued optional speed hints fall back to the normal cruise speed.  The
strict alternate-track gate proves all four cars advance through multiple
regions, produce skidmarks, and avoid spurious boundary contacts for 200
frames on the 2 MiB A1200 configuration.
Its HUD timer decodes `pieni.@f` directly from the original archive: the font's
character map, variable widths, fixed five-pixel glyph height, and raw bitmap
payload are consumed without converted or captured assets. The six-entry loop
at `19719h..19825h` is represented by native bevel and text rendering. The
observed BASIC-path status slice at `19828h..199F9h` now follows it: four exact
call-time planar indicator sprites are drawn at their measured positions and
the two measured counters both show `1`; the three conditional badge branches
are proved inactive on this path. The observed shared update suffix at
`19E4Ah..19EFEh` is native as well: it advances the title phase, updates the
pulsing colour slot, skips the inactive optional text, and maps the final VGA
start-address update to an Amiga no-op. The renderer at `199FAh` is the
wrapper's alternate, unreached branch. The title renderer still uses its
compact native font and remains a separate visual-fidelity task.

The title screen's host input boundary now feeds Amiga raw keys through the
native eight-key DOS scan-code classifier. Escape/F10 and
Return/Space/mouse activation follow the original cancel and activation
classes; deeper menu actions remain the next control-flow slice.
