# DOS actor layer contract

Technical evidence and historical verification record. The actionable current
queue is maintained separately in [open-work.md](open-work.md).

Recovered from `disasm/live-listing.txt`. Initial layer, transition branches,
current contact latch, emission-time point thresholds, and point masking are
now integrated. Car-sprite masking and cross-layer collision exclusion are
also implemented. Live DOS-input replay now verifies ordinary layer transitions;
whole-race equivalence and the gaps below remain open.

## Initialization

`1000:bc39..bc47` reads the byte following the start heading from the track
stream. `1000:bd1a..bd20` copies that byte to DS:5388 for every driver.
The native decoder already preserves it as `navigation.start_style`, but
the race now uses that field to initialize `actor_layer`. It is a layer state,
not a car colour.

## Per-driver transition

At `2000:2981..29ce`, DOS samples both map modes at
`(DS:53b6[car] + 3, DS:53be[car] + 3)`. The producer at
`2000:21b9..21f8` divides signed car X/Y by 100 and subtracts three.
Thus the sample is exactly the integer car centre (division toward zero),
not the variable sprite bounding-box centre.

The following order matters (`2000:29d2..2a61`):

1. If layer is zero, mode-zero material is zero, car-state word +29h is
   zero, and DS:536c contact is zero, set layer to one.
2. Select the current surface from mode zero when layer is zero; otherwise
   select mode one. Store that selection in DS:537c.
3. If the selected surface is 19, clear the layer. Do not resample the surface
   after clearing it in this update.
4. If car-state +29h is nonzero, set the effective driving surface to zero
   and clear the layer; otherwise retain the selected driving surface.

DS:3058 is car-record base DS:302f plus 29h, i.e. the already recovered
`special_drive_state`, not a separate unknown height field. Layer tests use
nonzero, whereas the special velocity branch uses positive: preserve that
distinction for negative values.

DS:536c is a current-update contact latch. It is set by the track-contact
path (`2000:134f..1352`) and by car collisions (`30a0..30ab`). At the end of
the driver update it is copied to DS:5370 and cleared (`3d7b..3d86`). It is
not interchangeable with a persistent previous-contact flag.

## Ordering dependencies

Wheel emission and its actor layer assignment occur before the transition
block; car-to-car collision handling follows it at `2000:2d27` onward.
The native update now emits wheel effects, updates the layer, then handles
car collisions. `actor_contact` is separate from persistent contact flags and
is set for both cars at contact, then cleared at the current driver's end.
The larger native per-car update structure still needs live DOS comparison;
this ordering correction alone does not prove whole-update equivalence.

Point actor `+3b` is emission-time layer * 15, not the driver's later layer.
At draw time, zero bypasses occlusion; otherwise the raw map byte must be
<= the threshold. The raw map is `(material << 3) | (surface & 7)`.
The native BASIC map reconstruction is verified across all 60,800 pixels by
`tools/verify_actor_mask.c`. Expiring permanent marks must use the same mask
as active points, not paint unconditionally behind a foreground object.

## Evidence still required

- Extend live transition replay to contact-suppressed entry and negative
  special states, beyond the ordinary and positive-special coverage below.
- Compare live rendered bridge passages and collision outcomes with DOS.

Point masking and retirement now share visibility through the saved-under
validity flag; hidden points do not commit a permanent mark. Tests cover 12
layer branch fixtures and all 256 raw mask values at thresholds zero and 15,
including permanent retirement. Both native car drawing paths now use the
same comparison, confirmed by `3000:3694` (zero bypass) and `4b21..4b24`
(raw byte greater than threshold is skipped). Car threshold assignment is
at `2000:3e18..3e21`. Tests also cover car visibility and saved-under
restoration across all 256 raw bytes, for both zero and 15 thresholds.
The collision loop now excludes unequal layers, matching `2000:2d53..2d60`;
the unequal-weight collision fixture checks that different-layer cars retain
their velocities and do not set contact or increment the collision counter.
The first silent A1200 run with car/point masking reached frame 700 without
an error, with all cars on lap 2 and the same positions as the preceding
build. This establishes a BASIC regression checkpoint, not DOS transition
equivalence. Its target framebuffer was inspected from `.run/actor-layers`.

Wheel sampling now also selects the map using the emission-time layer,
matching `2000:2348..2354` and the scattered-point check at `1000:e888`.
Any nonzero special-drive state suppresses wheel emissions before consuming
random numbers (`2000:233b`). Focused host tests and the Amiga build pass;
this follow-up was built after that live run. A subsequent strict A1200
one-lap run including it reached results at frame 618: all four finished,
positions 2/3/4/1, one winner sound event, results drawn, music started,
and no race error. Checksums were logical `2f26e8ea`, display `5ee09abf`.
The native results framebuffer was inspected in `.run/layer-results`.
The results gate now checks those persistent-mark image values while keeping
the exact frame, order, all-finished flags, music-start and winner-event
assertions. It passes with the contact-latch follow-up included:
`SLICKS_RESULTS_OK FRAME=618 POS=2,3,4,1 CHECKSUM=2f26e8ea DISPLAY=5ee09abf`.
The separate strict A1200 exit gate also passes `SLICKS_RESTORE_OK STATUS=1f`.
All 195 supplied tracks decode, and the focused collision, physics, surface
effect and dirty-list tests pass. This is native regression evidence, not
a claim that every bridge-layer transition matches a live DOS trace.

That follow-up moves setting `actor_contact` inside the new-impulse branch:
`2000:2f50` skips directly to `3183` for an already-latched overlap, bypassing
the `30a3/30ab` flag stores. A regression checks that repeated overlap does
not reassert the transient flag after it has been cleared. Focused tests
pass; the later results-gate run includes this correction.
Existing host tests and actual 68020 retirement/coordinate/display tests pass.
The Amiga build and results/restore gates above pass. Earlier rendered
checkpoint values must not be assumed unchanged after correcting
collision/emission order.

The existing race-state CSV captures car-state +29h, but not the layer or
the two contact-latch values, so it cannot establish transition equivalence.

## Live DOS transition replay

The optional `SLICKS_TRACE_LAYERS` hook records pre/post states at runtime
offsets `128d2` and `12961` (listing `2000:29d2` and `2000:2a61`). It records
layer, both sampled maps, current/previous contact, special state, selected
and effective surfaces, and coordinates without changing guest state.
Apply `tools/patches/dosbox-x-slicks-actor-layers.patch` after the existing
instrumentation and rebuild the local DOSBox-X.

```sh
make reference-layer-trace REFERENCE_FIXED_ROOT=tmp/pc-layers
make build/verify_actor_layers
build/verify_actor_layers tmp/pc-layers/layers-paced.log
```

The capture is silent and does not record video. Its one-second menu pacing
avoids the missed key releases seen with the older 0.2-second trace recipe.
The verifier rejects missing/malformed/unpaired records, checks that inputs
stay stable within each pair, and calls the production `update_actor_layer`
with the DOS coordinates, samples, special state and current contact latch.
It does not claim that native physics produced those same inputs, or that
the selected/effective driving surfaces are already integrated exactly.

The completed ordinary BASIC capture matched **10,758 transitions**, including
25 entries and 21 exits. There were 19 contact-bearing inputs, but **zero**
contact-suppressed entry cases, zero special-state cases, and zero skipped
out-of-bounds coordinates. This proves the observed layer decisions, not
full collision timing, actor draw ordering, or whole-race equivalence.

A second completed capture used the existing `SLICKS_TRACE_SPECIAL=1`
integrator test injection, with a separate `tmp/pc-layers-special` directory.
It matched **13,432 transitions**, including 32 entries, 28 exits and **41
positive special-state cases**. Five inputs had contact, none suppressed
entry, and none were out of bounds. This is deliberately injected guest-state
coverage, not evidence that an unmodified race naturally reached that state.
Negative special states remain covered only by the focused branch fixtures.

```sh
SLICKS_TRACE_SPECIAL=1 make reference-layer-trace \
  REFERENCE_FIXED_ROOT=tmp/pc-layers-special
build/verify_actor_layers tmp/pc-layers-special/layers-paced.log
```

## Car drawing order

`2000:3e7d..3eba` assigns car actor priority 3 for nonzero layer and 4 for
zero layer. The actor renderer traverses priorities in ascending order and
actor slots in ascending order (`3000:3c03..3c31`). The native car draw now
groups nonzero-layer cars before layer-zero cars, retaining player order
within each group, instead of allowing player number to override the layer.
Saved-under restoration reverses both groups and their internal ordering,
before simulation changes any layer. Starting-grid initialization has a
uniform layer, so its original player-order draw has the same ordering.

Focused overlap tests cover all 16 layer combinations in both authoritative
chunky and logical-buffer paths (32 cases), asserting top colour and complete
background restoration. The rebuilt silent A1200 results gate still passes
at frame 618, positions 2/3/4/1, logical `2f26e8ea`, display `5ee09abf`.
Priority-3 point actors now draw **after** nonzero-layer cars and **before**
layer-zero cars. Reverse restoration uses the opposite order. The DOS
allocator scans free slots from 1 (`3000:30c4..30e7`), while the captured
ordinary BASIC race-data car records at DS:3035 + car*36h contain actor
slots 1/2/3/4. Those persistent occupied slots precede the transient actors
in the renderer's ascending-slot traversal. This supports the tie ordering
for active race cars, not arbitrary future actors or freed car slots.

An additional 16 overlapping priority-3 point/car fixtures check all layer
combinations and exact background restoration in the authoritative chunky
path. Particle-to-particle equal-priority ordering after slot reuse remains
open: the native compact pool preserves insertion order, whereas DOS scans
actor slots. Do not claim the complete actor scheduler is exact.
The silent A1200 results gate also passes after the point/car tie correction:
frame 618, positions 2/3/4/1, logical `2f26e8ea`, display `5ee09abf` unchanged.

## Particle slot reuse: expiry prerequisite

`make verify-dos-particle-expiry` executes the original bytes at listing
`3000:3918..394f` in Unicorn x86-16, locating the block by a unique signature
in the local unpacked runtime. It does not emulate the guest in the Amiga
build. All 30 combinations of actor state 1..5, page 0/1, and remaining
lifetime 0/1/2 pass:

- Lifetime zero bypasses decrement entirely (unlimited), rather than expiring.
- Lifetime one on page zero is reset to one, keeping the actor active.
- Lifetime one on page one reaches zero and negates the actor state.
- Larger positive lifetimes decrement without changing state.

Negating the state does **not** free the slot. The later retirement traversal
at `3000:3fa7..4021` decrements negative states, redraws -6/-7, and invokes the
free routine at -3/-7. The retirement oracle now executes the original code
for **all 256 state bytes**, including the real `35a5` free routine for point
actors (zero saved-sprite allocation size). It verifies final state, redraw
and release call counts, and balanced stack. Only `3673` redraw is stubbed
with RETF: this is not pixel-rendering verification. State -5 becomes -6
with redraw on the first pass, then -7 with redraw/release on the next.
State -2 releases on its first pass. The -4-to--3 rewrite and signed-byte
wrap are also covered.

The same verifier now executes the original allocator decision block
`3000:30c4..3114`, stopping at actor setup or the failure exit `32a6`.
All **486 combinations** pass: four occupied car slots followed by five
independently free/active/retiring slots, at full capacity or with room to
extend the high-water mark. It chooses the lowest zero-state slot, retains
every negative-state slot, reuses holes even at full capacity, and grows only
when no free slot exists. Actor bytes remain untouched by this selection
block. This test excludes resource allocation and subsequent actor setup.

The native compact pool currently removes entries
on its one-byte lifetime expiry, so assigning first-free slot numbers without
recovering retirement/page cadence would give falsely exact ordering.

The race caller invokes `38e7(0)` after the four-car draw-setup loop at
`2000:3f51..3f58`. Within that routine, `3000:3e0a..3e10` flips the page
selector once, between the forward draw and restore/retirement traversal.
This is one alternating page per actor update, not two complete lifetime
decrements per simulation update. Live call cadence remains to be checked.

Next work: verify live page cadence, then preserve actor slot
identity and order without adding a VGA emulator or a second framebuffer.

The optional page trace is provided by
`tools/patches/dosbox-x-slicks-actor-pages.patch`, after the layer-trace patch.
It reads the page selector at actor-routine entry/exit, runtime offsets
`237e7`/`23f30`. Reproduce the silent capture with:

```sh
SLICKS_TRACE_PAGES=1 make reference-layer-trace \
  REFERENCE_FIXED_ROOT=tmp/pc-actor-pages
python3 tools/verify_actor_pages.py tmp/pc-actor-pages/layers-paced.log
```

The checker rejects malformed/unpaired calls, page changes between calls,
missing or duplicate simulation frames, and calls that fail to flip the
page. It excludes frame zero from the one-call assertion (setup) and frames
beyond 4000, before the existing integrator trace's sample cap saturates its
frame counter. Capture must finish before using the final checker result;
a still-running log may end in a deliberately rejected incomplete pair.

The first completed live capture **fails the one-call-per-simulation-step
assumption**: page toggles remain continuously paired, but actor calls skip
simulation counters regularly (first gaps 6, 11, 15, 19, 24). There are
2,710 complete actor calls including setup. This is important negative
evidence, not a passing cadence gate. The same capture still passes 10,837
native layer-decision replays. The strict page checker intentionally exits
nonzero on the skipped simulation steps. Recover the caller's update/render
scheduling before tying particle lifetime or slot retirement to native
simulation frame count; the existing integrator trace counter and the
actor-update count are not interchangeable.

The caller explains the skipped counters. `1000:fe5e..fe98` waits for a
timer change, stores the elapsed count in BP-2 (first update uses one), and
caps it at 45. Each car's integrator loops BP-3a from zero to BP-2 at
`2000:0dd1..14dc`. The car-three integrator-post trace increments its counter
inside this inner loop. Surface/layer handling and `38e7(0)` follow once per
outer update, not once per inner step. Consequently the prior "frame" name
means physics substeps in this trace, not rendered frames, and its gaps
should not be implemented as an arbitrary fixed render/physics ratio.

The page hook now records the caller's BP-2 at actor entry. With those new
records, `verify_actor_pages.py` requires each counter delta to equal the
recorded elapsed batch (1..45), while retaining paired page-toggle checks.
It still rejects gaps in older traces lacking the elapsed-count evidence.

## Contact across native physics substeps

With two integrations per native update, an early track hit must survive a
later clear substep. DOS writes DS:536c=1 at `2000:1352`, does not clear it
on the no-hit branch, and clears it only at the outer per-driver tail
`3d86`, after the layer decision. Native `record_track_contact` now sets
`actor_contact` on every hit and preserves it on clear substeps. The separate
`touching_solid` flag continues to represent the last substep and suppress
consecutive impact events. All four two-substep hit/clear combinations
verify latch retention, layer-entry suppression, event deduplication and
entry after the next batch clears contact. Focused host tests and the
Amiga build pass; this does not prove the whole track-contact walker exact.
The silent A1200 follow-up in `.run/two-tick-contact` reproduces the new
two-tick checkpoint exactly: complete results at frame 445, all finished,
positions 4/3/2/1, logical `dee9b678`, display `4a907ebf`. The unchanged
frame-618 regression baseline still fails, as recorded above; this run is
not reported as a passing strict gate or promoted to the normal launch build.

## Tick rate at the captured speed setting

Do not equate the DOS timer quantum with a centisecond. The race calls
`7bc2` with `DS:05de * 5` at `1000:fc55..fc5c`. The captured game-data
value is 100, so the argument is 500. `3000:7bc8..7bf8` calculates
`65536 * 100 / 500 = 13107` and programs that PIT reload value.
`tmp/pc-layers/slicks-ports.csv` independently records low/high bytes
`33/33` at runtime offsets `27b54`/`27b58`, with control `34` at `27b4f`.
DOSBox-X's IBM timer base is 1193182 in `include/timer.h`, giving about
91.03 timer ticks per second, not 100. This also explains why the captured
physics count outpaces the VGA actor-update count without a fixed integer
batch size.

The current fixed native two-tick/PAL-update integration is therefore an
intermediate correction, not exact wall-clock timing. It needs a rational
tick accumulator tied to the recovered game-speed setting, and the HUD's
time conversion must be checked separately. Keep the old normal-launch
binary until the corrected scheduling is validated; do not silently adopt
100 ticks/second merely because the HUD uses centisecond-named fields.

The completed slower-input capture `tmp/pc-ticks-slow-input/layers-paced.log`
now validates the batching explanation: **1,715 completed gameplay batches**
match the captured BP-2 count exactly (1,202 batches of one tick, 513 of two).
All 1,716 completed calls including setup toggle the page continuously.
Timed shutdown cuts off one last PRE at physics counter 2,229; the strict
checker rejects it. Its explicit `--allow-final-incomplete` option reports
and excludes that final call, without claiming its toggle was verified.
All 6,864 captured layer pairs also pass the native transition replay.

```sh
SLICKS_TRACE_PAGES=1 make reference-layer-trace \
  REFERENCE_FIXED_ROOT=tmp/pc-ticks-slow-input REFERENCE_KEY_PACE=2
python3 tools/verify_actor_pages.py --allow-final-incomplete \
  tmp/pc-ticks-slow-input/layers-paced.log
```

The native fixed-two-tick assumption has now been replaced by an integer
remainder accumulator: add 1,193,182 each nominal PAL update and consume
655,350 per physics tick (`13107 * 50`). The same one/two-tick batch is passed
to all cars and used for countdown progression. No floating point, wide
division, shadow framebuffer or emulated CPU was added. Initialization clears
the phase with the rest of the race state. A million-update host test checks
both the cumulative quotient and exact remainder against 64-bit arithmetic
at every update; existing force, contact, surface and collision tests pass.

Scope remains the captured 100% speed setting at nominal 50 Hz updates.
Other speed settings, actual PAL refresh calibration, catch-up after missed
vblanks, and exact HUD time conversion remain unverified. Actor lifetime
still advances once per native update; persistent DOS slot identities and
retirement/page parity are not yet integrated into the native pool.
