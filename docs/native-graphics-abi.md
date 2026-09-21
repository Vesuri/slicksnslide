# Native 68020 graphics ABI

The first native seam replaces proved VGA helper calls directly. It is not a
CPU-context API: translated blocks pass live values in registers and branch to
the helper with an ordinary `bsr`/`jsr`.

The initial correctness representation stores the four 64 KiB VGA planes
consecutively. This preserves the original 16-bit plane offsets and makes the
existing planar sprite formats byte-exact. It costs 256 KiB and leaves the
eventual Amiga display conversion as a separate, measurable design choice.
Changing to a chunky working surface later is allowed only behind the same
semantic tests and target measurements.

Common register contract:

| Register | Meaning |
|---|---|
| `a0` | Base of four consecutive 64 KiB logical VGA planes |
| `d0.w` | Unsigned guest x coordinate |
| `d1.w` | Unsigned guest y coordinate |
| `d2.b` | Pixel value for writes |
| `d3.w` | Guest page/screen base offset |
| `d4.w` | Guest byte stride |
| `d5-d6` | Scratch, caller-clobbered |

`sgfx_plot_plane` writes `d2.b` to plane `d0.w & 3` at the low 16 bits
of `d3.w + d1.w * d4.w + (d0.w >> 2)`.

`sgfx_plot` implements the adjacent helper whose VGA write plane was selected
by its caller. The native ABI makes that implicit hardware state explicit in
`d5.w`; all other inputs and the wrapped address calculation match
`sgfx_plot_plane`. It preserves `d0-d5` and `a0`, clobbering only `d6-d7`.

`sgfx_read_pixel` reads the same location and returns a zero-extended byte in
`d0.l`. Both routines deliberately preserve the 286's unsigned logical shift
and 16-bit address wrap. They do not load or store an emulated CPU structure.

`sgfx_planar_blit` and `sgfx_transparent_blit` additionally receive `a1`
pointing at the proved sprite format: byte width, byte height, then four plane
payloads of `width * height` bytes each. Source plane zero begins at the
destination phase `x & 3`; later source planes advance that phase and carry
into the destination byte offset exactly as the VGA sequencer rotation did.
The transparent form skips source bytes equal to zero. These larger helpers
clobber `d0-d3/d5-d7` and `a1-a5`, preserving the plane base in `a0` and stride
in `d4`. Zero dimensions are excluded by the measured source-buffer contract.

`sgfx_readback` receives pixel width in `d2.w`, height in `d5.b`, and the
destination sprite-buffer pointer in `a1`; the other inputs retain their common
meanings. It writes the two-byte `ceil(width/4), height` header, four rotated
plane payloads, and the trailing right-edge padding count `(-width) & 3`.
Starting `x & 3` selects the first source plane and carries into the 16-bit
source offset as the four planes rotate. The helper preserves `a0` and `d4`
and otherwise uses the larger-helper clobber set above.

`sgfx_planar_subrect_blit` uses `d0.w,d1.w` for destination x/y, `d2.w,d3.w`
for source x/y, `d5.w` for pixel width, `d6.b` for height, and `d7.w` for the
screen base; `a0`, `a1`, and `d4` keep their common meanings. Matching the
original routine, source x is truncated to a byte offset and source x/y are
also added to the destination address. Every row re-applies the source-x byte
offset, while the end of each plane skips the uncopied source rows. Source
bounds and nonzero dimensions remain caller contracts. The routine preserves
`a0` and `d4` and clobbers `d0-d3/d5-d7` and `a1-a6`.

The `verify-native-graphics` gate runs the original unpacked x86 helper bytes
and these assembled 68020 routines in independent Unicorn engines. It compares
the VGA plane selected by the original port write, the wrapped address, the
returned byte, and the write side effect over deterministic edge cases and
random states. The pixel helpers pass 2,040 paired states; each sprite blitter
passes 256 states with the complete four-plane 256 KiB destination compared
after every call. Readback passes 256 states with its complete 64 KiB
destination segment compared after every call. Sub-rectangle copy passes 256
states with the complete four-plane destination compared after every call.
The direct plot helper passes 512 states spanning all four caller-selected
planes.

`sgfx_title_pages` is the first translated caller from the observed BASIC.SS
path. It corresponds to runtime offsets `195F0h..19639h` and invokes
`sgfx_planar_blit` twice at `(0,0)`, preserving the source pointer between the
calls. The two page bases arrive in `d5.w` and `d6.w`; the trace observed
`7FBCh` followed by `0000h`. The caller preserves `d2-d7/a0-a6` for the
temporary platform boundary and uses the native register ABI internally.

The differential gate executes the original relocated x86 caller—including
both far calls and its global source/page loads—and the composed 68020 caller
plus blitter. It passes the exact 320 by 200 title-frame case and 32 randomized
page, size, overlap, and 16-bit-wrap cases.

`sgfx_title_crop` translates the next observed redraw block at
`196ECh..19718h`. It invokes `sgfx_planar_subrect_blit` with the measured fixed
arguments `(dest 0,0; source 110,77; size 100 by 97)` and accepts the current
page base in `d7.w`. The exact relocated x86 caller and composed 68020 code
agree for both observed pages and 32 randomized legal source/page states. The
routine is linked into the A1200 path after the two-page initializer; restoring
the crop from the same image intentionally leaves checksum `87956515`
unchanged.

`sgfx_checker_fill` is the first translated application-level caller rather
than an isolated VGA primitive. It corresponds to a recovered Slicks routine at
runtime-image offset `A498h`; the bounded BASIC trace has not reached this
caller, although it calls the heavily exercised live plot primitive. Its signed
half-open nested loops toggle a byte at every visited coordinate and call
`sgfx_plot_plane` on alternating pixels. The
native entry receives `x0,y0,x1,y1` in `d0.w-d3.w`, the colour in `d4.b`, the
screen base in `d5.w`, the stride in `d6.w`, and the plane store in `a0`.

The checker routine passes 256 additional whole-framebuffer differentials,
including empty and reversed signed ranges and both observed VGA pages. Unlike
the earlier leaf tests, its original x86 side runs the captured image at the
real DOS load address so its relocated far call reaches the original pixel
helper. The M68k side uses a mapped stack and executes the native caller and
callee together. At the temporary C platform boundary the routine preserves
`d2-d7/a0-a6`; future translated-to-translated calls can adopt a narrower
preservation contract once the block register allocator owns both sides.
