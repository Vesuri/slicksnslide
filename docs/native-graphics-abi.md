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

`sgfx_remap_copy` translates runtime `24499h..24553h`. It receives a 256-byte
translation table in `a1`, half-open rectangle coordinates in `d0-d3`, stride
in `d4`, and page base in `d5`. It replaces every logical pixel through the
four-plane store while preserving all data and address registers. The native
routine passes 256 complete four-plane comparisons against the relocated x86
implementation.

`sgfx_clear_full` replaces the VGA clear at `2AD92h`. The original enables all
four write planes, waits across a vertical-retrace edge, and clears the 64 KiB
VGA aperture. The native routine removes the hardware-only wait and clears the
four consecutive 64 KiB logical banks directly. Four differential cases seed
and compare the complete 256 KiB store while also checking the original port
sequence, retrace polling, and native register preservation.

`sgfx_mode_setup` replaces the observed mode-zero path through `2ADB7h`. It
retains the renderer-visible geometry while discarding BIOS mode entry, mouse
probing, VGA register programming, and retrace waits: physical size 320 by 200,
requested virtual width, byte stride, page size, maximum and bottom start rows,
and row padding. It composes `sgfx_clear_full` for framebuffer initialization.
Nine differentials cover virtual widths below, at, and above the physical
width, including the traced width 400 that establishes stride 100. The native
mode setup now runs in the A1200 diagnostic instead of relying on cleared
allocation memory to stand in for VGA initialization.

`sutil_fill_bytes` translates the portable semantics of the Borland far-memory
fill helper at `00D9Fh`: a 16-bit byte count and the low byte of the value are
written to a bounded destination. The 68020 implementation aligns once and
uses repeated longword stores before its byte tail. It passes 256 complete
64 KiB buffer comparisons, including zero count, odd destinations, all tail
lengths, and the observed 65,535-byte maximum. Calls targeting VGA or text
memory remain classified at their callers because those destinations have
hardware semantics beyond an ordinary flat byte fill.

The genuine race-entry trace adds a screen-transition call from `185ff`
whose `source_y + height` exceeds the declared sprite height, so the original
reads wrap within the 64 KiB source segment. The current native title crops use
bounded assets with the ordinary helper, so no wrapping entry is linked (an
oracle-tested `sgfx_planar_subrect_blit_far` is in git history).

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
planes, and the colour remapper passes 256 whole-framebuffer states.

`sgfx_span_fill` translates the live half-open rectangle filler at `29E35h`.
It receives signed `x0,y0,x1,y1` in `d0.w-d3.w`, the byte value in `d4.b`,
the guest page base in `d5.w`, the stride in `d6.w`, and the four-plane store
in `a0`. The native loop makes the VGA sequencer's boundary masks explicit by
selecting plane `x & 3` for each logical pixel while retaining the original
16-bit wrapped row address. Empty and reversed signed ranges perform no writes.
It preserves `d0-d7/a0-a6` at the temporary platform boundary and passes 256
whole-framebuffer differential cases against the original multi-plane VGA
writes.

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

`sutil_palette_nearest` translates the non-rendering palette search at
`26EAEh`, called three times by the surrounding UI routine. It scans entries
1 through 255 using Manhattan RGB distance and preserves the original initial
best index 1 / distance 300 rule. The requested component bytes are
sign-extended exactly like the original `CBW` instructions, while stored
palette bytes are unsigned. This distinction is normally invisible for 6-bit
VGA values but is covered by 512 randomized x86-versus-68020 cases.

`sui_title_step` composes the palette matcher and crop into the observed
application prefix at `19653h..19718h`. It performs all three original colour
queries, advances the byte animation counter by four, reproduces the folded
`counter / 4` ramp, stores the third colour word, and calls `sgfx_title_crop`.
Its register ABI returns the two colour indices consumed by the following UI
loop plus the stored third index. The differential executes the complete
relocated x86 prefix and native caller/callees for every possible initial
counter byte, comparing all live outputs, both memory results, register
preservation, and all four logical VGA planes.

`sui_color_slot` translates the compact state mutation helper at `1FD63h`.
It receives the render/font state in `a1`, the fallback colour word in `a2`,
the replacement byte in `d0.b`, and the signed low byte of the slot argument in
`d1.b`. Nonnegative slots return the previous byte at state offset `6 + slot`
and replace it only when the slot is below the unsigned count at offset 5.
Negative slots instead return the low byte of the fallback word and store the
replacement sign-extended to 16 bits. The native implementation accounts for
that low byte residing at `1(a2)` on the big-endian 68020 and preserves
`d1-d7/a0-a6`. All 256 possible low-byte slot values are exercised twice in
512 differential cases, including out-of-range slots and both replacement
signs.

`sui_bevel` composes `sutil_palette_nearest` and `sgfx_span_fill` into the
button renderer at `208CFh`. It preserves the original base-colour centre,
highlighted upper/side edges, shadowed lower/side edges, shrinking bevel width,
and inclusive layer count. Sixty-four whole-framebuffer differentials include
the observed `(120,82), 81 by 14, RGB 50/10/10` title-menu call.

`sui_title_menu` is the native observed selected-index-zero slice of the
six-entry loop at `19719h..19825h`. It keeps indices 0 through 6 with index 4 absent,
uses the translated bevel for the selected `GO !!!` row, and positions the six
resolved English labels at the observed 13-pixel row spacing. `sui_draw_text`
is currently a compact native 5x7 renderer for that vocabulary. It deliberately
avoids a guest CPU or C rendering layer, but it is not yet the original Slicks
font; recovering and using that font resource remains a visual-fidelity item.

`sui_title_status` covers the observed BASIC-path continuation at
`19828h..199F9h`. Trace arguments prove four active indicator slots with signs
`-,+,+,+`, at `(205,99)`, `(213,100)`, `(221,99)`, and `(229,100)`. The two
FNV-identified 2-byte-by-8-row planar sprites are copied by the already proved
transparent blitter. Direct instrumentation of the decimal renderer proves
that both overlaid counters have value `1`, at `(219,112)` with centred style 6
and `(222,114)` with left style 4. Execution edges prove all three optional
badge branches are inactive for the bounded BASIC path.

`sui_title_tail` composes the palette matcher and colour-slot helper for the
live shared wrapper suffix at `19E4Ah..19EFEh`. It preserves the original
signed 16-bit phase comparison and reset above 2000, including the rising
`phase+20`, falling `160-phase`, and steady 20 red components. Green and blue
remain 20. The resulting nearest palette index replaces colour slot zero. All
256 composed differential cases compare the original relocated x86 block with
the native counter and state mutations, including boundaries 99/100, 139/140,
2000/2001 and signed-word edge values. The traced optional-text flag is zero;
the trailing VGA start-address call receives `(0,0)` and has no Amiga-side
operation.

`sui_title_dispatch` translates the following caller's eight-entry scan-code
table at `1A2B2h..1A2C8h`. A focused 286 trace resolves the segmented alias as
`CS:IP = 1E7E:3CF2`, with the live table at normalized runtime offset `1A51Eh`.
Its keys are `01h`, `1Ch`, `1Dh`, `39h`, `3Bh`, `43h`, `44h`, and `58h`:
Escape/F10 select cancel, Enter/Ctrl/Space select the common activation case,
and F1, F9, and F12 select three distinct cases. The BASIC script idles with
zero events before delivering `1Ch`, whose original target is `1A3CCh`.
The native routine returns compact semantic case identifiers and is exhaustively
checked for all 65,536 word-valued inputs. Amiga raw Escape, Return, Space,
F1, F9, and F10 now enter this native classifier through the platform input
boundary; mouse activation maps to the original Enter case.

`sgame_post_title_init` is the first native block after the title routine has
returned zero to its outer caller. It translates runtime offsets
`16272h..162E4h` as one nested-loop region rather than as individual helpers.
For four players it copies a shared seed word, clears all 13 state words, and,
only when the mode word is zero, replaces a cleared word with 4 when the
corresponding shared flag byte has bit zero set. Its register ABI receives the
52-word grid, 13 flag bytes, four seed words, mode, and seed directly. The
complete outputs agree with the relocated x86 block in 256 randomized cases,
including both mode branches and all flag combinations encountered by the
corpus; word memory is normalized for the expected x86/68020 endian difference.

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
