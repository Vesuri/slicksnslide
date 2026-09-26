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
