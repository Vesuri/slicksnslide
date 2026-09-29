# Main-title dirty publication

Verified 2026-09-29. This is implementation evidence, not an open task list.

The main title previously deinterleaved and converted all 64,000 visible
pixels on every selection/count edit, directly into the displayed bitmap.
The whole-screen Kalms converter wrote planes 0–3 before returning for 4–7.

Title painters now report rectangle bounds. The original label/background
crop and status repaint merge into an aligned rectangle (96,77)..(256,174):
15,520 pixels, versus 64,000 previously. Registered-owner text has a separate
(0,190)..(320,200) rectangle, including its periodic colour update. Rectangle
overlaps merge; full background reconstruction explicitly invalidates the
whole title. No shadow image, per-row bitmap, or framebuffer-difference scan
is used for production dirty tracking.

Only those rectangles are unpacked from the four VGA banks into chunky
memory. After preparation, publication waits for a fresh display-end edge
and uses the existing C2P16 routine, which stores all eight final plane words
for each 16-pixel block. The duplicate initial whole-screen conversion is
removed. This reduces the exposure to mixed-plane tearing; it is not a
double-buffering guarantee or a measured worst-case blanking-time bound.
Other modal menus' publication implementation is unchanged by this fix.

Verification:

- `make verify-title-dirty`: bounds merging, alignment, full invalidation,
  overflow, clipping and 64,000 unpack/untouched pixel checks pass.
- `make verify-c2p16`: 128 single-bit cases, 1,536 randomized cases, 210
  exhaustive horizontal spans and 32 empty rectangles pass, including final
  stores, source bounds, canaries and ABI preservation.
- Native build succeeds. Muted 2 MiB/no-Fast A1200 test:
  `SLICKS_REGISTRATION_TEST=4 SLICKS_DEBUG_WARP=1
  FSUAE_RUN=.run/title-dirty ./debug.sh '' diag_title_dirty.gdb`.
  `REGCHECKD` feeds six real navigation/count-edit inputs then Escape; it
  does not inject menu state. The opt-in audit compares every displayed pixel
  and chunky pixel with the authoritative logical screen after publication.
  Result: full=1, partial=6, checks=7, errors=0, last_pixels=15520,
  restoration=31. The runner closes its emulator. The fixture is keyless;
  registered-owner pulse coverage is not claimed as a fresh native pass.
