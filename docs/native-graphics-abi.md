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

`sgfx_read_pixel` reads the same location and returns a zero-extended byte in
`d0.l`. Both routines deliberately preserve the 286's unsigned logical shift
and 16-bit address wrap. They do not load or store an emulated CPU structure.

The `verify-native-graphics` gate runs the original unpacked x86 helper bytes
and these assembled 68020 routines in independent Unicorn engines. It compares
the VGA plane selected by the original port write, the wrapped address, the
returned byte, and the write side effect over deterministic edge cases and
random states.
