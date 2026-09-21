# First Amiga execution skeleton

The repository now builds a bootable Amiga HUNK and runs it in FS-UAE as an
A1200 with a 68020, exactly 2 MiB of chip memory, and no fast memory. This is a
platform diagnostic rather than a C transliteration of the DOS program: C is
used only for the temporary Amiga OS/display shell, while the graphics operation
under test is the same hand-written 68020 `sgfx_plot_plane` routine exercised by
the x86-versus-M68k differential suite.

## What the diagnostic proves

`SlicksDiag` allocates the 256 KiB logical VGA store, draws a deterministic
pattern through the native plot helper and the first translated Slicks caller,
converts it to four Amiga bitplanes, opens a 320 by 200 screen, and displays the
result. The target-side GDB check stops at a named post-display marker and
verifies the checksum `86bdc061`.

The VGA representation is the unchained 256-colour layout used by the game:

```text
plane  = x & 3
offset = screen_base + y * 100 + (x >> 2)
pixel  = vga_plane[plane][offset]       # one complete 8-bit colour index
```

Those are four byte-interleaved VGA banks, not four Amiga bitplanes. An Amiga
bitplane stores one bit of every pixel. The current 16-colour diagnostic makes
that boundary explicit by converting the logical VGA store to four real Amiga
bitplanes once per displayed frame. A later renderer can optimize or replace
this conversion without changing translated drawing semantics.

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

## Current boundary

This milestone proves the complete host-build-to-target-display path, the
native graphics ABI, and one application-level control-flow translation on the
actual emulated target configuration. The block is Slicks' recovered
checker-pattern rectangle routine at runtime offset `A498h`; it calls the
native pixel helper and passes 256 whole-framebuffer comparisons against the
original x86 code. The current BASIC trace has not reached this particular
caller, so the next slice must come from the observed game path.
The diagnostic still does not enter the original game loop or display a full
original Slicks frame. That is the next vertical-slice boundary.
