# Rendering

The game draws into byte-per-pixel memory exactly as the DOS original
composes its VGA screen, then publishes only the changed parts to an AGA
bitmap. There is no double buffering, no shadow image and no
framebuffer-difference scan: every displayed change comes from a rectangle
or pixel that a painter reported. The register-level contract of the
translated VGA helpers is in [native-graphics-abi.md](native-graphics-abi.md);
it is not repeated here.

## Display

- **Mode:** 320x200, eight bitplanes, 256 colours, PAL A1200 AGA. The
  window is centred on hardware lines `$38..$ff` (DIWHIGH carries the ninth
  vertical stop bit), built by the supplied framework's `setPlayfield`.
- **Bitmap:** interleaved, in Chip RAM. Each 320-byte row holds the eight
  40-byte plane rows, so `BytesPerRow` is 320 and the chunky source and the
  bitmap share the same row offset (`mult320`).
- **Views:** two, each with its own bitmap and copper list
  (`SLICKS_AMIGA_VIEW_COUNT`). View 0 shows the title, menus, dialogs and
  loading panel; view 1 shows the race. A modal menu over a race uses view 0
  and leaves the race bitmap untouched.
- **Palette:** the copper list loads the 18-bit VGA palette as 24-bit AGA
  colours (each 6-bit component expanded by `v<<2 | v>>4`). The positions of
  every colour MOVE are recorded, so palette updates such as the track
  boundary animation (colours 199..203) patch copper words in place.

The platform owns the display after takeover. Disk I/O hands it back to
AmigaOS, except during track loading, where the original loading
panel is painted over the shown view (read back from the bitmap, painted by
the translated original painter, converted back) and stays visible while
loading.

## Surfaces

- **Logical VGA store:** four consecutive 64 KiB planes (256 KiB), with the
  original 100-byte row stride and the pixel at `(x&3)*64K + y*100 + x/4`.
  The title, the track scenery painter and every helper in
  `native-graphics-abi.md` use this layout, which keeps the original sprite
  formats and 16-bit address wrap byte-exact.
- **Chunky surface:** 320x200 bytes at 320-byte stride, plus 32 bytes of
  lookahead padding for the Kalms converter. Menus and dialogs draw here
  directly through `struct SlicksChunkyUi`.

At race start the finished scenery is deinterleaved once from the logical
store into the chunky surface. From then on the chunky surface is
authoritative (`chunky_authoritative`): the race step passes a null logical
pointer and all actor, HUD and particle drawing reads and writes chunky
pixels only.

## Conversion routines

`src/platform/amiga/vga_to_chunky.s` provides the three publication paths:

| Entry | Use | Converter |
|---|---|---|
| `slicks_chunky_rows_to_amiga` | full-width rows, whole screens | Kalms `c2p1x1_8_c5_bm` |
| `slicks_chunky_rect_to_amiga` | 16-pixel-aligned rectangles | `c2p16_interleaved` |
| `slicks_chunky_pixels_to_amiga` | sparse pixel lists | per-pixel `BFINS` into all eight planes |

`c2p16_interleaved` is a 16-pixel Kalms butterfly that stores only the final
eight plane words of each block, never transpose intermediates.
`c2p8_interleaved` and `c2p8_16_interleaved` are isolated experiments, not
production paths. `make verify-c2p16` and `tools/verify_planar_writes.sh`
check the converters on a 68020 emulator against independent bit-level
references, including every destination store.

## Race publication

Each race update runs in a fixed order:

1. `slicks_race_step` simulates and redraws actors into the chunky surface,
   reporting changes as dirty rectangles and dirty pixels.
2. `slicks_amiga_platform_wait_publication` waits for the next display-end
   edge (line `$100`), or returns at once if preparation has overrun that
   edge. Missed slots are not queued; the race clock is real time.
3. A pending boundary-palette change is patched into the copper list, and
   queued audio requests are dispatched.
4. Sparse pixels already inside a dirty rectangle are pruned
   (`dirty_prune.s`). The remaining pixels are converted first, then the
   rectangles; both read the same chunky surface, so a rectangle conversion
   cannot lose a sparse change.

Starting at line `$100` gives the conversion the lower border and the next
frame's upper border before the beam reaches the playfield. This reduces
tearing exposure; it is not a guaranteed blanking-time bound.

**Dirty rectangles** (`slicks_mark_dirty_rect`, `dirty_rect.s`, with the C
form in `race_runtime.c`) are clipped, widened to 16-pixel columns and
merged while they overlap (edge contact does not merge). At most
`SLICKS_DIRTY_ROW_MAX` (16) are kept; overflow unions everything into one
rectangle, never dropping a change.

**Dirty pixels** carry single-pixel changes from moving particles,
particle restoration and expiry, and HUD glyphs. The list holds 512
entries; overflow pixels go to the rectangle list instead.

## Actor layers and draw order

Each car has an actor layer, initialized from the track's start-style
byte. Every update samples both surface maps at the integer car centre and
follows the original transition order: enter layer 1 from a clear
layer-0 surface when no special-drive state or current contact exists;
select the surface from the map for the current layer; clear the layer on
surface 19; then clear it and zero the driving surface while the
special-drive state is nonzero. The contact latch is set by any track hit
in the update's physics substeps and by car collisions, and cleared at the
end of the driver's update. Wheel emission and its layer assignment come
before the transition; car-to-car collisions come after it, and cars on
different layers do not collide.

Occlusion uses the raw map byte `(material << 3) | (surface & 7)`. An actor
with threshold zero is never occluded; otherwise a pixel is drawn only where
the raw byte is not above the threshold. Points take threshold
`layer * 15` from their emission-time layer; cars take theirs from the
current layer. Permanent marks use the same mask, so hidden points never
commit a mark.

All actors share one 200-handle pool with the original allocator: lowest
zero-state handle first, negative retirement states stay reserved, handle
zero is never used. `actor_order.s` builds a chain per priority in
ascending handle order and its exact inverse. Drawing proceeds:

1. priority-0 points, priority-1 actors, car shadows, priority-2 actors;
2. cars with a nonzero layer (priority 3), in player order;
3. priority-3 points and actors (car handles 1..4 precede them);
4. layer-zero cars (priority 4), then priority-4 actors;
5. priority-5 points, then priorities 6 and above.

Restoration of saved backgrounds runs the exact reverse, before simulation
changes any layer, so the saved-under stack always unwinds correctly. The
HUD occupies rows 184..199; point actors are confined above row 184.

## Retention of unchanged track sprites

The original restores every actor and redraws all of them each update.
For an unchanged animated track sprite (kind 3, frames 0..3) that no actor
drawn before it touches, restore-then-redraw leaves identical pixels and an
identical saved background, so both steps are skipped
(`sprite_retention.inc`, `sprite_retention.s`, `retention_touch.s`).

- Candidate geometry lives in an 8x8-pixel cell grid and a point-row map,
  derived from actor fields, never from pixels. It is rebuilt only after a
  producer marks the geometry dirty and the sprites have settled.
- Every drawn point, car rectangle and moved sprite touching a candidate
  revokes it. Overlapping candidates form groups that are kept or restored
  together, in reverse draw order.
- A kept sprite that changed, or that a lower actor will touch, is
  late-restored after simulation and redrawn normally.
- Retention is off with fewer than eight candidates, during the countdown,
  in Arcade mode, and when shadows or weapon sprites are active. Race start
  and any pause or menu handoff invalidate it completely.

`make -C amiga RETCHECK=1` builds a check that compares every racing update
with a non-retention reference, and scans all geometry keys on cache hits to
catch missing invalidations.

## Stationary track-object cache

Kind-1 and kind-3 track objects keep their car-contact checks every update.
When the previous update fully processed an object while stationary and its
velocity is still zero, the loop skips repeating its material/layer
sampling and actor configuration. The validity bytes
(`track_stationary_ready`) sit beside the permanent handle array.

The shortcut is safe because of the write set:

- Only `track_actor_motion.inc` / `track_motion.s` write these objects'
  navigation position, layer and velocity during a race. Kinds and handle
  associations are fixed at setup.
- Configuration sets state 1, unlimited lifetime and zero motion and
  animation inputs, so actor advancement cannot change the configured
  fields or release the permanent slot.
- Weapons and effects use their own handles; the allocator never reuses a
  permanently positive track handle. Car contact changes navigation
  velocity, which invalidates the cache on the next update.
- Restoration, drawing, visibility caching and retention write only render
  metadata. Material and surface maps are built before the race and are
  read-only afterwards.
- Actor setup clears every validity byte; a new race resets the pool.

Zero velocity alone is not sufficient: a moving object samples the
prospective destination before its ray can be blocked, and damping can then
stop it. Every moving pass therefore clears validity, even when blocked or
damped to zero, and only a following fully processed stationary pass sets
it.

## Other race caches

These are derived from immutable track assets and maps, never from screen
pixels:

- `car_render_cache` prepares rotated, recoloured car frames and a 24x40
  tile maximum of the occlusion map, which can prove a whole car
  unobscured so `car_render.s` can draw it on a fast path. Any failed
  eligibility test falls back to the general renderer before a write.
- `track_sprite_visibility` caches per-pixel write masks for repeated
  stationary track-sprite draws; collisions only force recomputation.
- `status_bar_cache` reuses HUD bar commands until other drawing touches
  the status strip.

## Title and menus

The main title is painted into the logical store. Painters report
rectangles to `struct SlicksTitleDirty` (`src/ui/title_dirty.h`): at most
four, 16-pixel aligned, merged on overlap; a fifth disjoint rectangle, or a
full background rebuild, invalidates the whole screen. A selection or count
edit dirties the aligned label/status area (96,77)..(256,174), 15,520 pixels
instead of 64,000; the registered-owner line has its own (0,190)..(320,200)
rectangle. `publish_title_dirty` unpacks only those rectangles into chunky
memory, waits for a fresh display-end edge, and converts them with
`c2p16_interleaved`.

Menus, dialogs and the pause menu draw into the chunky surface and report
up to 16 rectangles through `src/ui/menu_dirty.h`, with the same alignment
and merge rules; overflow keeps the combined bounds.
`present_menu_surface` waits for display blank and converts each rectangle.
Help and standings text report bounds per glyph advance
(`help_text_dirty.h`), not measured string width.

The menus use the original `iso.@f` font; the race HUD and smaller native
title text use `kirj.@f`. HUD coordinates, alignment and colours come from
the translated original painter, including the lap digits that overlap the
upper panel borders as in the original.

## Verification

- `make verify-dirty-tracking`: dirty rectangle and pixel producers,
  merging, overflow, and chunky/VGA agreement.
- `make verify-title-dirty`, `make verify-menu-dirty`: title and menu
  rectangle merging, alignment, clipping and overflow.
- `make verify-c2p16`, `make verify-planar-writes`: converter output and
  every destination store.
- `make verify-track-actors`, `verify-actor-slots`, `verify-actor-advance`,
  `verify-weapon-actors`: track-object behaviour against original DOS
  updates and the shared actor pool.
- `make verify-dos-hud`: HUD composition against the original painter.
- Full-frame stale-pixel audits (`amiga/diag_dirty_sprites.gdb`) compare the
  displayed bitmap with the authoritative chunky surface after publication,
  on F1, CITY and WHACKO.
