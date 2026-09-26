# Gameplay performance evidence

## 2026-09-26 baseline

Stock PAL A1200, 68020, 2 MiB chip RAM, no Fast RAM; muted debug
output with game audio processing enabled. `SLICKS_BENCHMARK=1`,
`amiga/diag_benchmark.gdb`, source baseline `749bb11` (the later documentation
commit does not change the executable). No debugger stops during measurement.
Local log: `tmp/perf-baseline-749bb11.log`.

603 measured updates, all above the 312-raster-line/20-ms budget.
Maximum work: 1429 lines (91.6 ms), at frame 574. Maximum wall time:
1568 lines. Mean update cadence: 674241 / 602 = 1120 lines (71.8 ms).
These are emulated target measurements, not host execution timings.

Final measured frame, raster lines:

| Component | Lines |
| --- | ---: |
| Simulation and chunky rendering | 1070 |
| Audio | 11 |
| C2P | 70 |
| Diagnostics | 6 |
| Actor restore (within simulation/rendering) | 268 |
| Particle advance | 49 |
| Update | 128 |
| HUD | 39 |
| Actor draw | 582 |

The phase breakdown is the final frame, **not** the maximum-work frame.
Work excludes synchronization waits; actual update cadence must also meet
the target. This legacy benchmark is a reproducible optimization baseline,
not proof of general gameplay across tracks, vehicles and particle loads.
The fixture rejects race errors, missing measurements and audio VBI spills.

## Stable actor-priority index

`tmp/perf-order.log`: same benchmark, 603 updates. Maximum work 1033 lines
(66.2 ms), maximum wall 1117 lines, cadence 495776 / 602 = 823.5 lines
(52.8 ms). All updates still exceed budget. Final frame: step 686, audio 10,
C2P 69, diagnostics 7; restore 162, advance 44, update 130, HUD 37, draw 308.
The index preserves ascending handle order for drawing and descending order
for restoration, rebuilding after actor changes. It adds 329 bytes to runtime
state and does not replace producer-side dirty tracking.

`verify-track-actors`, `verify-weapon-actors`, `verify-dirty-tracking` pass,
including original full-screen comparisons, saturated mixed-priority pools,
edge clipping, exact restoration and permanent-mark survival. Target build
passes. The owned muted emulator exited after measurement.

## Actor row clipping and pointer walks

`tmp/perf-rows.log`: maximum work 1024 lines (65.6 ms), maximum wall 1114,
cadence 495464 / 602 = 823.0 lines (52.8 ms); 603/603 over budget.
Final step 682, audio 10, C2P 70, diagnostics 6; restore 162, advance 44,
update 130, HUD 37, draw 304. The improvement is small in this race: track
and weapon sprites are not the main remaining actor cost. Clipping and
source row multiplication now happen once per row instead of per pixel;
the same original renderer/restoration suites pass. No target errors or
audio spills, and the owned emulator exited.

## Rejected fused car-rendering experiment

`tmp/perf-cars.log`: maximum work 1027 lines, cadence 499520 / 602;
no improvement over the row baseline. The implementation was removed.
Retained supplemental regression coverage compares all 256 combinations of
rotation/body ramp/mask against the existing scalar VGA renderer and checks
saved backgrounds and restoration. This is optimization regression coverage,
not an independent original-executable oracle.

## Actor substage profile

`tmp/perf-detail.log`: final-frame restore index/high/cars+priority3/low
19/34/29/76 raster lines; draw index/low/cars+priority3/high 22/91/61/134.
High includes priorities 4..127 (restore also includes priority-mask work);
low includes priorities 0..2 and shadows. These grouped measurements must
not be attributed solely to cars or solely to point particles. Instrumentation
runs only on the diagnostic target frame and contributes to its measured time.
Maximum work 1028 lines, mean cadence 493592/602 lines, all603 over budget.

## Indexed restore specialization

`tmp/perf-indexed.log`: maximum work 915 lines (58.7 ms), maximum wall 1116,
cadence 419076/602 lines (44.6 ms), all603 over budget. Final step 590,
audio10, C2P69, diagnostics6; restore91, advance46, update135, HUD34, draw279.
Restore substages21/26/27/15; draw21/70/58/128. Indexed restoration caches
its array bases and omits priority tests already resolved by the index;
indexed drawing also avoids those redundant tests. Original renderer,
restoration and dirty-tracking suites pass; no target errors/audio spills.

## Compact 68020 point renderer

Separating the C point helper alone did not improve timing (`perf-point.log`:
927 maximum work lines, cadence422823/602). With the native point routine,
`tmp/perf-asm-point.log` reports maximum work903 lines (57.9ms), maximum
wall1116, cadence413442/602 (44.0ms), still603/603 over budget. Final step570,
audio12, C2P69, diagnostics7; restore93, advance43, update136, HUD34, draw260.

`make verify-particle-draw` compares 2048 native/scalar full-frame, metadata,
dirty-list and overflow cases and verifies the C register/stack ABI. This
is an optimization regression against the scalar path; independent original
x86 layering/pixel comparisons remain in the track/weapon actor suites, which
also pass. The target routine keeps the original clipped-point height and
mask rules, writes the authoritative chunky surface, and returns untouched
to the existing C fallback when fewer than two dirty slots remain.

## Bound priority traversal by populated maximum

`tmp/perf-priority-bound.log`: maximum work899 lines, maximum wall1118,
cadence412198/602 lines (43.9ms), all603 over budget. Final step568, audio11,
C2P69, diagnostics7; restore87, advance43, update136, HUD34, draw264.
The optimization removes the priority bitmap construction and bounds visits
by the highest indexed nonnegative signed-byte priority. Original saturated
pool priority/restoration tests pass, including high and negative priorities.

## Omit no-op index entries

`tmp/perf-active.log`: maximum work854 lines (54.7ms), maximum wall1061,
cadence367856/602 (39.2ms); 586/603 updates over budget. Final step500,
audio10, C2P69, diagnostics6; restore71, advance45, update141, HUD33, draw206.
Restore substages18/16/28/5; draw17/23/53/111. Checkpoint load:109 particles,
76 dirty pixels,2 dirty rectangles,3 equivalent C2P rows. Dirty-list overflow
is therefore not the checkpoint bottleneck.

Restoration indexes only saved actors; drawing indexes only live sprite
actors, while preserving point eligibility and stable ordering. Original
track/weapon/dirty regressions pass. No target errors/audio spills; owned
benchmark emulator exited. This is still not a general 50FPS result.

## Consecutive point batches

`tmp/perf-batch.log`: maximum work803 lines (51.5ms), maximum wall931,
cadence354128/602 (37.7ms),581/603 updates over budget. Final step450,
audio10,C2P69,diagnostics7; restore71,advance45,update141,HUD33,draw156.
109 particles/76 dirty pixels remain unchanged. Native batches retain
surface/mask/dirty pointers across points and stop at sprite actors; overflow
returns the untouched suffix to the existing fallback. Tests now cover 2048
single points and256 overlapping/reversed-order batches, partial overflow,
saved metadata and all preserved registers. Original rendering/dirty suites
pass; no target errors/audio spills, owned emulator exited.

## Native actor allocator

`tmp/perf-allocator.log`: maximum work790 lines, maximum wall918,
cadence353816/602 (37.7ms),586/603 over budget. Final step448, audio10,
C2P69,diagnostics6; restore71,advance45,update141,HUD33,draw154.
The compact byte scan matches the original x86 allocator directly over8192
pool states (returns, high-water and every slot), alongside the scalar
implementation. Improvement is limited in this benchmark, but the worst
frame is lower. Original actor/dirty suites and target build pass.

## Recycle actor metadata without clearing invalid backgrounds

`tmp/perf-recycle.log`: maximum work770 lines, maximum wall919,
cadence347888/602 (37.0ms),582/603 over budget. Final step437, audio10,
C2P69,diagnostics7; restore71,advance47,update130,HUD33,draw152.
Only actor metadata is cleared on reuse; saved=0 guards the old bytes until
the renderer captures every visible pixel. Inactive zero-state weapon slots
skip their no-op advancement. Original projectile, track/weapon actor and
dirty/restoration suites pass. Target reported no errors/audio spills.

## Prune sparse pixels covered by pending C2P rectangles

`tmp/perf-prune.log`: maximum work767 lines, maximum wall898,
cadence348822/602 (37.1ms),582/603 over budget. Whole-race cadence is not
meaningfully improved. Final C2P drops69 to59 lines, with sparse conversions
reduced76 to11. Step438,audio9,diagnostics7. The pruning cost is included in
the C2P interval. Capacity/edge/merged-rectangle coverage tests and planar
write checks pass. Both conversion paths still read the authoritative chunky
surface; no shadow framebuffer or per-row boolean array is introduced.

## Native car draw and restoration

`tmp/perf-native-cars.log`: maximum work759 lines (48.7ms), maximum wall888,
cadence341958/602 (36.4ms),573/603 over budget. Final step428,audio9,C2P59,
diagnostics7; restore64,advance48,update133,HUD32,draw147. Native routines
retain source/destination/saved pointers, apply rotation strides and colour
ramps, and preserve masking. 1024 scalar/native full-frame comparisons cover
all rotations, masking, ramps, full palette values, screen edges, saved
backgrounds, restoration and preserved registers. Original actor/dirty
regressions pass; no target errors/audio spills; owned emulator exited.

## Cache shared-particle map bases

`tmp/perf-map.log`: maximum work755 lines, maximum wall883,
cadence341646/602 (36.4ms),574/603 over budget. Final step424,audio9,C2P59,
diagnostics7; restore65,advance46,update130,HUD33,draw146. Mapping loops now
walk compact particle/handle arrays with cached counts and bases. The effect
is small; original actor and dirty-tracking suites pass.

## Native-setup benchmark coverage

`SLICKS_GAMEPLAY_BENCHMARK=0..3` selects BASIC/F1/CITY/WHACKO using
native setup and ordinary GO input. Four computer profiles select vehicles
5/2/0/0, setting100, Custom four laps, fuel10, damage300, weapons off,
seed0x1234. No moving-car state or framebuffer is injected. Target frame700;
the full-frame bitmap audit is disabled for timing, and emulated audio runs
with muted host output. This complements, not replaces, the legacy baseline.

First BASIC run `tmp/perf-native-basic.log`:603updates, all over20ms;
maximum work772lines (49.5ms), atframe209; maximumwall953,
cadence388563/602 (41.4ms). Final step378,audio7,C2P57,diagnostics9;
restore50,advance39,update81,HUD40,draw142.103particles,18dirty pixels,
6 sparse conversions,4rectangles,1equivalent C2P row. The last checkpoint
alone is not representative of the maximum.

## Worst-update stage capture

`tmp/perf-max-basic.log`: phase callbacks now run on every measured update,
including actor substages, and their overhead is included. Maximum783lines
atframe209: restore76,advance78,simulation230,HUD40,draw167,audio14,C2P117,
diagnostics13;148particles. Mean cadence391708/602lines (41.7ms).
Instrumentation can alter display-phase timing and BIOS-clock HUD updates;
compare subsequent runs using this same instrumentation. Measurements still
contain no per-update debugger stops. The maximum's simulation cost, not
only the final checkpoint's rendering, must be optimized.

## Cache unchanged HUD command inputs

`tmp/perf-hud-input.log`: maximum752lines atframe147, cadence382640/602
(40.7ms), all603 over budget. Final HUD7lines instead of40. Cache key is
lap, finish place, last/best lap words and status options; existing invalidation
and output comparisons remain. The original composed HUD oracle passes768
full-screen transitions plus dirty/cache and Arcade checks. Maximum-stage
audio170lines includes boundary-palette rebuilding, not just sound work;
that interval must not be attributed entirely to PCM playback.

## Incremental framework copper palette updates

`tmp/perf-palette.log`: maximum749lines atframe209; worst audio/palette
interval13lines instead of the earlier170-line rebuild spike. Mean cadence
381539/602lines (40.6ms), all603 over budget. Peak remains dominated by
simulation230, draw166 and C2P117lines. The framework still builds the full
display/copperlist. Palette MOVE indices are discovered from its output;
its existing `setColor` updates only the five changed high/low colour words,
without allocation or display-control changes. All banks/range boundaries
and262144 RGB values match framework encoding;20000 original boundary
updates match colours/timers/RNG. Target build and race complete without
reported errors or VBI spills.

## Split simulation timing

`tmp/perf-simulation.log`: maximum760lines atframe209, mean cadence
384347/602lines (40.9ms). The worst simulation236lines splits into
motion/control preparation50, weapons1 and per-car tails182lines.
All603updates remain over budget. The benchmark now rejects main-loop
audio blanking spills too, and raster reads retry a high-bit transition.
This is diagnostic evidence, not a completed performance target.

`tmp/perf-tail.log` further splits the worst tail: wheels93lines,
collision/surface27, smoke/contact61, finish6. Per-car callbacks add overhead;
the measured tail is193lines versus182 with only the outer markers.
Original composed trajectory (three7200-update scenarios), surface-effects
and car-collision oracles still pass with instrumentation enabled.

## Allocation-only emission cursor

`tmp/perf-emission-cursor.log`: worst work729lines (46.7ms), versus777 with
the same tail instrumentation. Tail193→143lines: wheels93→66,
smoke/contact61→43. Mean cadence386219/602lines (41.1ms); all603updates
still exceed20ms. The cursor is enabled only during allocation-only car
tails and discarded before actor retirement, preserving lowest-free-slot
selection.112640 batch selections match the DOS-verified scalar allocator;
8192 original/native allocation cases and all eleven7200-update composed
driving scenarios pass, as do surface-effect tuples and RNG checks.

## Native actor metadata clear

`tmp/perf-actor-reset.log`: worst719lines atframe222 (46.1ms), mean cadence
384970/602lines (41.0ms), still603 over budget. The new worst frame has171
particles: draw196, simulation166, C2P143, restore78 and advance61lines.
Reset clears the same33bytes with longword stores rather than a byte loop;
saved-under storage remains untouched.256 native canary/ABI cases pass,
along with8192 allocation cases,2432 original actor drawings,128 saturated
shared-pool reuse cases, and4096 original projectile-loop cases.

## Rejected standalone native shared-slot adapter

`tmp/perf-shared-adapter.log`: moving only the two surrounding slot-map
passes into assembly produced720lines maximum versus719, with mean cadence
385904/602lines. The advance substage saved only about one line.512 adapter
storage/ABI regressions and1920 original point-lifecycle comparisons passed,
but the extra production path and adapter-only test were removed because
there was no useful end-to-end improvement.

## Direct particle-order traversal

`tmp/perf-particle-chain.log`: final draw133→117lines; maximum720lines
atframe222 remains effectively unchanged, mean cadence384968/602lines.
The native point path now traverses the existing shared actor links, removing
the intermediate400-byte stack list and its construction pass. Sprite
boundaries and dirty-overflow suffixes still return to the existing caller.
2048 single,256 batch and256 scattered actor-chain pixel/metadata/dirty/ABI
cases pass, including early sprite boundaries; original actor/shared-pool
full-frame tests also pass. This is not an end-to-end50FPS result.

## Rejected packed foreground-mask experiment

`tmp/perf-packed-mask.log`: maximum719lines, mean cadence384681/602lines,
effectively unchanged from the direct-link baseline. Packing surface low
bits into material-map unused bits avoided another bitmap allocation, and
all eleven driving scenarios plus native packed-mask pixel checks passed.
Exhaustive material/visibility invariants and4864 original weapon images
also passed. Nevertheless the representation change and specialized loops
were removed: this representative test did not demonstrate a useful gain.

## Direct strided dirty-rectangle C2P

`tmp/perf-stride-c2p.log`: maximum717lines (46.0ms), mean cadence384347/602
lines (40.9ms). Worst C2P145→139lines against the direct-link baseline.
Kalms' converter now has a320-byte source-stride entry; dirty rectangles no
longer copy into packed scratch first. Its pipelined preload skips gaps
before reading the next row, but never skips after the final row.1419
rectangle/full-row/sparse cases verify every store and final byte, including
bottom/right edges; source reads stay inside64000+32bytes. Original packed
entry remains available and the particle lifecycle regressions still pass.
All603updates remain over budget.

## Sparse bitfield writes

`tmp/perf-sparse-bfins.log`: maximum709lines (45.4ms), mean cadence382793/602
lines (40.8ms). Still603 over budget. Sparse pixels use68020 BFINS with a
one-bit width rather than eight conditional set/clear branches.3467 writer
cases pass, including all256colours at all eight bit positions in the last
screen byte, preserving neighbours and subsequent rectangle conversion.
The maximum now occurs atframe209: C2P107lines, simulation195, draw178,
restore77 and advance78. Further simulation/rendering work is required.

## Rejected standalone native emission allocator

`tmp/perf-native-cursor.log`: maximum711lines versus709, mean cadence383126/602
lines.56320 consecutive native allocation comparisons passed, but moving the
existing cursor helper out of line did not improve the target measurement.
The experiment was removed; the earlier inline allocation-only cursor stays.

## Four-track rendering breakdown

`tmp/perf-draw-track-{0,1,2,3}.log` measures native-menu BASIC/F1/CITY/WHACKO,
603 updates each. Worst work in raster lines:716/1284/1014/791; mean cadence
in ms:41.0/79.9/61.0/44.6. All updates exceed20ms. Extra nested callbacks
separate car sprites from priority-three particles. F1's worst frame spends
463lines drawing, of which only41 are car sprites; high-priority actors
alone consume346. Restore275 and C2P263 are also substantial.

The first rectangle-area counter sampled after the dirty list was cleared,
so its zero values in those four logs are invalid. It now accumulates area
inside conversion and snapshots that value. `tmp/perf-draw-area.log` confirms
F1's worst update converts5440 rectangle pixels plus164 sparse pixels.
Car/dirty/shared-actor regression tests pass. General gameplay is clearly
slower than BASIC alone; stock-A1200 50FPS is still unfinished.

## Native track/effect sprite rectangles

`tmp/perf-native-sprites-{0,1}.log`: BASIC715lines maximum, F11183lines
(75.8ms), down from1285. F1 mean cadence676380/602lines (72.0ms), previously
79.9ms. The full-rectangle path reuses the native car blitter with colour
ramp zero and native restoration. Masked rectangles crossing row190 split
there to preserve the original cleared-mask tail. Clipped cases retain
the scalar path. Separate scalar and real68020-in-Unicorn runs match2432
original weapon images and128 shared-pool cases;1792 original track images,
64 ordering cases and permanent-mark restoration checks also pass. The
native test facade checks destination/saved-buffer write bounds and runs
the actual production fast path, not a duplicate blitter implementation.

## Wider saved-background restoration

`tmp/perf-wide-restore-{0,1}.log`: BASIC715lines maximum; F11174lines
(75.3ms), mean cadence667020/602lines (71.0ms). Restoration now uses
longwords with exact word/byte tails on68020, preserving arbitrary source
and destination alignment.512 additional width/height/alignment cases and
all original scalar/native actor comparisons pass. F1's parent restore
interval also includes moving-track-object simulation; it must not all be
attributed to copying saved pixels.

## Reuse track-contact pixel coordinates

`tmp/perf-motion-cache.log`: F1 maximum1160lines (74.4ms), mean cadence
647365/602lines (68.9ms), versus1174lines and71.0ms before. Car positions
are unchanged during track-object motion, so their signed divide-by100
results are computed once when movable objects exist. Actor coordinates
are similarly shared by rendering setup and contacts after the wall probe.
Velocities remain live between contacts.20000 original motion comparisons
and the full track-actor suite pass, including negative coordinates.

## Contiguous native sprite loop and cache sanity check

`tmp/perf-contiguous-sprites.log`: F1 maximum1153lines (73.9ms), mean
cadence640217/602lines (68.2ms). Contiguous sprites with no colour ramp
avoid rotation/tint operations; original masked and unmasked semantics
remain unchanged.1024 native car comparisons,512 restore cases,2432
original weapon images and1792 original track images pass, including
shared-pool ordering and permanent marks. Read-only Exec CacheControl(0,0)
reports0x1: the instruction cache was already enabled; no cache setting
is changed. All603 measured updates still exceed20ms.

## Materialize the particle emission base

An empty68020 address-register constraint makes GCC calculate the particle
base once instead of encoding the enclosing runtime's68256-byte displacement
in every field store. Disassembly confirms short-offset stores with no extra
runtime helper. Original surface-emission and collision tests pass.
`tmp/perf-particle-base.log`: BASIC709lines maximum (45.4ms), mean cadence
383415/602lines (40.8ms). This includes the preceding track-coordinate and
sprite changes since BASIC's715-line baseline, so the small combined gain
must not be attributed solely to this addressing change.

## Rejected size-optimized gameplay build

`tmp/perf-runtime-size.log`: changing only the gameplay compilation from
O3 to Os worsens BASIC to843lines maximum and410851/602lines mean cadence
(43.7ms). Phase timings also worsen; smaller code is not automatically
faster here. The production O3 setting is restored.

## Rejected native particle restoration chain

`tmp/perf-restore-chain.log`: BASIC maximum709lines unchanged; mean cadence
384353/602lines slightly worse than383415/602. A native ordered point
restoration helper passed512 overlap/state/sprite-boundary/ABI cases, but
did not improve the complete update. The helper and its dedicated tests
were removed; production retains the existing restoration path.

## Stronger display audit for dirty-region optimization

The earlier bitmap audit only checked writes outside declared dirty areas;
it could not detect a missing conversion. Debug audit runs now reuse their
existing snapshot buffer for a full reference C2P and compare all64000
display bytes after the bounds check. There is no new production shadow
buffer or full-frame conversion. `tmp/audit-missing-control.log` confirms
that the native BITMAPMISS positive control detects its deliberately omitted
pixel at frame1,x0,y100,plane0. The bounded `diag_dirty_sprites.gdb` fixture
checks600 native updates without debugger memory writes.

This stronger check found the same stale pixel at frame111,x312,y169,plane1
both with unchanged-sprite suppression (`tmp/audit-sprite-dirty-full.log`)
and with the preceding dirty-marking behavior (`tmp/audit-sprite-control.log`).
Thus that failure predates the suppression optimization. The tracing fixture
reports nearby actors and dirty rectangles to distinguish conversion failures
from missing producer notifications.

## Retiring particles and unchanged sprite conversions

The shared draw traversal incorrectly included retiring trail slots. A
transient point was redrawn after expiry, rearming its saved-under flag;
the next release pass restored it without an associated dirty notification.
The original3000:3c1e drawing gate is now executed for all256 state bytes,
independently confirming that only signed-positive slots enter active drawing.
Both ordered and fallback shared draw paths now apply that gate. Permanent
marks still bake during expiry; retiring slots remain reserved until release.
Original retirement/allocation/creation tests, mixed sprite/point screens,
and explicit retired transient/permanent dirty regressions pass.

Sprite restoration/drawing stays unchanged in the authoritative chunky
buffer. A small per-handle metadata list now defers display dirty rectangles
until redraw: unchanged position/frame/style/priority/mask needs no extra
conversion; moved, changed, clipped, removed and recycled sprites retain
old/new coverage. No pixel shadow or per-row booleans are used.320 new dirty
coverage cases include overlap, frame/mask/priority changes and slot reuse.

`tmp/audit-retired-point.log`: full display equality and write-bounds audit
passes600 native F1 updates,32 track actors and2076 marks. Before the particle
fix, the suppression benchmark (`tmp/perf-sprite-dirty-batch.log`) measured
1021lines maximum and569040/602lines mean cadence (60.6ms), versus68.2ms.
Those timing numbers precede the lifecycle correction; fresh production
benchmarks are required, and50FPS remains unfinished.

## Four-track baseline after dirty/lifecycle fixes

`tmp/perf-dirty-final-{0,1,2,3}.log`,603 measured updates per track:

| Track | Maximum work (raster lines / ms) | Mean cadence (ms) |
| --- | --- | --- |
| BASIC | 685 / 43.9 | 40.1 |
| F1 | 1019 / 65.3 | 60.5 |
| CITY | 824 / 52.8 | 46.5 |
| WHACKO | 708 / 45.4 | 40.6 |

Cadence sums are376868/567766/436231/381587 over602 intervals. All603
updates on every track exceed20ms. These runs exclude the expensive debug
bitmap audit, retain profiling, use four native-menu CPU drivers, and run
muted sequentially with emulator cleanup after each. The performance goal
is not complete; F1 still spends352lines drawing at its worst update.

## Cached sprite visibility and native word merges

Track assets now carry decoded transparency metadata. In-bounds sprites use
native long/word/byte merges that save the original chunky background. Masked
sprites cache their foreground visibility by handle, position, asset, frame
and occlusion threshold; cache collisions regenerate it, and race startup
invalidates all entries. No displayed-pixel shadow is introduced.

4096 native merge/ABI/canary cases, the original 2432 weapon and 1792 track
render comparisons (including repeated cache hits and forced collisions),
and a 600-update full native display audit pass. See local logs
`tmp/verify-visibility-cache-hits.log` and `tmp/audit-cached-visibility.log`.

F1 (`tmp/perf-cached-visibility.log`) measures 991 lines maximum work
(63.5 ms) and 563710/602 lines mean cadence (60.0 ms), versus 1019 lines
and 60.5 ms previously. The gain is small, and the 20 ms goal remains open.
A tested transparent-run implementation did not improve the baseline and
was discarded (`tmp/perf-sprite-spans.log`).

Optional `SLICKS_PROFILE_SPRITES` probes split sprite setup, painting and
dirty bookkeeping. They measured 66/107/75 lines at the worst F1 update,
but increased maximum work to 1093 lines, so are disabled by default.
Do not compare those detailed runs directly with lighter production profiles.

Correctness-only debug runs may use `SLICKS_DEBUG_WARP=1`; the runner rejects
this option for gameplay benchmarks. The missing-pixel positive control also
passes in warp mode (`tmp/audit-warp-control.log`). Normal run.sh audio and
benchmark pacing are unchanged.

Materializing the reusable actor, visibility-cache and previous-description
addresses in 68020 address registers reduces repeated indexed addressing.
`tmp/perf-sprite-bases.log`: F1 maximum 982 lines (63.0 ms), mean cadence
561188/602 lines (59.8 ms). Native sprite merges and original weapon/track
render tests plus dirty-coverage tests pass (`tmp/verify-sprite-bases.log`).
The full native display audit also passes 600 updates, 32 actors and 2076
marks (`tmp/audit-sprite-bases.log`).

Limiting optimization of the two sprite draw wrappers to `O2` keeps the
rare scalar fallback from expanding the shared draw loop. Physics remains
at `O3`. F1 (`tmp/perf-compact-draw.log`) measures 970 lines maximum work
(62.2 ms), with 557470/602 lines mean cadence (59.4 ms). The shared draw
function shrinks from 4516 to 1840 bytes. Original track-render comparisons
now also repeat draws with deferred dirty metadata and deliberate cache
collisions (`tmp/verify-deferred-cache.log`).
The full display audit passes 600 updates (`tmp/audit-compact-draw.log`).

A separate unchanged-track-sprite wrapper passed original pixel comparisons
but increased F1 maximum work from 970 to 979 lines and cadence sum from
557470 to 559966 (`tmp/perf-unchanged-track.log`). Its repeated validation
and extra call outweighed the simpler geometry path; it was removed.

## Reuse draw ordering for the following restoration

Between drawing actors and their next restoration, the production loop does
not run actor simulation or allocation. Reverse the actual per-priority draw
chains instead of rescanning actor state and priorities. Saved-background
validity still gates restoration; fresh startup and callers without a prior
draw retain the independent scan. Pool initialization invalidates reuse.

512 mixed active/retired/clipped sprite and point ordering cases match the
independent restoration scan. Original weapon/track render, overlapping
shared pools, permanent marks and dirty coverage pass
(`tmp/verify-reverse-order.log`). F1 (`tmp/perf-reverse-order.log`) improves
to 947 lines maximum work (60.7 ms), and 551229/602 lines mean cadence
(58.7 ms). The restoration-order phase falls from 15 to 2 lines in the final
sample. The 20 ms performance goal is still unfinished.
The full native display audit passes 600 updates, 32 actors and 2076 marks
(`tmp/audit-reverse-order.log`). A new baseline regression also verifies
point allocation/drawing/restoration and subsequent sprite reuse for all
199 handles with deliberately poisoned stale sprite metadata.

Reducing point-allocation sprite reset to just its kind/saved flags passed
the poisoned-slot and original effect/render tests, but did not improve the
end-to-end result: 947 maximum lines and 553388/602 mean cadence lines
(`tmp/perf-point-reset.log`). Keep the full metadata reset. Its additional
display audit was cancelled and its owned emulator closed after the timing
rejection; no audit completion is claimed for this discarded experiment.

## Retained sprite optimizations: four-track recheck

`tmp/perf-retained-sprites-{0,1,2,3}.log`, 603 measured updates each:

| Track | Maximum work (lines / ms) | Mean cadence (ms) |
| --- | --- | --- |
| BASIC | 674 / 43.2 | 39.6 |
| F1 | 946 / 60.6 | 58.7 |
| CITY | 775 / 49.7 | 42.8 |
| WHACKO | 712 / 45.6 | 40.0 |

Cadence sums are 372170/551230/401494/375635 over 602 intervals. All measured
updates still exceed 20 ms. Debug sessions were muted, sequential and closed
after each run. Stock PAL 68020/2 MiB Chip RAM settings are unchanged.

`make verify-particle-advance` now runs the actual native update assembly
against original DOS lifetime and signed-word motion fragments for 514
batches (0..256 particles on both pages). It also checks compaction, full
saved particle records, permanent pixels, dirty saturation, priority buckets,
canaries and the callee-saved ABI. The original retirement oracle separately
checks all 256 state bytes. Reused Unicorn execution requires explicit
instruction hooks at fragment boundaries; varying `until` alone initially
let cached translated blocks run through a boundary in the test harness.
