# Measured external surface

This is the currently observed DOS/BIOS/interrupt boundary for the bounded
BASIC.SS reference race. It is evidence for the narrow native runtime, not yet
a claim of exhaustive coverage across every menu, track, and game mode.

## Hardware vectors

| Vector | Observed native target | Role |
|---|---:|---|
| `08h` | `27A24h` | PC timer interrupt |
| `09h` | `26D29h` | PC keyboard interrupt |

Explicit vector tracing accounts for every otherwise unexplained transition in
the measured run. The timer handler is entered both from game code and while an
external service is active, so the Amiga replacement must preserve its
asynchronous state effects rather than compile it as an ordinary call.

## BIOS video (`INT 10h`)

Observed AH functions: `00h`, `08h`, `0Fh`, and `12h`. The captured `AH=00h`
call uses `AL=13h`, establishing VGA mode 13h as the principal display mode.
The runtime contract will need set/get mode, character/attribute readback used
during setup, and the observed alternate-select queries; it does not imply a
general VGA BIOS implementation.

## DOS (`INT 21h`)

| AH | DOS service |
|---:|---|
| `1Ah` | Set disk transfer address |
| `25h` | Set interrupt vector |
| `29h` | Parse filename |
| `2Ah` | Get date |
| `2Ch` | Get time |
| `2Fh` | Get disk transfer address |
| `30h` | Get DOS version |
| `35h` | Get interrupt vector |
| `37h` | Get/set switch character |
| `38h` | Get country information |
| `3Bh` | Change current directory |
| `3Dh` | Open file |
| `3Eh` | Close file |
| `3Fh` | Read file/device |
| `41h` | Delete file |
| `42h` | Seek file |
| `43h` | Get/set file attributes |
| `44h` | Device I/O control |
| `4Ah` | Resize memory block |
| `4Bh` | Execute program |
| `4Dh` | Get child return code |
| `4Eh` | Find first file |
| `4Fh` | Find next file |

The trace retains AX, BX, CX, DX, SI, DI, BP, SP, and FLAGS for every event, so
subfunction and argument shapes can be derived before implementing each native
service. Counts are deliberately not contractual because live race duration
and interrupt timing vary between runs.

## Multiplex and mouse

- `INT 2Fh AX=4300h` and `AX=4310h`: XMS installation and entry-point queries.
- `INT 33h AX=0000h`, `0003h`, `0008h`, and `0009h`: mouse reset/status,
  position/buttons, vertical bounds, and graphics-cursor definition.

## Direct port I/O

The aggregate tracer reduced 10,290,225 observed operations to 885 unique
`(direction, width, port, value, resume offset)` rows. The volume is dominated
by polling and inner-loop VGA programming; reproducing those operations one by
one on the Amiga would defeat the purpose of ahead-of-time translation.

| Ports | Observed role |
|---|---|
| `20h`, `21h` | PIC acknowledgement and interrupt mask |
| `40h`, `43h` | PIT channel 0 reads and control writes |
| `60h`, `61h` | Keyboard data and PC speaker/PPI control |
| `00h`-`0Fh`, `83h` | DMA channel 1 programming for digital audio |
| `201h` | Joystick polling |
| `226h`, `22Ch` | Sound Blaster DSP reset, command, and status |
| `3C0h`, `3C4h`-`3C5h`, `3CEh`-`3CFh` | VGA attribute, sequencer, and graphics-controller programming |
| `3C8h`-`3C9h` | VGA palette index and data |
| `3D4h`-`3D5h` | VGA CRTC programming |
| `3DAh` | VGA status/retrace polling |

The BASIC race alone performs millions of PIT and VGA-status reads and
hundreds of thousands of graphics-controller writes. The native design should
recognize and replace the surrounding timing, palette, and drawing routines;
it should not implement these as generic per-port calls. The sequencer and
graphics-controller traffic also proves that treating mode `13h` as only a
flat 320x200 framebuffer would be insufficient for faithful translation.

## VGA and runtime memory

The memory tracer aggregates 2,958,705 accesses into 25,193 rows. The observed
run contains 1,134,378 VGA writes from only 16 instruction sites and 811,478
VGA reads from five instruction sites. Writes touch 307 distinct 256-byte
VGA-window buckets; reads touch 193. This small set of source routines is the
natural boundary for native Amiga drawing replacements.

The program also performs 1,012,849 writes to mutable storage within its loaded
runtime allocation, covering 97 distinct 256-byte pages. A byte-level check
against every dynamically decoded instruction finds zero writes overlapping
executed instruction bytes. Thus the measured BASIC path is not
self-modifying after the Compack handoff. This does not yet prove that every
unvisited mode has the same property, but it removes self-modification from the
known hot-path design.

### Native graphics candidates

Across the bounded BASIC runs, the live map currently groups 19 VGA access
instructions into 12 functions. Names remain provisional except where stack
use and traced arguments establish the contract.

| Runtime offset | Live symbol | Evidence |
|---:|---|---|
| `00D9Fh` | `live_far_fill` | Generic far fill observed against VGA memory |
| `24499h` | `live_vga_remap_copy` | VGA read, lookup/remap, and write |
| `29E35h` | `live_vga_span_fill` | Plane-masked repeated byte spans |
| `2A97Ch` | `live_vga_transparent_blit` | Conditional planar writes |
| `2A9F2h` | `live_vga_planar_blit` | Four-plane repeated copy |
| `2AAE5h` | `live_vga_readback` | Four-plane VGA readback |
| `2AD92h` | `live_vga_clear_full` | Full-plane 64 KiB clear |
| `2ADB7h` | `live_vga_mode_setup` | CRTC/sequencer setup and clear |
| `2B40Ah` | `live_vga_plot` | Direct pixel-byte write |
| `2B45Eh` | `live_vga_plot_plane` | Plane-selected pixel write |
| `2B48Eh` | `live_vga_read_pixel` | Plane-selected pixel read |
| `2B8DEh` | `live_vga_planar_subrect_blit` | Four-plane sub-rectangle copy |

`live_far_fill(destination_far, count, value_word)` is the Borland far-memory
fill helper: four stack words because the destination pointer occupies two.
It repeats only the low byte of `value_word`, first byte-aligning an odd
destination and then using word stores. The bounded race made six calls, with
counts from zero through 65,535; every destination remains within its 16-bit
segment. One call fills VGA segment `A000h` and another fills text segment
`B800h`, so the helper cannot be discarded as host-runtime scaffolding even
though the other four calls target ordinary memory.

`live_vga_remap_copy(x0, y0, x1, y1, table_far, screen_base)` receives seven
stack words and remaps a half-open rectangle in place. It visits the four VGA
planes, reads each selected byte, replaces it through the 256-byte far lookup
table, and writes it back. Four observed calls use screen base zero and stride
100, covering rectangles from `100 x 20` through `110 x 102`; all lookup-table
and VGA spans pass the recomputed 16-bit bounds.

The plot and sub-rectangle helpers also use Borland far cdecl and leave stack
cleanup to the caller. `live_vga_plot(x, y, value, screen_base)` computes
`screen_base + y * stride + x / 4`; plane selection is deliberately outside
this helper. A bounded run made 138,093 calls with stride 100, x
`5..313`, y `49..198`, 42 byte values, and screen bases 0 and 32,700.

The adjacent helpers have equally direct contracts. `live_vga_plot_plane(x,
y, value, screen_base)` takes the same four words, selects sequencer plane
`x & 3`, and writes the byte. `live_vga_read_pixel(x, y, screen_base)` takes
three words, selects graphics-controller read plane `x & 3`, and returns the
zero-extended byte in AX. Static callers clean eight and six argument bytes,
respectively, confirming far cdecl. Dynamic tracing shows that these are major
hot-path boundaries: one bounded race made 88,708 plane-selected writes and
428,740 reads. Writes use x `58..293`, y `7..176`, and both screen bases;
reads cover every x `0..319`, y `0..189`, and both bases. Every computed plane
offset remains inside the 64 KiB VGA window.

`live_vga_mode_setup(mode, virtual_width)` has a two-word far-cdecl contract;
the BASIC fixture calls it once as `(0, 400)`. It clears the old video state,
enters BIOS mode `13h`, switches VGA into an unchained planar layout, loads a
mode-specific CRTC table, clears the new 64 KiB aperture, and derives the
logical stride and page geometry. Mode 0 with virtual width 400 establishes
the measured 100-byte stride and 32,700-byte page separation used by every
traced drawing primitive. On Amiga this routine should initialize equivalent
renderer state directly rather than reproduce VGA register programming.

`live_vga_clear_full()` takes no arguments. The fixture calls it once from
mode setup. It enables all four VGA planes, waits across a vertical-retrace
edge, and zeroes the full 64 KiB aperture. The native equivalent is a page or
buffer clear plus whatever presentation synchronization the Amiga renderer
chooses; the polling loop is not part of the portable semantic contract.

Three sprite-buffer routines also have stable far-cdecl layouts.
`live_vga_planar_blit(x, y, source_far, screen_base)` and
`live_vga_transparent_blit(x, y, source_far, screen_base)` each receive five
16-bit words because the far pointer occupies two. Both parse the source as a
byte-width, a height byte, then four consecutive planar payloads. The opaque
form copies every byte; the transparent form advances over zero source bytes
without touching the destination.

`live_vga_readback(x, y, width, height_word, destination_far, screen_base)`
receives seven words, but consumes only the low byte of `height_word`. It
writes `ceil(width / 4)` and that effective height as the destination header,
copies the selected rectangle from all four VGA planes, and appends the
starting-plane alignment `(4 - (x & 3)) & 3`. Call sites clean 14 bytes,
confirming the layout.

The expanded call trace observes 2,688 transparent blits, 6,585 opaque blits,
and 6,582 readbacks. All sprite payloads and far destination buffers stay
within their 16-bit segments, and all computed VGA spans stay within the 64 KiB
plane on this run. Coordinates include `FFFFh`, so a native replacement must
preserve unsigned 16-bit coordinate arithmetic and logical shifts rather than
prematurely treating every coordinate word as a signed host integer.

`live_vga_span_fill(x0, y0, x1, y1, value, screen_base)` is more precisely a
rectangular fill assembled from horizontal spans. It receives six words,
rejects empty half-open ranges, selects edge planes from `x0 & 3` and
`(x1 - 1) & 3`, enables all planes for middle bytes, and repeats the span for
`y1 - y0` rows. Only the low byte of `value` is written. Its existing symbol
is retained for map stability. The helper is now included in dynamic argument
tracing, but the bounded BASIC.SS race made zero calls to it while exercising
the other primitives more than 150,000 times. It is therefore not part of the
live race renderer captured by this fixture; another menu, track, or game-mode
path may use it, or it may be unused library code.

`live_vga_planar_subrect_blit(dest_x, dest_y, source_x, source_y, width,
height, source_far, screen_base)` receives nine 16-bit stack words. The source
starts with byte-width and height bytes, followed by four planes. One bounded
run copied a `100 x 97` region at `(110,77)` from an `80-byte x 200-row` source
to both screen bases. Recomputed source and 64 KiB plane bounds pass for every
captured tuple. Other runs exercised this helper much more heavily, so call
mix—not the contract—is path- and timing-dependent. These routines remain
early direct-68020 replacement candidates.

## Still unmeasured

- Semantic contracts for the remaining VGA-access functions and their
  dirty-region behavior.
- Executable-write coverage on additional tracks and modes.
- Additional paths reached by other tracks, menus, multiplayer modes, and
  failure cases.
