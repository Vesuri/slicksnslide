# First Amiga execution skeleton

The repository now builds a bootable Amiga HUNK and runs it in FS-UAE as an
A1200 with a 68020, exactly 2 MiB of chip memory, and no fast memory. This is a
platform diagnostic rather than a C transliteration of the DOS program: C is
used only for the temporary Amiga OS/display shell, while the graphics operation
under test is the same hand-written 68020 `sgfx_plot_plane` routine exercised by
the x86-versus-M68k differential suite.

## What the diagnostic proves

`SlicksDiag` allocates the 256 KiB logical VGA store and invokes the translated
title-page caller, which feeds a call-time capture of the BASIC.SS title frame
through the native opaque blitter to both original VGA pages. The platform
shell converts the visible page to eight Amiga bitplanes, installs the captured
256-colour VGA palette, opens a 320 by 200 AGA screen, and displays the result.
The target-side GDB check stops at a named post-display marker and verifies
checksum `37048854` after the native title menu and status indicators are drawn.

The VGA representation is the unchained 256-colour layout used by the game:

```text
plane  = x & 3
offset = screen_base + y * 100 + (x >> 2)
pixel  = vga_plane[plane][offset]       # one complete 8-bit colour index
```

Those are four byte-interleaved VGA banks, not four Amiga bitplanes. An Amiga
bitplane stores one bit of every pixel. The diagnostic makes that boundary
explicit by converting the 8-bit indices in the logical VGA store to eight
real AGA bitplanes once per displayed frame. A later renderer can optimize or
replace this conversion without changing translated drawing semantics.

## Commands

From the repository root:

```sh
make amiga          # build amiga/out/SlicksDiag.exe
make amiga-run      # show the diagnostic; mouse button or Escape exits
make amiga-debug    # open the target under the M68k GDB stub
make amiga-check    # boot it and verify the displayed-frame marker/checksum
```

`amiga/env.sh` selects the shared toolchain, FS-UAE, and default Kickstart path,
following the conventions of the existing ports. Generated objects, emulator
state, mounted scratch disks, maps, and debugger files remain ignored.

An AmigaOS `SetPatch` binary must be present at `tmp/SetPatch`. It remains
ignored and is copied to the generated boot volume as `C:SetPatch`; both launch
paths run it before opening the eight-bitplane AGA screen.

## Current boundary

This milestone proves the complete host-build-to-target-display path, the
native graphics ABI, and application-level control-flow translations including
an observed BASIC-path caller using original game data on the actual emulated
target configuration. The translated `195F0h..19639h` block submits the title
image to both original VGA page bases through the native blitter. The observed
`196ECh..19718h` redraw block then restores its fixed `100 by 97` crop through
the native sub-rectangle blitter. That redraw now executes as part of the
translated `19653h..19718h` UI prefix, including its three palette searches and
animation-counter update. The original title-frame blob is identified by
FNV-1a hash
`a4fc8a1cbea08a30`; its call-time palette is
`9b17b223ef7f93e3`. The ignored capture bundle is extracted during the build,
so no original game bytes are committed.

The diagnostic still does not enter the original game loop. The six-entry loop
at `19719h..19825h` is represented by native bevel and text rendering. The
observed BASIC-path status slice at `19828h..199F9h` now follows it: four exact
call-time planar indicator sprites are drawn at their measured positions and
the two measured counters both show `1`; the three conditional badge branches
are proved inactive on this path. Control returns to the observed shared update
suffix at `19E4Ah`; the renderer at `199FAh` is the wrapper's alternate,
unreached branch. The current 5x7 title and numeric font is intentionally
compact; replacing it with the recovered original font remains a
visual-fidelity task.
