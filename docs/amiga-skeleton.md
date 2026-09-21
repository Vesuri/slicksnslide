# First Amiga execution skeleton

The repository now builds a bootable Amiga HUNK and runs it in FS-UAE as an
A1200 with a 68020, exactly 2 MiB of chip memory, and no fast memory. This is a
platform diagnostic rather than a C transliteration of the DOS program: C is
used only for the temporary Amiga OS/display shell, while the graphics operation
under test is the same hand-written 68020 `sgfx_plot_plane` routine exercised by
the x86-versus-M68k differential suite.

## What the diagnostic proves

`SlicksDiag` allocates the 256 KiB logical VGA store, draws a deterministic
pattern through the native plot helper, converts it to four Amiga bitplanes,
opens a 320 by 200 screen, and displays the result. The target-side GDB check
stops after the screen is ready and verifies the checksum `bb0e7f26`.

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

This milestone proves the complete host-build-to-target-display path and the
native graphics ABI on the actual emulated target configuration. It does not
yet execute a statically translated x86 basic block or display an original
Slicks frame. The next useful vertical slice is a translated control block that
calls the native graphics ABI and reaches a reference-derived visual checkpoint.
