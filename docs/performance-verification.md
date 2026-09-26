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

## Omit redundant legacy particle buckets

The shared actor pool already owns draw ordering. Particle creation and
advancement no longer also construct four unused legacy priority buckets.
Isolated-particle callers retain those buckets; the rare shared dirty-capacity
fallback scans the dense particle pool for priority-zero permanent marks.

1028 native legacy/shared update batches pass the DOS-backed oracle, alongside
original weapon/track rendering, surface effects, dirty coverage and poisoned
slot reuse (`tmp/verify-shared-buckets.log`). BASIC measures 667 maximum work
lines (42.8 ms), versus 674, with cadence sum 371876/602 (39.6 ms);
advancement at the worst frame falls from 77 to 68 lines
(`tmp/perf-shared-buckets.log`). This is a small reduction, not a 50 FPS result.
The full native display audit passes 600 updates, 32 actors and 2076 marks
(`tmp/audit-shared-buckets.log`).

A fused native handle-remapping experiment passed 1428 batch cases, including
all shared state/index/handle arrays and guards, but was slower: BASIC maximum
685 lines and cadence 375602/602, versus 667 and 371876/602. Final-frame
advancement rose from 40 to 50 lines (`tmp/perf-fused-particle-maps.log`).
It was removed; the simpler two C mapping passes remain.

Whole-runtime `O2` was also rejected: although the step function shrank from
about 34 KiB to 18 KiB, BASIC worsened to 769 maximum lines and 393092/602
cadence lines (`tmp/perf-runtime-o2.log`). Keep `O3` with the two measured
compact draw wrappers. The runtime object now depends on its Makefile so
changing these compiler options actually rebuilds it.

## Paint status bars in their final colour

The old status renderer cleared fuel/damage bars and repainted them even when
their final pixels were unchanged, producing unnecessary sparse conversion.
Normal bounded bars now resolve each row's final foreground/background before
writing. Wrapped/out-of-range widths retain the original ordered-rectangle
fallback; no framebuffer shadow is used.

Original 768 full-screen HUD transitions, 8640 weapon-HUD command/fault cases,
dirty saturation, exact VGA/chunky output and repeated unchanged status draws
pass (`tmp/verify-final-status-bars.log`). BASIC's worst update drops from
667 to 608 lines (39.0 ms), with cadence 344176/602 lines (36.6 ms), and
588/603 rather than 603/603 updates exceeding 20 ms. At the worst update,
sparse conversion drops from 139 to 9 pixels and C2P from 92 to 43 lines
(`tmp/perf-final-status-bars.log`). The overall performance goal remains open.
The full display audit passes 600 updates (`tmp/audit-final-status-bars.log`).

Caching just the final bar widths/colours avoids rereading unchanged pixels
when neither producer rectangles nor sparse dirty points touch the strip.
Startup and fallback rendering invalidate this metadata. Original HUD and
explicit point/rectangle repaint-invalidation tests pass
(`tmp/verify-status-cache-final.log`). BASIC improves to 595 maximum lines
(38.1 ms), cadence 335128/602 (35.7 ms), with 574/603 updates over budget
(`tmp/perf-status-cache.log`). No screen-pixel shadow or per-row flags are used.
The full display audit passes 600 updates, 32 actors and 2076 marks
(`tmp/audit-status-cache.log`).

Collision bursts now reuse their car position and signed base velocity
divisions across all emitted points. Original damage/contact/effect comparisons
pass (`tmp/verify-contact-divisions.log`). The measured difference is marginal:
BASIC 596 maximum lines and cadence 334796/602 versus 595 and 335128/602;
the collision/smoke phase at that worst update falls from 42 to 40 lines
(`tmp/perf-contact-divisions.log`). RNG ordering and full-pool behavior remain
unchanged; no large speedup is claimed for this small arithmetic cleanup.

## Native unchanged-track dispatch

The common unchanged, in-bounds track-sprite path validates geometry/style
and cached visibility in 68020 assembly before calling the existing native
pixel merge. It still saves and redraws the authoritative chunky pixels;
it does not retain sprites only in the display bitplanes. Clipping, changed
metadata and cache misses fall back to the general renderer.

The 4096 native opacity/visibility cases and original weapon/track full-screen
comparisons pass, including 464 direct uses of the new dispatcher. Native
tests check preserved registers and exact permitted write bounds
(`tmp/verify-native-track-fast-final.log`). Absolute calls use normal ELF
relocations in production and an explicit load base in the flat test image.

Matched F1 runs with the new dispatcher disabled/enabled give 857/836 maximum
work lines and 496420/478928 cadence lines over 602 intervals: 52.9/51.0 ms
average cadence (`tmp/perf-native-track-control.log`,
`tmp/perf-native-track-fast.log`). Every measured update remains over 20 ms.
The control build was replaced with the default optimized build afterward.
The full native display audit passes 600 updates, 32 actors and 2076 permanent
marks (`tmp/audit-native-track-fast.log`). Track motion, particle advancement
and dirty-region regression suites also pass
(`tmp/verify-track-fast-regression.log`).

Materializing the actor-configuration address in a register was tested and
removed: original actor tests passed, but F1 maximum work rose from 836 to
841 lines and cadence from 478928 to 479240/602
(`tmp/perf-config-address.log`). Fewer indexed stores did not improve the
whole update.

Explicitly sharing the two wheel-centre divisions also passed the surface,
off-road and original damage tests but showed no improvement: BASIC maximum
598 lines, cadence 338560/602, and the worst-frame wheel phase still 68 lines
(`tmp/perf-wheel-centres.log`, `tmp/verify-wheel-centres.log`). Removed rather
than retaining an unmeasured arithmetic improvement.

Compiling only track-object motion at `O2` was slower too: F1 859 maximum
lines and 489224/602 cadence, with the restore-plus-motion interval rising
from 183 to 202 lines at the worst update (`tmp/perf-track-motion-o2.log`).
Restored its original `O3` compilation.

Explicit signed divide-by-100 instructions were also rejected. The compiled
long-quotient helper matched x86 IDIV over 231083 cases; a bounded word-quotient
variant with full-width fallback matched 231089, including its exact range
edges. BASIC was 597 maximum lines / 336688 cadence lines for the long variant
and 612 / 350708 for the word variant, versus the retained 596 / 334796
(`tmp/perf-div100-basic.log`, `tmp/perf-div100-word-basic.log`). The compiler's
reciprocal multiplication remains. Experiment-only helpers/tests were removed;
arithmetic evidence remains in the ignored verification logs.

Dedicated four/eight-pixel opacity and visibility row loops passed 4096
native cases and the original full-screen actor comparisons, but did not
improve F1: 837 maximum lines and 480485/602 cadence versus 836/478928
(`tmp/perf-small-sprite-rows.log`). Removed. Asset inspection confirms these
widths are used, alongside five/six-pixel objects and nine-to-eleven-pixel flags;
the failed measurement is not evidence that the small rows were unreachable.

## Native sprite restoration setup

In-bounds deferred restoration now validates and copies the saved background
and twelve-byte dirty description in assembly. Clipped/empty rectangles retain
the scalar fallback. Original comparisons exercise 1312 native restorations;
additional rejection tests include positive signed-word coordinate overflow,
negative coordinates and empty dimensions (`tmp/verify-native-restore-final.log`).
The full display audit passes 600 updates, 32 actors and 2076 permanent marks
(`tmp/audit-native-restore-setup.log`). F1 maximum work is unchanged at 836
lines; mean cadence falls slightly from 478928 to 475184/602 lines, 51.0 to
50.6 ms (`tmp/perf-native-restore-setup.log`). This is a small gain, not a
completed 20 ms target.

## Cache-sized particle drawing loop

Particle batches keep the continuation in an address register and publish
their dirty count at the batch boundary, instead of doing a stack call/return
and count store for every point. The chain walker, paint body and old-point
queue are adjacent within 256 bytes, avoiding the previous walker/paint-code
instruction-cache alias. Single-point and overflow behavior remain intact.
All 2048 single, 256 ordered-batch and 256 actor-chain native pixel/metadata/
dirty-list/ABI comparisons pass (`tmp/verify-particle-cache-loop.log`).

BASIC's worst work drops to 551 lines (35.3 ms), with the 148-particle worst
frame's draw interval 142 rather than 185 lines. Mean cadence is only slightly
lower: 333860/602 lines (35.5 ms), and 579/603 updates still exceed 20 ms
(`tmp/perf-particle-cache-loop-basic.log`). The maximum-work reduction must not
be presented as a comparable gain in average displayed frame rate.
The full native display audit passes 600 updates, 32 actors and 2076 permanent
marks (`tmp/audit-particle-cache-loop.log`).

Moving particle retirement and legacy-bucket code outside the contiguous
shared-pool motion loop passes all 1028 original/native update batches
(`tmp/verify-particle-motion-cache.log`). BASIC improves further to 546 maximum
lines (35.0 ms), cadence 332320/602 lines (35.4 ms). At the same worst frame,
advancement falls from 65 to 58 lines (`tmp/perf-particle-motion-cache-basic.log`).
With both particle changes, F1 measures 788 maximum lines and 466760/602
cadence lines (49.7 ms), compared with the earlier 836/475184
(`tmp/perf-particle-cache-f1.log`).

## Stationary movable track objects

Zero-velocity objects still sample material/layer, configure their sprites
and test every car contact. They bypass only the original no-callback
zero-length ray (`exclude=0`) and zero-valued damping. Another 20000 explicit
stationary-object cases bring original motion/layer/contact verification to
40000 (`tmp/verify-stationary-track-motion.log`). F1 falls from 788 to 776
maximum work lines, and cadence from 466760 to 449288/602 lines (47.8 ms)
(`tmp/perf-stationary-track-motion.log`).
The combined particle-motion/stationary-object build passes the 600-update
full display audit with 32 actors and 2076 permanent marks
(`tmp/audit-stationary-track-motion.log`).

## Point-slot initialization: BASIC recheck

The earlier F1-only rejection of clearing just sprite kind/saved validity was
revisited after particle drawing improvements, using BASIC's heavier creation
load. All 199 poisoned-slot transitions and original effect/damage tests pass
(`tmp/verify-point-reset-basic.log`). BASIC improves from 546 to 541 maximum
lines and from 332320 to 330429/602 cadence lines (35.2 ms), with the busy
wheel/contact phases falling from 69/41 to 66/38 lines
(`tmp/perf-point-reset-basic.log`). Retained for this measured small gain.
Point motion comes exclusively from the particle record; subsequent sprite
allocation still resets the complete sprite metadata.

## Word-aligned saved backgrounds

Move existing padding/validity bytes ahead of the car and actor background
arrays, without increasing either structure's size. Actor reset now clears
34 bytes including padding; native setup offsets and their write guards are
updated together. Native allocation/reset, car and original actor rendering,
dirty tracking and poisoned-slot tests pass (`tmp/verify-aligned-backgrounds.log`).

BASIC is unchanged at 541 maximum lines and 330428/602 cadence. A matched
F1 alignment-only control gives 774/443361 versus aligned 771/440294 lines,
about 0.3 ms lower mean cadence (`tmp/perf-background-align-control-f1.log`,
`tmp/perf-aligned-backgrounds-f1.log`). The complete display audit passes 600
updates, 32 actors and 2076 marks (`tmp/audit-aligned-backgrounds.log`), also
covering the retained minimal point-slot initialization. The emulator log
confirms 68020, real speed, cycle-exact mode and disabled JIT; these remain
emulator measurements, not physical-machine measurements.

## Native collision-burst experiment (rejected)

A complete native RNG/allocation/particle-initialization burst passed 4870
original/scalar-composition cases, including full records, allocation failure,
colour thresholds, guards and ABI. Although the local smoke/contact interval
fell to four lines, full BASIC cadence regressed from 330428/602 to
336356/602 lines. Tightening the hot loop to fit the instruction cache still
gave 336044/602 and 548 maximum work lines, versus the retained 541.
The candidate was removed; local speed alone did not justify retaining it.
Evidence: `tmp/verify-native-contact-compact.log`,
`tmp/perf-native-contact-burst-basic.log`, and
`tmp/perf-native-contact-compact-basic.log`.

Word-aligning track assets (259 to 260 bytes) also passed the original sprite,
weapon and track-actor checks, but F1 cadence increased from 440294/602 to
443050/602, with maximum work 772 rather than 771 lines. Removed rather than
retained on an alignment assumption (`tmp/verify-track-asset-alignment.log`,
`tmp/perf-track-asset-alignment-f1.log`).

## Race-wide stage totals

The benchmark now sums measured work/stages over all 603 racing updates,
not just the maximum-work snapshot. Collection runs after the work timer,
only in benchmark mode; its bookkeeping is still included in cadence, so
future cadence comparisons need this same instrumentation on both sides.
BASIC has 233647 total work lines (24.8 ms average work), including 84802
car-update, 58008 draw and 23918 C2P lines. F1 has 389278 (41.4 ms average
work), including 138756 draw, 98218 restore/track-motion, 78465 car-update
and 28180 C2P lines. Mean cadence remains 35.3/47.3 ms, respectively.
Evidence: `tmp/perf-average-stage-basic.log`, `tmp/perf-average-stage-f1.log`.

## Register-entry track sprite painting

The native unchanged-sprite dispatcher now enters the existing pixel loops
with register arguments, avoiding another C argument frame and duplicate
register saves. Public C entries retain their ABI. Original sprite, weapon
and track rendering tests pass (`tmp/verify-register-sprite-entry.log`).
F1 drawing totals drop from 138756 to 133634 lines, total measured work from
389278 to 384217, maximum from 772 to 763, and cadence from 444353/602 to
433689/602 (46.2 ms). The complete display audit passes 600 updates,
32 actors and 2076 marks (`tmp/perf-register-sprite-entry-f1.log`,
`tmp/audit-register-sprite-entry.log`). Still above the 20 ms target.

## Material-first masked car pixels

The native masked pixel loops compare material to the mask's upper bits first;
only equality reads/tests the surface's low three bits. The source pixel is
loaded once, and mask zero retains its unmasked meaning. Existing car,
weapon and surface oracles pass, plus 768 new cases covering every mask byte
at material/residual boundaries (`tmp/verify-mask-material-first.log`,
`tmp/verify-mask-boundaries.log`). Matched BASIC control/candidate totals are
233522/232434 work lines and 57841/56921 drawing lines over 603 updates,
about 0.12 ms less work per update. Cadence improves from 331073/602 to
329825/602; maximum work is effectively unchanged at 536/537 lines.
The full display audit passes 600 updates, 32 actors and 2076 marks
(`tmp/perf-material-control-basic.log`, `tmp/perf-mask-material-first-basic.log`,
`tmp/audit-material-first.log`).

Hoisting per-car force denominators, velocity factors and heading across
physics quanta passed original integration/damage checks and all eleven
7200-update trajectories (`tmp/verify-physics-constants.log`), but increased
BASIC total work from 232434 to 233177 lines and preparation from 36465 to
37318. Cadence also worsened, 329825/602 to 332009/602. Removed the candidate
(`tmp/perf-physics-constants-basic.log`).

## Bounded native memory primitives

Replace the linked byte-at-a-time memcpy/memset with 68020 block loops,
preserving arbitrary alignment and exact bounds. The external support file
is untouched; its old definitions are renamed locally at compilation and
discarded by section collection. Entries use .text because elf2hunk cannot
resolve cross-section PC-relative references to the standard runtime names.
24912 copy/clear/fill tests cover all small lengths, alignments, lengths over
64 KiB, exact read/write bounds, full bytes/canaries and ABI
(`tmp/verify-native-memory.log`).

BASIC work decreases from 232434 to 232027 lines over 603 updates; cadence
from 329825/602 to 329201/602, a modest gain rather than a major bottleneck
removal (`tmp/perf-native-memory-basic.log`). The full 600-update display audit
passes with 32 actors and 2076 marks. Native mode-1 menu/race transitions pass
two race starts, two pause menus, restoration 31 and zero audio spills
(`tmp/audit-native-memory.log`, `tmp/modes-native-memory.log`).

## Reject inactive ordering slots early

Build draw-order chains only after checking positive slot state, and keep
the maximum priority local until construction completes. Reverse restoration
still includes saved retired actors. Original weapon/track ordering and full
pixel tests pass (`tmp/verify-local-actor-max.log`). BASIC draw totals fall
from 56566 to 55479 lines; total work from 232027 to 231355, cadence from
329201/602 to 327955/602. The cumulative F1 build measures 381398 total work
lines, 756 maximum, and 426201/602 cadence lines (45.4 ms); this F1 comparison
also includes the previously retained mask/memory changes.
The full display audit passes 600 updates, 32 actors and 2076 permanent marks
(`tmp/perf-local-actor-max-basic.log`, `tmp/perf-local-actor-max-f1.log`,
`tmp/audit-local-actor-max.log`).

## Compact working-state layout

Move bulk resources/maps behind private native working state, and put weapon
slot lookups ahead of sprite storage. Car state moves from offset 65974 to
3168; RNG/order fields move from about 200 KiB to 15 KiB. Target assertions
keep key working fields within short displacement range. No game-file layout
or existing particle/sprite record ABI changes. The car-preparation function
shrinks from 14774 to 13086 code bytes.

The first layout lost four-byte particle alignment and regressed advancement.
Explicit alignment for particle and visibility arrays restores that cost;
total runtime storage remains 216220 bytes, unchanged from the control.
Original actor, HUD, collision and surface checks pass, as do rebuilt native
car/particle/actor and dirty-region comparisons (`tmp/verify-hot-state-layout.log`,
`tmp/verify-hot-state-aligned.log`). This work exposed a verifier-only map
initialization overrun; its separate bounds fix passes AddressSanitizer
(`tmp/verify-surface-effects-asan.log`).

Aligned BASIC total work is 225138 lines over 603 updates, maximum 518 and
cadence 315160/602 (33.6 ms), versus 231355/534/327955. F1 is
370620/737/410912 (43.8 ms cadence), versus 381398/756/426201.
The full display audit passes 600 updates, 32 actors and 2076 marks
(`tmp/perf-hot-state-aligned-basic.log`, `tmp/perf-hot-state-aligned-f1.log`,
`tmp/audit-hot-state-aligned.log`). Neither track meets the 20 ms target.
Native mode-4 transitions also pass two starts, two pauses, restoration 31
and zero audio spills (`tmp/modes-hot-state-aligned.log`).

## Aligned car state and actor saved pixels

Group each car's 32-bit working values and align its saved pixels/stride to
four bytes; the car shrinks from 302 to 300 bytes. Original physics, damage,
HUD, collision, surface and native car rendering checks pass
(`tmp/verify-aligned-car-state.log`). BASIC work/cadence improve from
225138/315160 to 224058/312334 lines; F1 from 370620/410912 to
369127/409997 (603 updates, 602 cadence intervals).

Actor saved pixels move to offset 36 with a 164-byte stride. Native reset,
render offsets, assertions and oracle guards change together. Original and
native actor/reset/dirty checks pass (`tmp/verify-aligned-actor-buffers.log`).
This second change is effectively neutral on BASIC (224135 work lines,
312334 cadence), while F1 falls to 368189 work / 409352 cadence lines.
The combined runtime uses 392 additional bytes. Full native pixel auditing
passes 600 updates, 32 actors and 2076 permanent marks
(`tmp/audit-aligned-car-actor.log`). Timing logs are
`tmp/perf-aligned-car-state-{basic,f1}.log` and
`tmp/perf-aligned-actor-buffers-{basic,f1}.log`.

## Prepared native car sprites

Prepare all sixteen rotations and player colour ramps from the loaded car
assets at race startup. Transparent pixels retain their original opacity,
including synthetic ramp wrap-to-zero. An 8x8-tile foreground maximum permits
unmasked longword drawing only when the whole rectangle is provably visible;
otherwise the prepared pixels use the exact native foreground-mask primitive.
Unsupported ramp-wrap cases retain the original general path. Reloading a car
asset invalidates its cache even if decoding fails; start invalidates the atlas
and the platform rebuilds it after the new maps/configuration are ready.

The cache uses 14996 bytes (15000 additional runtime bytes including alignment).
Native 68020 tests compare 4096 rotations/colours/sizes/edge/mask cases with
the established scalar renderer, including 2096 unmasked assembly calls,
exact saved backgrounds, write guards and ABI checks. These are compositional
rendering checks, not a new whole-car DOS oracle. Existing native car and
surface/dirty/planar checks also pass (`tmp/verify-car-cache-final.log`,
`tmp/verify-car-cache.log`, `tmp/verify-car-cache-integration.log`).

The initial unmasked-only candidate did not improve BASIC overall. Adding
the masked contiguous path reduces BASIC work from 224135 to 222732 lines
over 603 updates (23.68 ms average), cadence 312334 to 312041/602. F1 work
falls from 368189 to 364440 (38.74 ms), cadence 409352 to 405920/602.
Worst work is still 513/729 lines, respectively; the 20 ms target is unmet.
See `tmp/perf-car-cache-v2-{basic,f1}.log`. The complete display audit passes
600 updates, 32 actors and 2076 permanent marks (`tmp/audit-car-cache.log`).
Native mode-4 transitions pass two starts, two pauses, full restoration (31)
and zero audio spills (`tmp/modes-car-cache.log`).

## Track-object contact bounds

Build a conservative rectangle enclosing the original contact areas of cars
whose measured speed exceeds 300. Objects outside it still run original
movement, material/layer, damping and sprite configuration, but cannot collide
with any car and skip the four detailed tests. Positions that could trigger
the original signed-word distance wrap disable this shortcut. Initialize the
bounds only when the first movable object needs them. All 40000 original DOS
motion/contact cases pass, with added far-away and wrapped-coordinate cases;
the 200 original flag configurations also pass
(`tmp/verify-contact-bounds-expanded.log`).

Final BASIC work/cadence are 223370/312665 lines, compared with 222732/312041:
about 0.07 ms more work per update. F1 improves from 364440/405920 to
358163/404685: about 0.67 ms less work, with restoration/motion falling from
91921 to 84991 lines. This is retained for the substantially larger F1 gain,
not represented as a BASIC improvement. Both use 603 updates and 602 cadence
intervals (`tmp/perf-contact-bounds-v2-{basic,f1}.log`).
The full display audit passes 600 updates, 32 actors and 2076 permanent
marks (`tmp/audit-contact-bounds.log`). The 20 ms objective remains open.

## Rejected homogeneous sprite-block branches

Native branches for fully transparent/opaque longwords preserve saved pixels
and pass original/native actor and car tests (`tmp/verify-sprite-homogeneous.log`).
They nevertheless increase F1 work from 358163 to 361052 lines, draw from
123057 to 124880 and cadence from 404685 to 405920/602. Keeping only the
foreground-masked variant is also worse: 361478 work, 125631 draw and
406842/602 cadence (`tmp/perf-sprite-homogeneous-f1.log`,
`tmp/perf-sprite-visible-blocks-f1.log`). Both variants are removed; production
retains the straight-line longword blend.

## Rejected straight-driving steering gate

Skipping the pure steering-delta calculation when neither turn bit is set
regresses BASIC work from 223370 to 224560 lines and cadence from 312665 to
316390/602; preparation rises from 34183 to 35743 lines
(`tmp/perf-straight-steering-basic.log`). The branch candidate is removed.

Exact keyed reuse of the unit-tick steering calculation is also rejected.
One million signed-key/tick-wrap checks and eleven original 7200-update
driving scenarios pass (`tmp/verify-steering-cache-keys.log`,
`tmp/verify-cached-steering.log`), but BASIC work increases to 224226 lines
when inlined, or 224744 with a separate helper, versus 223370. Cadence is
315473/315785 rather than 312665/602. Both the cache and its experimental
test are removed (`tmp/perf-cached-steering-basic.log`,
`tmp/perf-cached-steering-leaf-basic.log`).

## Batched native sprite restoration

A read-only debugger count on F1 updates 200–229 finds 960 sprite draws,
930 fast-path attempts and 911 successes (`tmp/sprite-counts.log`). The fast
path is already used; repeated native/C call setup is worth removing.

Restore consecutive sprites in one 68020 traversal, retaining the exact
priority-chain order and returning the first particle, clipped, empty or
unsaved sprite to the existing path. A register-entry restoration primitive
avoids repeated register saves and C argument frames. The new 384-case native
oracle checks 6696 restorations, all pixels/actor metadata/descriptors/dirty
handles, exact permitted writes, returned fallback handles and the ABI.
Existing original/native actor and 4096 sprite-opacity tests also pass
(`tmp/verify-sprite-restore-regs.log`).

F1 work decreases from 358163 to 340527 lines over 603 updates (36.20 ms
average), restoration/motion from 84991 to 67444, cadence from 404685 to
389697/602. Maximum work falls from 732 to 703 lines. BASIC total work is
effectively unchanged, 223370 to 223400, cadence 312665 to 313894/602.
See `tmp/perf-sprite-restore-chain-{basic,f1}.log`. The 20 ms target remains
unmet on both tracks.
The full display audit passes 600 updates, 32 actors and 2076 permanent
marks (`tmp/audit-sprite-restore-chain.log`); the batch oracle output is in
`tmp/verify-sprite-restore-chain.log`.
