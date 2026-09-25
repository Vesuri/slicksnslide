# Main-menu font and GO loading

## 2026-09-25

The main-menu path used a substitute 5x7 alphabet. The original menu loop
at 29852..29928 uses DS:0688 (`iso.@f`), flags 5 and 13-pixel row spacing.
The native title now loads and decodes that original resource and uses the
existing translated string routine with a four-bank glyph-store backend.
Supplementary native title text uses `kirj.@f` to fit its smaller rows.
Both resources are loaded before takeover and released on exit.

`make verify-font-planar` passes 1,584 full-frame comparisons with the
original x86 glyph renderer using iso: palette changes, transparent pixels,
clipping and preserved registers. The native A1200 capture in the local-only
`.run/load-timing/title.png` was visually inspected. No captured pixels are
used by the program.

## Loading measurements

Muted FS-UAE, A1200/68020, 2 MiB Chip/no Fast, NATURALD BASIC setup, native GO
input. `diag_load_timing.gdb` reads DateStamp checkpoints in the OS-owned
preparation phase. One tick is 20 ms; these are preparation times, not the
subsequent starting-light countdown or every possible track/configuration.

| Stage | Before | After |
| --- | ---: | ---: |
| Initialization, disk reads, palette and clear | 0.18 s | 0.18 s |
| Scenery | 8.06 s | 0.46 s |
| Masks/resources/diagnostic scans combined | 5.04 s | 1.90 s |
| Race start, view, C2P | 0.22 s | 0.22 s |
| Total | 13.50 s | 2.76 s |

The final combined block includes mask construction 1.22 s, diagnostics
0.24 s and HUD/car resources 0.44 s. Disk reads are not the main bottleneck.

Production now paints visible scenery directly in destination scanline/bank
order, and omits provisional DAT material maps/pit routes that the following
original `/masks` pass immediately overwrote. Legacy DAT-only callers retain
their existing map behavior. The authoritative mask pass also walks output
scanlines, moving rotation, clipping and address calculation out of its
pixel loop. No maps or pictures are cached from DOS.

Verification:

- `make verify-track-visuals`: 2,560 whole-plane comparisons with the previous
  painter, including rotations, transparency, clipping and wrapped coordinates.
- `make verify-track-mask-fast`: 780 full-map comparisons across 195 supplied
  tracks, both service states and both bridge states.
- `make verify-dos-ai`: passes, including 77,824 original material compositor
  combinations, pit routing, service gates and the full AI regressions.
- Native A1200 build reaches race entry successfully with the original fonts.

Checkpoint indices: 0 entry; 1 initialization; 2 files; 3 palette; 4 clear;
5 scenery; 10 masks; 11 diagnostic scans; 12 HUD assets; 6 car/light assets;
7 race initialization; 8 view; 9 C2P. Timing diagnostics do not alter input,
invent inventory or bypass the real track/asset loaders.
