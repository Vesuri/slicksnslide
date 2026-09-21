# First Amiga execution skeleton

The repository now builds a bootable Amiga HUNK and runs it in FS-UAE as an
A1200 with a 68020, exactly 2 MiB of chip memory, and no fast memory. This is a
platform diagnostic rather than a C transliteration of the DOS program: C is
used only for the temporary Amiga OS/display shell, while the graphics operation
under test is the same hand-written 68020 `sgfx_plot_plane` routine exercised by
the x86-versus-M68k differential suite.

## What the diagnostic proves

`SlicksDiag` allocates the 256 KiB logical VGA store, invokes the native opaque
planar blitter with a call-time capture of the BASIC.SS title frame, converts
the result to eight Amiga bitplanes, installs the captured 256-colour VGA
palette, opens a 320 by 200 AGA screen, and displays the result. The target-side
GDB check stops at a named post-display marker and verifies checksum
`0e00b1da`.

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
native graphics ABI, one application-level control-flow translation, and an
observed BASIC-path call using original game data on the actual emulated target
configuration. The original title-frame blob is identified by FNV-1a hash
`a4fc8a1cbea08a30`; its call-time palette is
`9b17b223ef7f93e3`. The ignored capture bundle is extracted during the build,
so no original game bytes are committed.

The diagnostic still does not enter the original game loop. The next vertical
slice is translating the observed caller around runtime offset `19711h` so the
frame is reached through native control flow rather than invoked by the
temporary platform shell.
