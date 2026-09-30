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

## Original loading panel during track loads (B3, 2026-09-30)

**What changed.** `prepare_race` no longer hands the display back to AmigaOS
before track loading. When the platform owns the display:
1. it reads the panel rectangle (x 96..223, y 90..114) back from the shown
   view into chunky;
2. it runs the verified painter `slicks_loading_presentation` (original
   `1b488..1b58f`) with the view's source palette, resident `kirj.@f`
   (DS:0680), and a caption built by `slicks_loading_caption` from the track
   stem plus DS:099c `...`, or DS:09a0 `DEMO` for demos;
3. it converts the rectangle back;
4. it loads through `begin_io`/`end_io`.

A failed preparation still ends in the released display state its callers
expect. Successful callers show the race view with `show_view`. Chunky is not
published during I/O, because the decoder borrows it.

**Checks:**
- **Pixels of the captured bitmap at the I/O window** (`amiga/diag_race_view_capture.gdb`,
  `tools/render_bitmap_capture.py`, local `tmp/capture-load.png`): the title
  tinted by both rectangles, with a centered `DEMO` caption.
- **A GO race** showed `SL_A...` for `SL_A.SS` (user screenshot). The race
  view at update 40 of that race is intact (`tmp/capture-race-setupf.png`).
- **Demo lifecycles** 1–7, 9 and 12 pass (`tmp/b3-demo*.log`). The deliberate
  load-failure cases report their single expected failure. Case 12's copper
  fault injection now also applies while view 0 is still shown.
- **SETUPF and SETUPG** late load failure, dismiss, Players and GO retry pass.
- **Fixed-clock F1 benchmark** keeps `FINAL_STATE`.
- **Full-frame dirty-sprite audits** (`diag_dirty_sprites.gdb`, NATURALO1..3Q)
  pass with 600 frames each: 32/18/5 actors and 2068/1480/1854 marks
  (`tmp/b3-audit*.log`).

**Not yet established.** This is bitmap content, not visible scanout, so the
user's one visual check remains. The other disk boundaries (intermission
preview, records, cup image, setup save) still use the OS hand-off.
