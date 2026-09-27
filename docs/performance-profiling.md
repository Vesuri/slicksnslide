# Target-side profiling and native-replacement verification

Measured evidence for the 2026-09-27 performance work. Current actionable
work stays in [open-work.md](open-work.md).

## Exact HUD row repainting (2026-09-28)

The bounded status renderer uses a twelve-bit car/row repaint mask. Changed
ends or colours invalidate only their row; background, participation or
cold-cache transitions invalidate all necessary rows. Producer rectangles
and sparse pixels invalidate intersecting cells. Weapon icons and the
unbounded-width fallback retain their existing behavior. No shadow surface
is used in normal gameplay.

Initial normal-build comparison against 3bd216f, 603 updates per track:

| Track | Parent work / worst | Row-mask work / worst |
| --- | ---: | ---: |
| BASIC | 159205 / 369 | 158617 / 370 |
| F1 | 184429 / 459 | 183720 / 448 |
| CITY | 153622 / 361 | 153057 / 354 |
| WHACKO | 162979 / 423 | 162122 / 422 |

Logs: `tmp/hud-rows-20260927-{0,1,2,3}.log`. Final states match the
controls. Total savings are 0.4-0.5%; F1's maximum moves to race frame 481,
which must remain in the regression set. The 50 FPS requirement is not met.

`make verify-status-cache` compares complete surfaces and exact dirty-list
sequences/coverage against forced-cold repaint: 24 isolated row damage
cases, 3380 successful randomized transitions and 716 matching faults.
It covers participation, options, weapons, signed/wrapped widths,
division faults and dirty-pixel overflow. An initial harness failure came
from discarding pending damage after an intentionally failed draw; normal
gameplay exits on that error. The recovery stress test now retains pending
damage across failed draws rather than silently dropping it.

`make verify-dos-hud` passes, including 768 original full-screen HUD
transitions. All three display audits pass 600 updates. RETCHECK now also
compares the actual platform-side status call with forced-cold repaint,
reusing its existing snapshot allocations, outside normal builds. All four
tracks pass 603 retention checks and 700 status comparisons with zero
mismatches and canonical final states. Geometry-cache checks also pass
(575 F1, 603 CITY; cache disabled for the smaller BASIC/WHACKO actor sets).
Evidence: `tmp/hud-rows-{cache-oracle,dos,audit,retcheck}-20260927*.log`.

Final normal-build repeat after the diagnostic-only integration:
`tmp/hud-rows-final-20260928-{0,1,2,3}.log`, all 603 updates and canonical
final states. Work/worst: BASIC 158617/370, F1 183724/448, CITY 153068/358,
WHACKO 162130/421. CITY's maximum is now frame 700 (already a regression
probe). These final numbers supersede the initial candidate for acceptance;
the small timing variation does not change the conclusion. The normal
build is restored and the muted benchmark sessions exited successfully.

## Post-geometry profiles and next spike target (2026-09-27)

HEAD 3bd216f, fresh sequential captures:
`tmp/pcprof-geometry-{f1,whacko}-20260927.{bin,log,elf}`. Both cover 603
active-window updates starting at race frame 98, with canonical final states
and zero missed samples (10356 F1, 8849 WHACKO). Matching ELFs are archived.
The sampler raises measured total work to 205073 and 181904 lines versus
uninstrumented 184429 and 162979 (about 11%); these are diagnostic profiles,
not replacement acceptance timings.

Approximate sampled lines per update (25.4 lines/sample, not precise stage
timers; wait samples excluded from these named work stages):

| Stage | F1 | WHACKO |
| --- | ---: | ---: |
| Remaining race-step C | 40.5 | 37.5 |
| Native car integration | 20.1 | 21.7 |
| Rectangle C2P | 19.4 | 20.1 |
| Wheel emission | 18.2 | 19.8 |
| Sprite retention preparation | 14.0 | below top 22 |
| Point drawing | 12.8 | 14.3 |
| Generic actor advancement | 12.7 | 7.8 |
| Point advancement | 11.2 | 12.0 |
| Status bars | 9.4 | 10.2 |
| Draw-priority C wrapper | 9.0 | 6.7 |

F1 retention preparation is down from the pre-cache profile's 24.4 sampled
lines to 14.0. Point-count correlations remain roughly 88/100 added lines
per 100 particles on F1/WHACKO; draw and advance contribute about 24-27
each, and WHACKO point restoration about 11. These are correlations, not
causal per-particle guarantees; retirement/compaction and emission matter.

Race-frame 613 (F1) and 685 (WHACKO) correspond to one-based measured
indices 516 and 588. Each exact update has only 24 samples including waits:
do not infer precise single-update percentages. Thirteen-update windows
510..522 and 582..594 have 200 and 222 non-wait samples. F1 remains spread
across simulation, points, sprites and HUD; WHACKO emphasizes point draw,
advance and emission. Status drawing has 11 and 13 samples in these windows,
versus means of 9.4/10.2 sampled lines over the full capture.

Source-level status samples identify the bounded 20-pixel row paint loop
(`slicks_race_draw_status`, lines 3423/3425 at this commit): 34/222 F1 and
48/243 WHACKO status samples. A change to any cached bar row, or a dirty
rectangle touching any bar, currently repaints all active cars' three rows.
An exact per-car/per-row repaint mask is the next measured candidate:
preserve all changed colours/ends, background/participation invalidation,
intersecting dirty rectangles and sparse HUD pixels, weapon icons and the
unbounded-width fallback. Compare against a forced-cold cache/original HUD
oracle and run target display/retention audits. No gain is claimed yet.

Draw-priority wrappers also spend substantial samples in prologue/epilogue
and native-call setup. A later candidate is avoiding empty-priority calls
or a smaller native-chain argument interface, preserving the legacy path
and sprite/overflow boundaries. Conversely, disassembly already gives
`finish_car_update` a short-displacement car base; do not assume adding
another address-register barrier there removes repeated long displacements.

## Producer-invalidated sprite geometry cache (2026-09-27)

The native retention pass can reuse its geometry entries when no producer
changed their source keys. It still resets per-update conflicts, counts down
settling, touches moved-entry rectangles, processes particle/car conflicts,
and checks frame/colour/occlusion before retaining pixels. No actor update
or rendering-order step is removed.

Writer inventory: race-start/pause invalidation; actor allocation and
configuration (including flag activation inside the car update); native
track-object placement; generic kind-3 actor movement, expiry/retirement,
and transitions across the frame-0..3 eligibility boundary. Navigation
count, permanent track handles and loaded assets are immutable between
starts. Point allocation uses only free slots; a formerly live track slot
must retire first, which invalidates geometry. Within-union animation,
colour and occlusion remain separately checked. Fractional movement may
conservatively invalidate even when the pixel key stays the same.

The reference advancement path conservatively dirties live kind-3 actors.
The shadow harness saves/restores the derived dirty byte separately so
reference invalidation cannot mask a missing native write. Exact dirty-byte
equality is not required: RETCHECK independently scans every cached source
key on cache hits, without shadow execution. The host diagnostic test
injects nine missing invalidations (X/Y, asset, priority, state, kind, frame
eligibility, handle and navigation count), all detected, and accepts four
non-geometric changes. Native advancement passes 8192 random/single-slot
pools plus fourteen isolated producer cases with independently required
invalidations, unchanged state and ABI. Isolated cases include each motion
axis/acceleration, expiry, frame eligibility in both directions, signed
position/frame wrap, and unchanged within-union animation; one actor cannot
mask another's missing write by setting the shared dirty flag first.
Existing group, restore-chain and draw-chain oracles also pass.

Initial outer-only benchmark against 7219259 (same control code as d4762ea):

| Track | Control work / worst | Candidate work / worst |
| --- | ---: | ---: |
| BASIC | 158981 / 368 | 159206 / 370 |
| F1 | 189303 / 469 | 184400 / 459 |
| CITY | 156635 / 365 | 153611 / 362 |
| WHACKO | 162635 / 424 | 162975 / 422 |

All final states match. F1 improves 2.6% total work, CITY 1.9%; other
tracks are effectively flat. The worst updates remain over budget.
Logs: `tmp/geometry-cache-20260927-*.log` and
`tmp/geometry-cache-{host,final-oracles}-20260927.log`.

All four tracks pass 603 RETCHECK updates with zero chunky-hash or particle
mismatches and canonical final states. F1 exercises 575 clean geometry
cache hits and CITY 603, with zero source-key mismatches; BASIC/WHACKO have
too few track actors to enable sprite retention. Evidence:
`tmp/geometry-retcheck-20260927-*.log`. Full-frame display audits pass
600 updates each on F1/CITY/WHACKO (32/18/5 actors, 2076/1480/1854 marks):
`tmp/geometry-audit-20260927-*.log`. Shadow sites 3 and 4 each pass 700 calls
per track on all four tracks, zero state mismatches/race errors and matching
final states (`tmp/geometry-shadow-20260927-*.log`); F1 exercises 27 moving
track-object probes. The final normal-build timing repeat
(`tmp/geometry-final-20260927-*.log`) reproduces the gain: work/worst
159205/369, 184429/459, 153622/361 and 162979/423, with canonical final
states. Accepted after these gates. All owned muted emulators are closed;
the normal executable is restored. All four means are now below 20 ms,
but worst updates still require 14-32% cuts; this is not attainment of 50 FPS.

## Whole-gameplay compiler footprint experiment (2026-09-27)

Against d4762ea's accepted `GAMEPLAY_CFLAGS=-O3` build, force-rebuild
`race_runtime.o` with `-O2`, then with `-Os`. No source or default flags
change. Each alternative runs all four tracks sequentially, muted, on the
same stock A1200 configuration. All eight runs complete 603 updates and
match the control's exact FINAL_STATE. Work/worst are raster lines:

| Track | O3 control | O2 | Os |
| --- | ---: | ---: | ---: |
| BASIC | 158981 / 368 | 157446 / 377 | 170271 / 425 |
| F1 | 189303 / 469 | 189044 / 503 | 203402 / 555 |
| CITY | 156635 / 365 | 155735 / 384 | 168629 / 426 |
| WHACKO | 162635 / 424 | 163002 / 460 | 177712 / 522 |

O2 changes total work by less than 1% but worsens all four maxima; Os
regresses total work by 7-9% and maxima by 15-23%. Reject both for the
worst-update goal. The `slicks_race_step` symbol shrinks from 24186 bytes
(O3) to 14828 (O2) and 11152 (Os), but this is only the function's own
extent, not all out-of-line callees. Smaller code alone is not a speed win.
These timings do not identify a unique cause among calls, spills, generated
arithmetic and instruction-fetch/layout changes.

Evidence: `tmp/compiler-{o2,os}-20260927-{0,1,2,3}.log`, corresponding
`tmp/compiler-{o2,os}-build-20260927.log`, and
`tmp/compiler-restore-o3-20260927.log`. Final-state equality is a screening
gate, not full rendering equivalence; rejected builds need no adoption
audits. All owned emulators closed before rebuilding the normal O3
executable. No experimental compiler option remains in the defaults.

## Stable-geometry retention decision (2026-09-27)

The geometry pass already compares each entry's handle, actor kind, asset,
priority and masked pixel-position key. If no rebuild happens, an eligible
kept entry cannot have moved: movement removes candidacy until a rebuild.
NEXT's unchanged-draw producer invariant also establishes that its previous
descriptor describes that same cached geometry. The final decision can
therefore omit repeated kind, X/Y and asset comparisons on this path.
Frame, colour and priority/occlusion comparisons remain unconditional.
All rebuild paths (including count changes and settling) explicitly set
the full-validation flag before invoking the rebuild helper. Invalidated
groups still restore in their original reverse order.

| Track | Parent work / worst | Candidate work / worst |
| --- | ---: | ---: |
| BASIC | 158967 / 369 | 158981 / 368 |
| F1 | 190634 / 468 | 189303 / 469 |
| CITY | 157609 / 370 | 156635 / 365 |
| WHACKO | 162708 / 421 | 162635 / 424 |

Parent e2c0258; logs `tmp/retention-stable-20260927-*.log`. Final states
match. Total-work reductions are 0.7% F1 and 0.6% CITY; BASIC/WHACKO
are essentially flat and worst-update changes are mixed. Existing group
and native producer-invariant tests pass
(`tmp/retention-stable-host-20260927.log`). Accepted after all four tracks
pass 603 RETCHECK updates with zero surface-hash/particle mismatches and
canonical final states (`tmp/stable-retcheck-20260927-*.log`). Full-frame
dirty audits also pass 600 updates each on F1/CITY/WHACKO, with 32/18/5
actors and 2076/1480/1854 marks (`tmp/stable-audit-20260927-*.log`). All
owned muted sessions close and the normal build is restored. This is not
a worst-frame breakthrough.

## Retained-sprite descriptor traffic reduction (2026-09-27)

The restore chain used to recopy all twelve descriptor bytes for every
retained sprite. NEXT can only come from a validated unchanged draw or a
validated kept draw. Both preserve every descriptor field except the kind
marker, which unchanged drawing clears for deferred dirty handling. The
candidate refreshes only that marker and the retention flag; it does not
write pixels, saved backgrounds or the other descriptor fields.

The restore-chain oracle now exercises retained sprites with that producer
invariant: 384 cases, 5287 restorations, 1409 retained sprites, full state,
exact allowed writes, fallback return and ABI. The draw-chain oracle checks
both producers (eligible unchanged and already kept), in addition to cold,
warm, stale-mask, changed-frame and fractional-position cases: 2688 cases,
28548 actual draws. Both pass. The old oracles had omitted the existing
retention-byte write from their allowed-write maps; fallback clearing and
draw-side retention updates are now explicitly checked, not silently ignored.
Log: `tmp/retained-metadata-oracle-20260927.log`.

| Track | Parent work / worst | Candidate work / worst |
| --- | ---: | ---: |
| BASIC | 158948 / 369 | 158967 / 369 |
| F1 | 193476 / 478 | 190634 / 468 |
| CITY | 159409 / 369 | 157609 / 370 |
| WHACKO | 162720 / 420 | 162708 / 421 |

Parent 38198d8; logs `tmp/retained-metadata-20260927-*.log`. All final
states match. Total work falls 1.5% on F1 and 1.1% on CITY; the other
tracks are effectively flat. Only F1's maximum improves meaningfully.
Accepted after all four target RETCHECK runs passed 603 updates each with
zero surface-hash and particle-record mismatches and canonical final states
(`tmp/metadata-retcheck-20260927-*.log`). Full-frame dirty-region audits
pass 600 updates each on F1/CITY/WHACKO, with 32/18/5 actors and
2076/1480/1854 marks (`tmp/metadata-audit-20260927-*.log`). Host group
and dirty-tracking suites also pass. All owned muted emulators closed;
the normal executable is restored after the diagnostic build.

## Exact per-car steering cache (2026-09-27)

The five staged signed-word steering divisions depend on four operands,
not on elapsed ticks. A per-driver cache keys the exact one-tick result by
input, steering scale, damage channel 3 and steering property; the final
signed-word tick multiplication still happens on every call. Changes to any
operand invalidate the cached result. No approximations or changes to the
order of steering, damage yaw, motion or car tails are made. The cache is
appended to the runtime so existing native field offsets remain unchanged.

| Track | Parent work / worst | Cached work / worst |
| --- | ---: | ---: |
| BASIC | 159509 / 370 | 158948 / 369 |
| F1 | 193977 / 473 | 193476 / 478 |
| CITY | 159977 / 368 | 159409 / 369 |
| WHACKO | 163236 / 423 | 162720 / 420 |

Logs: `tmp/steering-cache-20260927-*.log`, parent 2b3bcbc (c64e40c
changes documentation only). All final states match. The total-work gain
is only 0.26-0.36%; worst updates are mixed, so this is not a worst-frame
budget breakthrough. Verification includes 200000 signed-word hit/miss,
individual-key-change and tick-boundary cases, all driving-physics tests,
and eleven original-DOS trajectory comparisons of 7200 updates each.
Logs: `tmp/steering-cache-{host,trajectory}-20260927.log`.

Opt-in target shadow site 7 compares each cached scalar directly with the
uncached arithmetic and reports the first differing pair. It requires no
state snapshot and compiles out of normal builds. Four tracks and the jump
fixture pass 2412 comparisons each with zero mismatches and race errors:
`tmp/shadow-steering-cache-20260927-{0,1,2,3,SLICKS_JUMP_TRACK_1}.log`.
An incorrectly named `SLICKS_ICE_TEST` launch was stopped and is not evidence
of ice coverage; only the explicit corrected fixture runs count.
The corrected ice and high-zone fixtures pass 412 comparisons each, zero
mismatches/race errors (`tmp/shadow-steering-cache-special-20260927-*.log`).
The normal build is restored afterward and all owned emulators close.

## Fresh post-group CPU profiles (2026-09-27)

`tmp/pcprof-postgroups-{f1,whacko}-20260927.{bin,log,elf}` captures the
accepted pre-steering-cache build, with active-window indexing, 603 updates,
zero missed samples and canonical final states. F1 has 11235 samples,
WHACKO 8841. These are instrumented profiles, not acceptance timings.
In F1 the sprite draw chain now averages 9.7 sampled raster lines, down
from 26.8 in the earlier isolated-sprite profile, while retention preparation
is 24.4 lines. The inlined race step averages 40.4 lines; 70 of its 960
samples fall in steering arithmetic. WHACKO still correlates with roughly
101 sampled lines per 100 live points, including about 26 for point drawing
and 25 for advancement. Retention preparation and point traffic remain
better opportunities than C2P-only optimization.

Assembly-address attribution within F1's 579 preparation samples gives
240 final-decision, 189 geometry, 91 point-conflict, 29 reset, 26 guard and
4 car-call samples. Ranges in the matching archived ELF are respectively
5061a..506d4, 503ca..5053c, 5053c..50610, 503a4..503ca,
50320..503a4 and 50610..5061a (hex, exclusive end). The first two groups
represent about 10.1 and 8.0 instrumented raster lines per update. These
are pointers for the next experiment, not additional measured savings.

## Shared particle retirement row-table experiment (2026-09-27)

Replacing the one remaining shared-retirement `mulu.w #320` with the
existing row table passed all 1028 legacy and 800 shared DOS-backed native
particle batches, including exact permanent pixels, dirty saturation,
compaction, slot/index metadata and ABI preservation. It did not deliver
a useful performance improvement on the stock Chip-only target:

| Track | Control work / worst | Row-table work / worst |
| --- | ---: | ---: |
| BASIC | 159509 / 370 | 159553 / 371 |
| F1 | 193977 / 473 | 193787 / 478 |
| CITY | 159977 / 368 | 159967 / 372 |
| WHACKO | 163236 / 423 | 163466 / 423 |

All final states match. Logs: `tmp/particle-table-20260927-*.log`,
oracle `tmp/particle-table-oracle-20260927.log`; control 2b3bcbc. Total
changes are below 0.15%, with no worst-frame improvement. The candidate
was removed before further target audits. This cold-path exception does
not undo the existing row tables throughout the hot rendering paths.

Inspection also found that skipping shared-particle slot-state publication
is not immediately valid: allocation starts a slot at state 1, while a new
permanent point's particle record starts at state 5, and advancement brings
the slot into agreement. Any future invariant-based omission must first
account for construction in both the C and native emission paths and retain
the original retirement timing. The current native oracle deliberately
tests arbitrary incoming slot/index metadata, so silently dropping those
writes would violate its existing contract.

## Overlapping stationary sprite groups (2026-09-27)

The candidate extends isolated sprite retention to connected components of
overlapping track rectangles. Every member must earn retention for the next
update; invalidation of any kept member restores the whole component in
descending priority/handle order. Rebuilding geometry releases the old
components before replacing their lists. Clipped components remain excluded.
The selective-rebuild version is accepted after timing and display audits.

The first version released every isolated retained sprite on a rebuild too.
Although it passed equivalence, F1's worst update rose to 507 lines at race
frame 507. A correctly indexed profile puts rebuild and late restoration in
that update. The revised version restores old groups first, then restores
only formerly isolated members that join new groups. Their old saved
rectangles were disjoint, so those remaining restores are independent.
The host oracle also checks this transition and preservation of unrelated
isolated sprites. This removes the new maximum while retaining the average
gain. No simulation or drawing-order changes are made.

| Track | Control work / worst | Selective group work / worst |
| --- | ---: | ---: |
| BASIC | 159466 / 370 | 159509 / 370 |
| F1 | 208162 / 484 | 193977 / 473 |
| CITY | 162940 / 372 | 159977 / 368 |
| WHACKO | 163130 / 420 | 163236 / 423 |

Logs: `tmp/groups-selective-20260927-*.log`. All 603-update final states
match. F1 total work falls 6.8%, CITY 1.8%; BASIC/WHACKO total changes
are under 0.1%, not improvements, and WHACKO's maximum is three lines
higher. F1 still needs a substantial further cut to reach the 312-line
budget. Initial version logs/profile are `tmp/groups-20260927-*.log` and
`tmp/pcprof-groups-f1-20260927.{bin,log,elf}`.

The host group oracle passes 1024 priority/handle permutations, including
transitive overlap, independent reverse saved-background restoration,
all-or-none eligibility, rebuilding after motion, clipped components and
invalidation. The target RETCHECK build passes 603 updates on each of BASIC,
F1, CITY and WHACKO, with zero surface-hash or particle-record mismatches
and unchanged canonical final states. Logs:
`tmp/group-retcheck-20260927-{0,1,2,3}.log`. All four muted correctness
emulators closed before rebuilding the normal executable for timing.

The final selective version repeats all four RETCHECK runs successfully:
603 updates each, zero surface-hash and particle-record mismatches, canonical
final states (`tmp/group-selective-retcheck-20260927-*.log`). Full-frame
dirty-region audits pass 600 updates each on F1/CITY/WHACKO, statistics off,
with 32/18/5 actors and 2076/1480/1854 marks
(`tmp/group-audit-20260927-*.log`). Host dirty-tracking, track/weapon actors,
surface effects and original-DOS particle-expiry suites pass. All owned
emulators closed after their tests. No gameplay effects were removed.

## Rejected single-rectangle prune specialization (2026-09-27)

An exact single-rectangle path avoided testing the same bounds as both the
union and its sole member. It passed all 12000 native prune cases and the
host dirty-tracking suite, but did not provide useful end-to-end savings.

| Track | Control work / worst | Candidate work / worst |
| --- | ---: | ---: |
| BASIC | 159466 / 370 | 159187 / 369 |
| F1 | 208162 / 484 | 208026 / 487 |
| CITY | 162940 / 372 | 162941 / 373 |
| WHACKO | 163130 / 420 | 163099 / 422 |

All final states match. The largest total reduction is 0.18% on BASIC;
F1 and WHACKO maxima worsen. The candidate was removed without further
rendering audits. Logs: `tmp/prune-single-20260927-*.log`, control ad38849.
The isolated draft remains local-only at `tmp/dirty_prune_single.s`.

## Corrected CPU-sample frame attribution (2026-09-27)

The old IRQ handler tagged samples with `completed_updates - 1`, although
the completed-update counter advances only after the current work. Most
work samples were therefore tagged one update behind, and post-work
benchmark aggregation was also sampled. This does **not** change any
uninterrupted WORK_SUM or maximum-work measurement, but earlier narrow-frame
profiles and particle correlations are approximate. Whole-window historical
shares also include benchmark bookkeeping; do not mistake its removal
from sampling for a game speedup.

The diagnostic now publishes an explicit active-window index at update
entry and $ffff after the work timer stops. The handler accepts indices
0..703, including the first and last measured updates, and ignores inactive
intervals. Logs identify the new convention with
`PC_FRAME_INDEX=active-window first=98`. The analysis tools warn for older
captures; the summary's default includes all 603 measured updates. Its
`--frames` filter is one-based within that window, not a race-frame number:
race frame 613 is index 516 in this interface.

`verify-pc-sampler` passes 1536 cases: exact first/last/boundary tags,
inactive exclusion, valid/invalid exception frames, buffer capacity, timer
jitter writes, untouched bytes and register/stack preservation. Full target
captures cover every index 0..602 with no missed samples and canonical
final states: BASIC 8435, F1 12542, CITY 8382, WHACKO 8847 samples.
Files: `tmp/pcprof-indexed-{0,f1,2,3}-20260927.{log,bin,elf}`. All runs
use normal presentation statistics off and close their muted emulators.

Fresh F1 has 9113 non-wait samples. C2P is 450 (4.9%), the inlined race
step 967 (10.6%), sprite draw chain 636 (7.0%), car integration 562 (6.2%),
retention preparation 514 (5.6%), wheel emission 418 (4.6%), sprite restore
chain/helper 517 (5.7%), point draw 316 (3.5%), and shared point advance
266 (2.9%). Within the inlined step, motion preparation has 140 samples,
steering 75, the per-car tail 67, update-loop glue 55 and layer update 55.
No single remaining routine explains the required cut.

Point-count regression gives approximately 88/90/99/100 additional sampled
raster lines per 100 live points on BASIC/F1/CITY/WHACKO. These are
instrumented correlations, not causal per-point costs or normal timings;
they cannot explain retirement/compaction and HUD transition costs alone.
The sampler adds roughly 11% work and is never used for the acceptance
timing. Seven-update windows around BASIC 219, F1 551/613 and WHACKO 685
contain only about 110-128 non-wait samples each: useful for locating groups
of work, not precise single-frame percentages. CITY's end window is shorter
and its update 700 also includes the final diagnostic snapshot.

The uninterrupted post-correction control is
`tmp/indexed-control-20260927-*.log`: work/worst 159466/370,
208162/484, 162940/372, 163130/420, with canonical final states. Total
work is essentially unchanged from 6569d64 (within 0.05%), but F1's maximum
moves from 475 to 484 and back to update 613. This is not a speedup claim;
use the fresh control for subsequent candidates and keep both F1 transitions
in the regression set. Small peak changes do not establish a robust budget
margin. No owned emulator remains after these runs.

## Consecutive point restoration (2026-09-27)

The ordered restoration loop now has a native candidate for consecutive
point handles, stopping at a sprite handle so the existing mixed ordering
is preserved. Short base pointers and a packed old-X/Y load reduce address
calculation and executed code. Only saved pixels are written; saved-valid
flags make the same transition to 2. No dirty coverage or retention rule
changes. The RETCHECK reference deliberately retains the scalar C path.

The first six-argument interface saved little total work: BASIC 159694/371,
F1 208331/476, CITY 163229/373, WHACKO 163437/423 (work/worst). Passing
only the race pointer and first handle removes repeated argument pushes;
the routine derives its short-offset array bases itself.

| Track | Parent work / 603 | Two-argument work / 603 | Worst: parent -> candidate |
| --- | ---: | ---: | --- |
| BASIC | 160230 | 159419 | 375 -> 371 |
| F1 | 208376 | 208059 | 486 -> 475 |
| CITY | 162914 | 162949 | 372 -> 373 |
| WHACKO | 163792 | 163109 | 428 -> 422 |

Parent 2cc044d; logs `tmp/point-restore-20260927-*.log` and
`tmp/point-restore-abi-20260927-*.log`. All final states match. CITY is
essentially flat, not an improvement. New maximum-frame indices are
219/551/700/685; the earlier 613/506 transitions remain regression probes.

`verify-point-restore` checks 4096 chains, 335076 point visits, 170497
restores and 1359 sprite boundaries against an independent scalar oracle.
It compares the full 128 KiB working image, returned handle, saved registers
and stack; cases include repeated pixels, flags 0..3, screen edges and
invalid old coordinates on unsaved points. Deliberately broken flags,
stride and saved colour were all rejected in both the isolated draft and
the final two-argument version.
Host dirty-tracking/order and original-DOS surface-effect tests pass.
Full-frame display audits pass 600 updates each on F1/CITY/WHACKO with
statistics off, 32/18/5 actors and 2076/1480/1854 marks:
`tmp/audit-point-restore-20260927-{1,2,3}.log`. All four tracks pass 603
RETCHECK updates with zero surface-hash or particle-record mismatches and
canonical final states: `tmp/point-restore-retcheck-20260927-{0,1,2,3}.log`.
The host track/weapon actor and original-DOS expiry suites also pass.
All owned muted emulators closed; the normal build was restored. The
two-argument candidate is accepted for its lower busy-frame cost, without
claiming a meaningful CITY or overall F1 average improvement.

## Car force-vector experiments (2026-09-27)

Caching per-call force coefficients and direction vectors on the stack saved
only 0.12-0.19% across the four tracks. Its work/worst results were
160559/375, 209252/487, 163484/375 and 164153/430 against parent 0dbe6e0's
160841/375, 209500/489, 163789/374 and 164378/431. Stack traffic consumed
most of the saved calculation at roughly 1.82 physics quanta per update.
The candidate was removed; its local archive is
`tmp/motion-invariants-20260927.patch` and logs are
`tmp/motion-invariants-20260927-*.log`.

A smaller candidate pre-scales the sixteen signed direction vectors by 200.
Two long multiplies disappear from each normal physics quantum. The vectors
fit signed words and multiplication remains identical modulo 2^32:
`(direction * speed) * 200 == (direction * 200) * speed`. Division, special
states, rounding and simulation order are unchanged.

| Track | Parent work / 603 | Vector work / 603 | Worst: parent -> vector |
| --- | ---: | ---: | --- |
| BASIC | 160841 | 160230 | 375 -> 375 |
| F1 | 209500 | 208376 | 489 -> 486 |
| CITY | 163789 | 162914 | 374 -> 372 |
| WHACKO | 164378 | 163792 | 431 -> 428 |

All final states match. Logs: `tmp/motion-vectors-20260927-*.log`.
The independent native integration test covers 12000 no-wall cases,
all sixteen directions, normal/coasting/special states, signed timestep
boundaries, both layers, coordinate clamps, complete car bytes and ABI.
Its randomized force range deliberately avoids host-long-width overflow;
the modular identity above covers the arithmetic transformation separately.
The existing 24000-case car-ray oracle covers blocked probes. Both tests
pass for the baseline, rejected stack cache and pre-scaled vectors. The
long original-DOS trajectory comparisons also pass. Target motion shadows
pass 2412 comparisons each on BASIC, F1, CITY, WHACKO and jumps, plus
412 each on ice and zones, with zero state mismatches or race errors.
Logs: `tmp/shadow-vectors-20260927-*.log`. All muted sessions closed and
the normal build was restored. The smaller vector change is accepted.

## Native draw-chain construction (2026-09-27)

Inspection of GCC's inlined loop found per-handle sprite-pointer maintenance,
long-displacement indexed accesses and handle reconstruction from a moving
pointer. The native builder holds the handle directly, keeps short base
pointers for the arrays and calculates a sprite address only for sprites.
It clears the head array with an aligned unrolled body and retains the C
reverse-construction path. Descending handle scans and head insertion preserve
the original ascending-handle order within each priority.

| Track | Parent work / 603 | Native work / 603 | Worst: parent -> native |
| --- | ---: | ---: | --- |
| BASIC | 162362 | 160841 | 381 -> 375 |
| F1 | 210343 | 209500 | 491 -> 489 |
| CITY | 164623 | 163789 | 377 -> 374 |
| WHACKO | 165823 | 164378 | 435 -> 431 |

The 0.4-0.9% total-work gain is small but consistent across all four tracks;
all final states match. Control: 066b949, `tmp/c2p16-table-20260927-*.log`.
Candidate: `tmp/actor-order-20260927-*.log`.

`verify-actor-order` compares the complete 64 KiB working image against the
independent C builder for 12000 empty/mixed/full pools and 413394 linked
handles, including all priorities, signed state bytes, duplicate particle
indices, stable chains, untouched bytes and ABI preservation. Native offset,
particle-stride and alignment assumptions are checked at build time.
Host dirty/order regression tests also pass.

Full-frame display audits pass 600 updates each on F1/CITY/WHACKO with
statistics off (32/18/5 actors, 2076/1480/1854 marks):
`tmp/audit-order-20260927-{1,2,3}.log`. RETCHECK now explicitly selects
the independent C ordering path for its no-retention reference, rather
than using the native builder in both updates. All four tracks pass 603
checks with zero surface-hash and particle-record mismatches and canonical
final states: `tmp/order-retcheck-20260927-{0,1,2,3}.log`. All runs were
muted and closed. The normal build was restored afterward.

## Worst-update state/region inspections after C2P16

Read-only pre/post snapshots at 066b949, not timing runs:
`tmp/inspect-worst-20260927-{0,1,2,3}.log`. The pre-snapshot is at entry to
the update; the post-snapshot is immediately before clearing the converted
dirty list. All four inspections reached their requested frame and closed
their muted emulators. No debugger writes to game state were used.

| Track / update | Pre records | Pre retired records | Post records | Post active | C2P rectangle area |
| --- | ---: | ---: | ---: | ---: | ---: |
| BASIC 219 | 172 | 31 | 155 | 155 | 544 |
| F1 613 | 106 | 19 | 89 | 89 | 1136 |
| CITY 506 | 14 | 0 | 16 | 13 | 1856 |
| WHACKO 685 | 160 | 19 | 158 | 158 | 736 |

The count change includes new emissions before advancement, not just
retirement. First released record indices are 34, 7, none and 14:
the remaining records must compact after these holes. CITY retires three
points this update and restores a HUD cell.

Actual half-open rectangles:

- BASIC: (176,124)-(208,141).
- F1: (160,69)-(176,76), (80,95)-(96,103), (128,145)-(144,152),
  (96,136)-(160,143), (80,144)-(96,151), (224,28)-(256,35).
- CITY: (48,13)-(96,25), (64,27)-(80,33), (256,186)-(320,200),
  (48,34)-(80,43).
- WHACKO: (144,109)-(176,122), (160,131)-(176,139),
  (144,58)-(176,64).

A conservative offline retention screen finds 121/74/10/131 points whose
saved pixel survives lifetime and signed-word motion unchanged. Accounting
for changing lower-priority points, permanent baking, and old/new
car/sprite rectangles leaves 56/6/6/63 candidates in this screen.
Rounding those conflicts into the handoff's suggested 8x8 cells leaves
only 19/4/4/34. This is design screening, **not an implemented or verified
retention algorithm**: cars are conservatively considered below every point,
and every saved sprite rectangle is considered a possible write, even if
that sprite is retained. New/compacted points are matched by actor handle.
The result warns against assuming that most motionless points can be kept;
grid maintenance still needs a measured cost below the saved redraw work.

## Single-pass 16-pixel C2P (2026-09-27)

The racing rectangle path now uses a 16-pixel adaptation of Kalms' public-
domain c5 butterfly transpose. A nibble exchange preserves both halves of
each input byte, instead of reading the rectangle once for each four-plane
pass. Four registers produce eight final plane words. The inner conversion
loop fits inside the 68020's 256-byte instruction cache. Only final values
reach display memory; there is no intermediate plane write or lookahead.
The rectangle wrapper reuses its existing mult320 row lookup. Full-screen
startup/menu conversion retains the original Kalms routine.

Dirty bounds now round to 16 rather than 32 pixels, with unchanged clipping,
half-open union/merge rules and full-list fallback. This is a combined
converter/area improvement; the benchmark does not separately attribute
the savings to narrower bounds versus the single-pass conversion.

| Track | Parent work / 603 | New work / 603 | Worst: parent -> new |
| --- | ---: | ---: | --- |
| BASIC | 172347 | 162362 | 396 -> 381 |
| F1 | 220839 | 210343 | 550 -> 491 |
| CITY | 174168 | 164623 | 403 -> 377 |
| WHACKO | 178284 | 165823 | 444 -> 435 |

Total work falls 4.8-7.0%. All four canonical final states match. The final
benchmark is `tmp/c2p16-table-20260927-{0,1,2,3}.log`; parent game behavior
is unchanged from 7be80df through 49c7525. The earlier candidate using one
row multiply per rectangle also improved all tracks
(`tmp/c2p16-final-20260927-*.log`), but is not the committed variant.
An initial run named `c2p16-20260927` accidentally selected only the new
object as the default make target and ran the old executable; it was
interrupted, the make rule placement corrected, and its measurements are
excluded.

Verification: `verify-c2p16` tests all 128 input basis bits, 1536 randomized
rectangles and 32 empty rectangles under Unicorn/68020. It compares the
entire destination against an independent per-pixel conversion, checks
every plane store is a final word, checks source bounds/no writes, canaries,
callee-saved registers and stack. `verify-dirty-rect` passes 240000 native/C
calls; dirty tracking, pruning, native particle drawing, surface effects,
track/weapon actors and original DOS particle expiry suites pass.

The final build also passes full-frame display audits for 600 updates each
on F1, CITY and WHACKO with live statistics explicitly off: 32/18/5 actors
and 2076/1480/1854 marks. Logs: `tmp/audit-c2p16-20260927-{1,2,3}.log`.
No simulation or draw order changed. Audit and timing emulators were muted
and closed, and the executable was never rebuilt while they used it.

## Incremental car-ray experiment (rejected)

The candidate replaced per-step signed division and multiplication by 320
with a positive quotient/remainder walk and an incrementally maintained map
offset. It used a proven no-wrap domain (both deltas at most 181 and a visible
origin); larger rays and the lower map's unusual wrapped origins retained
the original loop. Eight small loops specialized orientation, layer and
minor direction. Coordinates, map reads, errors and last-clear outputs were
identical, but the complete candidate was slower in these real races.
Guard/setup cost and code layout are plausible explanations, not separately
measured attributions. The candidate is **not** retained.

| Track | Parent work/max | Incremental candidate work/max |
| --- | ---: | ---: |
| BASIC | 172347 / 396 | 172614 / 396 |
| F1 | 220839 / 550 | 221132 / 551 |
| CITY | 174168 / 403 | 174340 / 404 |
| WHACKO | 178284 / 444 | 178694 / 443 |

Parent: 7be80df, `tmp/hud-copy-20260927-T.log`; candidate:
`tmp/incremental-rays-20260927-T.log`. All four canonical final states match,
but all total-work sums regress (0.10–0.23%). No shadow/display runs were
performed for the rejected candidate. The normal ray source and binary are
restored rather than leaving an unhelpful fast path installed.

`verify-car-probe` remains as a regression test: it enters the actual native
ray loop and compares its exit, every map-read count and last-clear outputs
against the original signed-word-product/truncating-division equation.
24000 cases cover both layers, all directions, materials, boundary levels,
off-screen/wrapped lower-map origins, and explicit 181/182 product-overflow
edges. Both versions pass: 10525 collisions, 5440 bounds errors, 8035 clear
rays, including 2279 large rays. The post-hit aliasing code is unchanged.

## F1 retention eligibility inspection

The read-only checkpoint inspection (`tmp/inspect-retention-20260927.log`,
7be80df, not a timing run) finds 21 candidates among 32 track sprites. The
hidden finish flag is off-screen. Ten other objects are rejected due to
overlap: nine tyre handles (24–26 and 28–33), plus banner handle 36.
These sit around x229–246, y4–35. The
remaining objects can be retained. Overlap-group retention would need exact
group invalidation and reverse-order late restoration; simply removing the
isolation test would corrupt saved backgrounds.

## Fresh F1 profile after stationary-object caching

`tmp/pcprof-cache-f1-20260927.{bin,log,elf}` profiles the restored normal
build at 2390b77, before the HUD-copy experiment. All 603 updates complete,
with the canonical final state, 13715 samples and zero missed samples.
Waits account for 3816 samples; of the remaining 9899, C2P takes 910 (9.2%),
the inlined race step 901 (9.1%), sprite draw chain 661 (6.7%), car integration
573 (5.8%), retention preparation 504 (5.1%), sprite restore chain plus its
pixel helper 525 (5.3%), and track-object motion 180 (1.8%). Sprite advancement
is 268 (2.7%). Stationary-object setup is no longer the dominant track cost;
drawing/restoration, validation and retention overhead remain significant.
These are sampled shares, not uninterrupted frame timings.

Within the 901 race-step samples, the largest inlined pieces are motion
preparation (133), per-car tail (66), steering (65), update-loop glue (57),
checkpoint advancement (47), throttle (43), profiling gates (39), and actor
layer update (33). No single tail function explains most of the cost.

## HUD transition restore experiments

The old byte-at-a-time C loop restores 14 visible rows of each changed HUD
cell: 52 pixels for drivers 0..2, 50 for driver 3. A candidate compared
four-byte groups against the original saved HUD background, copied only
differences, and bounded dirty conversion to those differences plus text.
It passed 12000 native restore/bounds cases and host dirty/original-DOS HUD
checks, including foreign pixels outside text runs, but was rejected:

| Track | Control work/max | Bounded candidate work/max | Plain native copy work/max |
| --- | ---: | ---: | ---: |
| BASIC | 172409 / 413 | 173027 / 400 | 172347 / 396 |
| F1 | 220834 / 550 | 221784 / 550 | 220839 / 550 |
| CITY | 174249 / 420 | 174973 / 411 | 174168 / 403 |
| WHACKO | 178390 / 445 | 179079 / 465 | 178284 / 444 |

Logs are `tmp/{stationary-final,hud-restore,hud-copy}-20260927-T.log`.
All final states match. The bounded candidate increases total work on every
track and worsens WHACKO's maximum by 20 lines. The simpler native copy
retains the original full dirty rectangle and pixel order, using unrolled
longword copies and one word tail for the clipped cell. BASIC update 409
and CITY 506 each save 17 lines (~1.1 ms); total work is essentially unchanged
(-0.036%, +0.002%, -0.046%, -0.059%). This is a transition improvement, not a
claim that typical or particle-heavy frames are fixed.

The plain copy passes `verify-hud-restore` (12000 complete-buffer native
comparisons, four alignments, both widths, canaries and ABI),
`verify-dirty-tracking`, `verify-dos-hud` (including 768 composed original
full-screen HUD transitions), and `verify-arcade-hud` (2100 original commands).
Full-frame display audits pass 600 updates each on F1/CITY/WHACKO with
statistics explicitly off, 32/18/5 actors and 2076/1480/1854 permanent marks
(`tmp/audit-hudcopy-20260927-{1,2,3}.log`). Drawing order and retention are
unchanged. All three owned audit emulators close on success.

## Debugger sampling is phase-locked

FS-UAE's `barto_gdbserver` only services a GDB interrupt at vsync. Reading
`VPOSR`/`VHPOSR` in every SIGINT sample showed beam line 0 or 312 each time.
Because the race loop waits for display blank before C2P, SIGINT sampling
(the Rescue on Fractalus `diag_sample.sh` method) over-weighted C2P about
2x and attributed no samples at all to car physics. It is invalid here and the
script was not kept.

## CIA-B PC sampler

`SLICKS_BENCHMARK_DETAIL=8` selects `NATURALS<track>`: the outer-only
benchmark plus `src/platform/amiga/pc_sampler.s`. The diagnostic allocates a
CIA-B timer through `ciab.resource` (touching only a timer whose vector was
free), enables EXTER for the measured race, and reloads the running latch
with a xorshift-jittered 900..1411 E-clock period (~614 Hz) so samples cannot
lock to the beam. The handler finds the level-6 exception frame (format
`$0078`, plausible SR, even PC) on the supervisor stack and stores PC,
measured-frame index, frame offset and SR high byte. Only measured updates
1..602 are kept; a 16384-sample buffer is dumped by `diag_pc_sample.gdb` at
the frame-700 checkpoint together with per-update work/particle samples.

    cd amiga; . ./env.sh; ./pc_profile.sh TRACK LABEL
    tools/prof_summary.py tmp/pcprof-LABEL.bin [--inlined FUNC] [--lines FUNC]
        [--particles LO:HI]
    tools/particle_slope.py tmp/pcprof-LABEL.bin   # mean/intercept/slope per function

Whole-update timings (no sampler) come from `amiga/bench_tracks.sh LABEL`,
which builds, refuses stale executables and prints WORK_SUM, maximum work,
cadence and FINAL_STATE per track; compare with a control of the parent
commit. A behaviour-preserving change must leave FINAL_STATE identical.

`pc_profile.sh` archives the matching ELF as `tmp/pcprof-LABEL.elf`; old
sample files cannot be resolved against a rebuilt layout. The sampler adds
about 12% uniform work (F1 265255 -> 297309 lines) and finds every frame
(`missed=0`). Final car positions and marks are unchanged, so it is valid for
shares, not absolute timings. Wait-loop samples are reported, not hidden.

## Where the work goes (baseline 5fa9458 + sampler)

Shares of non-wait samples, all measured updates (`tmp/pcprof-{t0,f1,t2,t3}`):

| Function | BASIC | F1 | CITY | WHACKO |
| --- | ---: | ---: | ---: | ---: |
| `prepare_car_motion` | 11.8 | 9.7 | 11.3 | 12.5 |
| `slicks_race_step` (inlined tails, emission, particle maps) | 13.0 | 8.0 | 9.8 | 11.5 |
| C2P | 9.5 | 7.9 | 8.3 | 11.4 |
| track sprite draw chain | 0.9 | 12.7 | 7.7 | 2.0 |
| `draw_car` C setup | 5.2 | 3.4 | 4.4 | 5.3 |
| track-object motion | 0.2 | 8.0 | 8.2 | 0.1 |
| HUD status + rectangles + timers | 6.5 | 5.4 | 5.9 | 7.0 |
| particle draw/restore/advance/emission | ~16 | ~10 | ~9 | ~13 |

BASIC updates with at least 120 particles (`--particles 120:400`) spend about
40% of work in particle-related code: native point drawing 9.1%, native
advance 7.1%, `add_trail_component` 5.9%, C point restoration 4.7%, draw-order
construction 4%, plus wheel emission and dirty-list handling. F1's 32 track
objects are 9 animated 8x7 banners, 20 static 5x5 tyres, two 4x4 markers and
one hidden flag; all were stationary through frame 700. Their total drawn
area is about 1036 pixels, yet motion, advancement, restoration and drawing
cost about 30% of F1 work: per-object overhead, not pixels.

## Cost model: instruction fetch dominates

Line-level profiles of compiled C are flat: the cost is spread over whole
functions executed once per car or per frame. `draw_car`'s setup (about 2.9 KB
of spill-heavy code with 32-bit displacements into the 245 KB race structure)
costs about 3900 cycles per call for four calls per update, more than its
pixel work. With all code and data in Chip RAM, each longword of executed
code outside the 256-byte instruction cache costs a Chip-RAM access, like a
data access. Executed code volume, not arithmetic, is the main cost.

## Shadow equivalence checks for native replacements

`make SHADOW=1` (driven by `amiga/shadow_check.sh`) defines
`SLICKS_SHADOW_CHECK`. At each wrapped call the verified C reference runs on
the live state; hashes of every kilobyte of the leading 32 KB working state
(cars, counters, RNG) record its result; the snapshot is restored and the
native routine runs; hashes are compared. A mismatch captures the first
differing kilobyte from both versions and continues from the reference.
`diag_shadow_check.gdb` reports calls, mismatches, differing bytes, final
positions and track-collision counts. Runs use warp; they are correctness,
not timing, runs. A static assertion ties the window to the wrapped write set.

## Native car motion core

`src/game/car_motion.s` replaces `integrate_car_motion` (force, velocity,
scaled positions, the probe ray of `move_car_through_track`, contact latch
and track clamp). Offsets come from `src/game/race_offsets.c`, compiled by
the 68020 compiler into `amiga/obj/race_offsets.i`. Rare blocked rays call
the C `resolve_track_velocity`. When a special state or disabled sampling
makes every sample zero, the ray is skipped after the original count and
wrap checks. The C reference remains for host oracles.

Shadow checks: 2412 calls on each benchmark track, 412 on the jump, ice,
zone and road fixtures, zero mismatches, unchanged final states; BASIC's run
exercised 25 wall hits (`tmp/shadow-motion*-*.log`). Host oracles pass,
including all eleven 7200-update DOS driving scenarios
(`tmp/verify-native-motion{,-driving}.log`).

Same-layout control (reference forced with `-DSLICKS_REFERENCE_MOTION`)
versus native, work lines over 603 updates (`tmp/refmotion-*`, `tmp/motion-*`):

| Track | Reference | Native | Change |
| --- | ---: | ---: | ---: |
| BASIC | 191318 | 190122 | -0.63% |
| F1 | 265722 | 264723 | -0.38% |
| CITY | 213347 | 212029 | -0.62% |
| WHACKO | 197170 | 195285 | -0.96% |

Maxima moved 453/585/475/509 to 451/589/471/515 lines. The gain is small
because the translation still executes about 700 bytes of largely
straight-line code per physics quantum; its samples are spread evenly over
the routine, like the C it replaced. A literal translation is not enough:
denser code with register-resident state and hoisted per-call invariants is
required. Final checkpoint control for later comparisons (`tmp/ckpt1-*`):
189881 / 264529 / 211897 / 195053 work lines.

## Status-bar and timer change detection

Without weapons, `slicks_race_draw_status` now computes each active car's
three cached row ends directly and returns when they, the background and the
active mask match the cache and no dirty rectangle touches rows 187..189.
Errors, unbounded widths, weapon HUDs and any difference take the unchanged
general path. The old per-frame scan of every queued dirty pixel is replaced
by `dirty_pixel_hud`, set by C `mark_dirty_pixel` for rows at or below
`SLICKS_POINT_HEIGHT` (native point producers cannot reach them) and cleared
with the lists; it occupies existing structure padding. `draw_timers` hoists
its fixed options byte and uses register car/cache bases.

Host HUD, Arcade HUD and dirty-tracking suites pass, including the injected
strip-pixel invalidation case (`tmp/verify-status-fast.log`). Full display
audits pass on F1 (600 updates, 32 actors, 2076 marks), CITY (18 actors, 1480
marks) and WHACKO (5 actors, 1854 marks) (`tmp/audit-hudfast-{1,2,3}.log`);
BASIC completes before update 600 in that fixture. The fuel fixture's 3
status-pixel checks and the damage fixture's 4 have zero failures.

| Track | Control (`ckpt1`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 189881 | 183574 | -3.3% | 452 -> 441 |
| F1 | 264529 | 257582 | -2.6% | 588 -> 586 |
| CITY | 211897 | 205131 | -3.2% | 469 -> 471 |
| WHACKO | 195053 | 189130 | -3.0% | 514 -> 487 |

Final positions and marks are unchanged (`tmp/hudfast-*`).

The fuel (`diag_fuel.gdb`) and damage (`diag_damage_race.gdb`) race fixtures
fail on both this build and a rebuilt 5fa9458 with byte-identical output:
a finished car's lap counter reaches 6, and the fuel fixture counts no
finished cars. The status-pixel parts of both pass.

## Native wheel emission

`src/game/car_emission.s` replaces `emit_wheel_surface` for shared-pool
races, including `emit_offroad_wheel`, `add_trail_component`, the wheel sound
request and the Borland RNG. Particle records are written with one long store
for saved/permanent/occlusion/state, so each new particle receives its final
occlusion limit at creation instead of in a trailing pass. Allocation keeps
the emission cursor semantics (and calls the native lowest-slot allocator if
the cursor is zero); out-of-range wheel samples call the shared C sampler.
In heavy BASIC frames the C prologue alone (seven arguments copied to stack
slots) was 21% of `add_trail_component`.

The shadow window grew to 56 KB to cover particles, slot tables and actor
kind/saved bytes. Emission shadow checks: 2412 calls on each benchmark track
and 412 on the ice, zone and road fixtures (2412 on the jump fixture), zero
mismatches and unchanged final states (`tmp/shadow-emit-*.log`). Host surface,
track-actor and driving oracles pass (`tmp/verify-native-emission.log`).

| Track | Control (`hudfast`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 183574 | 180871 | -1.5% | 441 -> 425 |
| F1 | 257582 | 255157 | -0.9% | 586 -> 570 |
| CITY | 205131 | 203757 | -0.7% | 471 -> 468 |
| WHACKO | 189130 | 186772 | -1.2% | 487 -> 482 |

## Native track-object motion and actor advancement

`src/game/track_motion.s` replaces `update_track_actor_motion` and the
shared-pool `advance_weapon_actors`. The motion loop keeps the common
stationary path (kind test, next-position material sample, layer update,
actor configuration and contact-rectangle test) within the instruction
cache; car-pixel setup, off-map samples, moving-object rays and car contacts
are out of line. Moving objects call `slicks_track_actor_probe`, a C wrapper
around the unchanged `slicks_moving_probe`. The actor loop skips point slots
with one word test and applies `slicks_actor_advance` exactly.

C callers can tail-call these entries (`bra.l`), which produced an
`R_68K_PC32` cross-section relocation that `elf2hunk` rejects. As with
`memory.s`, the native motion, emission and track modules now use the
compiler's `.text` section; the Amiga Makefile sets `.DELETE_ON_ERROR` so a
failed conversion cannot leave an empty executable that looks current.

Shadow checks: 700 motion and 700 advancement calls on each benchmark track
(countdown included), zero mismatches, unchanged final states; F1 exercised
27 moving-object rays after a car contact (`tmp/shadow-track-*.log`,
`tmp/shadow-advance-*.log`). Host track-actor, weapon-actor and dirty
suites pass (`tmp/verify-native-track-motion.log`).

| Track | Control (`emit`) | Candidate | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 180871 | 181030 | +0.1% | 425 -> 426 |
| F1 | 255157 | 246131 | -3.5% | 570 -> 556 |
| CITY | 203757 | 196391 | -3.6% | 468 -> 462 |
| WHACKO | 186772 | 186763 | 0.0% | 482 -> 482 |

BASIC and WHACKO have one and five track objects respectively.

## Native dirty rectangles and prepared car drawing

`src/game/dirty_rect.s` replaces `mark_dirty_rect` (clip, widen to
32-pixel columns, fold overlapping or touching rectangles, full-list
fallback). `make verify-dirty-rect` runs the real 68020 code in Unicorn
against the C body over 240000 randomized calls (19904 merges, 587
full-list fallbacks), comparing every list entry, the count, the stack and
callee-saved registers. Offsets come from the target compiler
(`build/offsets/race_offsets.i`).

`src/game/car_render.s` replaces the prepared-cache part of `draw_car`.
All eligibility tests (bounds, cache identity, frame size, tile occlusion
maxima, masked-style limit) precede side effects; an ineligible car
returns to the unchanged C renderer. Rendering shadow checks also snapshot
and compare the chunky surface: 2804 car draws on each of BASIC and F1,
zero mismatches (`tmp/shadow-render-{0,1}.log`). The F1 display audit
passes 600 updates, 32 actors and 2076 marks (`tmp/audit-cardraw-1.log`);
host dirty, HUD and car-draw suites pass (`tmp/verify-native-car-render.log`).

| Track | Control (`tmotion`) | Candidate | Change | p99 ms | Max lines |
| --- | ---: | ---: | ---: | --- | --- |
| BASIC | 181030 | 176112 | -2.7% | | 426 -> 419 |
| F1 | 246131 | 242290 | -1.6% | 34.23 -> 33.85 | 556 -> 564 |
| CITY | 196391 | 192284 | -2.1% | | 462 -> 454 |
| WHACKO | 186763 | 181708 | -2.7% | 29.62 -> 29.17 | 482 -> 496 |

All F1/WHACKO percentiles through p99 improve; single maxima rose slightly.

## Upper bound for unchanged-sprite retention

A throwaway build that skipped every track sprite in the native restore and
draw chains (visually wrong) reduced F1 work from 246131 to 187782 lines
(-23.7%, maximum 556 -> 458) and CITY from 196391 to 170985 (-12.9%,
462 -> 416) (`tmp/noskip-ub-{1,2}.log`). This exceeds the sampled chain
shares because it also removes chain switching and conversions. Exact
retention of unchanged, untouched sprites is therefore the main F1 lever.

## Unchanged track-sprite retention

`src/game/sprite_retention.inc` (policy, C helpers, host reference) and
`src/game/sprite_retention.s` (per-update pass) keep unchanged, isolated
track sprites in the authoritative chunky surface instead of restoring and
redrawing them. Restoring then redrawing such a sprite leaves identical
pixels and an identical saved background when no actor drawn before it
touches its rectangle; actors drawn after it already saved its pixels.
The actor's former padding byte holds `retain` bits. The native restore
chain keeps `RETAIN_NEXT` sprites and records their pre-simulation
description; after simulation the pass detects geometry changes from raw
motion keys (union rectangles over animation frames), scans drawable
points against candidate rows/cells (only lower-priority points: track
actors hold permanent handles below every point), marks car rectangles,
then keeps or late-restores each sprite. The draw chain skips kept sprites
and grants `RETAIN_NEXT` only after an unchanged native draw. Moving
objects become conflict sources until motionless for 16 updates, then the
maps are rebuilt. Shadows, weapon sprites, Arcade mode, the countdown,
pause handoffs and tracks with fewer than eight track objects disable it.

A late restore must keep the pre-simulation description recorded at the
keep. Using `restore_weapon_actor` there replaced it with post-simulation
state, so an animation-frame change drew without a dirty rectangle; the
display audit caught this (F1 and WHACKO, update 103) and the fix restores
pixels without touching the description.

Verification: `make RETCHECK=1` runs every racing update first without
retention from a snapshot of the race state and chunky surface, then for
real, comparing the surfaces (`diag_retention_check.gdb`). All updates
match on BASIC, F1, CITY and WHACKO (603 each), the jump fixture (603) and
the ice, zone and road fixtures (103 each) (`tmp/retcheck*.log`). Display
audits pass on F1 (600 updates, 32 actors, 2076 marks), CITY and WHACKO
(`tmp/audit-retain*.log`); host track/weapon/dirty suites run the C
reference pass and pass (`tmp/verify-retention-host.log`). On F1 about 20
of 31 sprites are kept per update.

| Track | Control (`cardraw`) | Candidate (`retain9`) | Change | p95 ms | Max lines |
| --- | ---: | ---: | ---: | --- | --- |
| BASIC | 176112 | 176565 | +0.3% | 23.72 -> | 419 -> 421 |
| F1 | 242290 | 232294 | -4.1% | 31.60 -> 30.64 | 564 -> 569 |
| CITY | 192284 | 183690 | -4.5% | 24.42 -> | 454 -> 441 |
| WHACKO | 181708 | 182435 | +0.4% | 26.54 -> 26.73 | 496 -> 497 |

Earlier C-only and naive native passes cost more than they saved (C pass
about 14% of F1 samples; frame-keyed rebuilds and alternate-frame rebuilds
for slowly sliding objects each regressed F1 worst frames by up to 120
lines). The retained design is limited by the per-update pass in
point-heavy frames, where conflicts also reduce the number kept.

## Particle cost per live point

Regressing per-frame sample counts against the live particle count (retain9
profiles, `tools/particle_slope.py`) gives about 1.1 raster lines per live
point on BASIC and WHACKO: draw 29, advance 22, emission 16, the C loops
around the advance 12, restore 11-15 and actor ordering 11 lines per 100
points. The zero-point intercept is about 270 lines, so particle-heavy
frames (130-170 points at the race start and in skids) are the BASIC,
CITY and WHACKO worst cases. F1 is slow without points: about 150 lines
per update of track-sprite work.

Calibration from GCC's restore loop (24 instructions, 9 Chip data
accesses, about 103 cycles per point) puts a Chip data access near 7 cycles
and a cached instruction near 2.5. The compiled restore loop is already
as lean as a hand-written one, so it stays in C.

## Native shared-pool particle advance

`slicks_advance_shared_particles` (`src/game/particle_runtime.s`) replaces
the C handle/index/state loops that surrounded the shared-pool advance. One
pass releases points retired on the previous pass (slot state 0, index -1),
moves or retires the rest exactly as before, compacts records and handles,
and publishes each survivor's trail index and slot state. Each record is
read with one `movem` and written once; records past the new count are left
untouched (the in-place reference also advanced those dead records, so the
shadow site clears that tail in both runs before comparing).

Verification: `make verify-particle-advance` adds 800 shared-pool batches
checked against the DOS-derived motion/lifetime results plus handle
compaction, trail indices, slot states and untouched dead records;
injected mutations are caught. Shadow site 6 (render variant, chunky
compared for permanent-mark baking) has no mismatches over 603 updates on
all four tracks (`tmp/shadow-adv1-*.log`, `tmp/shadow-adv2-2.log`), with
final positions and marks equal to the control. Host particle, dirty,
surface, track- and weapon-actor suites pass (`tmp/host-adv-*.log`).

| Track | Control (`retain9`) | Candidate (`adv1`) | Change | Max lines |
| --- | ---: | ---: | ---: | --- |
| BASIC | 176565 | 173064 | -2.0% | 421 -> 414 |
| F1 | 232294 | 230657 | -0.7% | 569 -> 567 |
| CITY | 183690 | 182464 | -0.7% | 441 -> 436 |
| WHACKO | 182435 | 179804 | -1.4% | 497 -> 475 |

Frames with 130 or more points gain 10-14 lines (about 7-8 lines per 100
points, below the 130-cycle estimate); frames under 50 points gain 0-2.

## Rejected tighter particle draw chain (2026-09-27)

Applied the handoff's local `tmp/chain_draft.s` without changing the single
or ordered-batch entries. It packs metadata/old coordinates/dirty writes,
keeps chain tables in registers and moves material/surface loads to the
occluded path. The estimated saving did not survive four-track timing.
Fresh outer-only controls use eb4d4f4; logs are
`tmp/resume-{control,chain}-20260927-{0,1,2,3}.log`.

| Track | Control work / 603 | Draft work / 603 | Control worst | Draft worst |
| --- | ---: | ---: | ---: | ---: |
| BASIC | 172965 | 174390 | 418 | 414 |
| F1 | 230702 | 230012 | 567 | 569 |
| CITY | 182465 | 183011 | 435 | 435 |
| WHACKO | 179791 | 180290 | 475 | 484 |

All four final states match exactly. The native draw oracle passed 2048
single, 256 ordered-batch and 256 actor-chain cases; host surface-effect,
dirty-tracking, track/weapon-actor and DOS particle-expiry suites passed.
Three tracks regress in total work, and the small F1 total saving does not
improve its worst frame. Reverted the source experiment; display audits
were therefore not run for this rejected draft. Fewer nominal accesses on
one path are not sufficient evidence of a real-workload improvement.

## Native sparse-pixel pruning (2026-09-27)

First measured removing the prune call entirely. Both sparse and rectangle
C2P read the final chunky image, so this is safe, but duplicated pixel
conversions still cost enough to regress BASIC. Kept pruning instead:
`src/game/dirty_prune.s` computes the union bounds once in registers and
rejects outside pixels before visiting individual rectangles. It preserves
the C result exactly, including ordering, unused bytes and the untouched
tail of the list. No actor ordering, retention or timing boundary changes.

| Track | Control work / 603 | No prune | Native prune | Worst: control -> native |
| --- | ---: | ---: | ---: | --- |
| BASIC | 172965 | 173392 | 172561 | 418 -> 414 |
| F1 | 230702 | 229474 | 228718 | 567 -> 563 |
| CITY | 182465 | 181772 | 181515 | 435 -> 432 |
| WHACKO | 179791 | 179386 | 178931 | 475 -> 468 |

Logs: `tmp/resume-{control,noprune,nativeprune}-20260927-{0,1,2,3}.log`.
All final car positions and mark counts match. Native pruning improves
total work by 0.23%, 0.86%, 0.52% and 0.48% respectively. Mean work times
are 18.34, 24.31, 19.30 and 19.02 ms; maxima are 26.54, 36.09, 27.69 and
30.00 ms. These small gains do not meet the 20 ms worst-update target.

`make verify-dirty-prune` runs 12000 native lists against the unchanged C
reference, checking half-open edges, empty/full lists, duplicates, unsigned
outliers, stable compaction, all surrounding bytes and the preserved ABI.
`verify-dirty-tracking` and `verify-planar-writes` also pass (3467 planar
writer cases plus the original DOS-derived particle lifecycle checks).
Full-frame display audits pass for 600 updates each on F1 (32 actors,
2076 marks), CITY (18 actors, 1480 marks) and WHACKO (5 actors, 1854 marks):
`tmp/audit-nativeprune-20260927-{1,2,3}.log`. All owned emulator sessions
closed on exit; debug audio was muted without disabling emulated audio.

## Palette reload cost ceiling (2026-09-27)

Temporarily replaced the palette block's first three MOVEs with COP2LC and
COPJMP2 targeting the final WAIT. This deliberately incorrect-colour build
isolates the cost of all 528 palette MOVEs; it is not a playable candidate.
Restored the production source afterwards. Relative to f9dcca8:

| Track | Normal work / 603 | Skip palette | Normal worst | Skip worst |
| --- | ---: | ---: | ---: | ---: |
| BASIC | 172561 | 171437 | 414 | 414 |
| F1 | 228718 | 227824 | 563 | 561 |
| CITY | 181515 | 181160 | 432 | 427 |
| WHACKO | 178931 | 177580 | 468 | 459 |

All final positions and mark counts match. Logs:
`tmp/palette-skip-20260927-{0,1,2,3}.log`, against
`tmp/resume-nativeprune-20260927-{0,1,2,3}.log`. Total savings are only
0.65%, 0.39%, 0.20% and 0.76%, before adding any palette-gate bookkeeping
or necessary reloads. This falls below the handoff's roughly 1% threshold;
do not introduce frame-sensitive palette gating for this small upper bound.
The existing full-palette path remains unchanged.

## Opt-in presentation statistics (2026-09-27)

User approved making expensive diagnostic bookkeeping opt-in. Plain play
and outer-only benchmarks now skip presentation snapshots and rectangle
area/equivalent-row accumulation; `NATIVE`, detailed profiling and ordinary
diagnostic fixtures retain them. `LIVE_STATS` in benchmark/profile logs
explicitly marks whether dirty-area/sparse/audio snapshot fields are valid.
The optional collectors are out-of-line; the rectangle conversion loop has
no per-rectangle statistics branch. All real rendering and audio updates
remain unconditional. Timer boundaries stay in place. Audio blank-spill
monitoring remains enabled in benchmarks, conservatively including its
cost; plain play need not read raster time solely for that diagnostic.

| Track | Control work / 603 | Opt-in outlined | Worst: control -> outlined |
| --- | ---: | ---: | --- |
| BASIC | 172561 | 172240 | 414 -> 416 |
| F1 | 228718 | 229191 | 563 -> 565 |
| CITY | 181515 | 181644 | 432 -> 431 |
| WHACKO | 178931 | 178154 | 468 -> 442 |

Logs: `tmp/optin-outlined-20260927-{0,1,2,3}.log`, against
`tmp/resume-nativeprune-20260927-{0,1,2,3}.log`. All final states match.
Total work across tracks changes by only -0.07%; this does NOT establish a
general 13-line saving. BASIC/F1 worst frames slightly regress while
WHACKO's improves. The earlier inline-gated experiment (`tmp/optin-stats-*`)
was worse on three tracks and is not retained.

Per-update samples also qualify the handoff's particle-heavy-maxima claim:
BASIC update 409 has zero live particles, CITY 506 has 16, F1 482 has 63,
WHACKO 687 has 159. BASIC's C2P portion alone is 116 lines (also 116 in the
control, with 4352 rectangle pixels). Investigate retirement/redraw
transitions; a live-count regression model is not a worst-case bound.

Verification: F1/CITY/WHACKO each passed 600 full-frame display audits
(`tmp/audit-optin-20260927-{1,2,3}.log`), but later inspection found that the
debugger override did not establish statistics-off mode. Those runs verify
pixels, not the claimed statistics-off coverage. The stationary-cache work
adds an explicit `NATURALO<track>Q` launch mode (`SLICKS_LIVE_STATS=0`) and
checks the flag before the first update and at the final audit. With statistics
enabled, `diag_live_stats.gdb` checks all 600 updates against the actual
converted rectangle/sparse lists (`tmp/live-stats-check2-20260927.log`). It
inspects just before the list is cleared, not at the later race-progress
checkpoint. Detailed statistics remain accurate, including per-rectangle
integer rounding. All four benchmark final states match the control.

## Inert shared-actor advancement (2026-09-27)

The native advance loop processes lifetime/retirement first, then skips its
remaining writes only when age, frame, period, both velocities and both
accelerations are all zero. In this state position, animation and age are
unchanged even when the frame count is zero. This is a direct state test,
not an ownership cache; initial randomized animation periods still take
the ordinary path. Two existing short branches grew to word branches.

| Track | Parent work / 603 | Candidate work / 603 | Worst: parent -> candidate |
| --- | ---: | ---: | --- |
| BASIC | 172240 | 172131 | 416 -> 414 |
| F1 | 229191 | 227283 | 565 -> 563 |
| CITY | 181644 | 180000 | 431 -> 436 |
| WHACKO | 178154 | 178179 | 442 -> 442 |

Parent is c1909a2 (`tmp/optin-outlined-20260927-*`); candidate logs are
`tmp/inert-advance-20260927-*`. All final states match. F1/CITY total work
improves 0.83%/0.91%; BASIC/WHACKO barely change, and CITY's worst update
regresses five lines. This is not a solution to the worst-update budget.

`make verify-actor-advance` compares 8192 complete 200-slot pools with the
reference, covering inert/near-inert states, signed wrapping, expiry on all
page values 0..3, animation, reserved/point slots, surrounding bytes and
callee-saved registers. `verify-actor-slots` also passes its original DOS
motion and allocator comparisons. Host track-object, weapon-object and
dirty-tracking suites pass (`tmp/inert-advance-host-20260927.log`).

Actor-only shadow checks pass 700 calls on each of BASIC, F1, CITY, WHACKO
and the jump fixture, and 200 each on ice and zones, with zero mismatches
and race errors. F1 also exercises 27 moving-object probes. All four main
track final states match their benchmark controls. Logs:
`tmp/shadow-inert-final-20260927-*.log`. The runner exits successfully,
restores the normal build and closes its emulators.

The diagnostic-only shadow build now has a compile-time site mask
(`SHADOW_SITES`, default 0xfe/all sites). Both ordinary and render wrappers
honor it before snapshotting; normal builds contain no mask or extra branch.
`SHADOW_SITES=16 ./shadow_check.sh LABEL ...` checks only actor advancement.
The report prints the mask and per-site call counts and fails on mismatches,
race errors or an unexercised selected site. An initial debugger-controlled
selector did not take effect; constant-data reporting then returned code
bytes, and the first BSS-report assignment was mistakenly in shutdown.
The final version publishes the mask to BSS in race setup. Earlier aborted
or reporting-invalid runs are not counted as verification.

## BASIC worst-update rectangle inspection (2026-09-27)

At update 409 the actual pre-clear dirty list is `(64,186)-(160,200)`,
`(160,83)-(256,112)`, `(0,2)-(32,9)`: 1344 + 2784 + 224 = 4352 pixels.
The final live particle count is zero. This confirms HUD/car/flag rectangle
conversion, not merely a stale area statistic. The first attempt to inspect
at an earlier target frame failed because startup overwrites that setting;
the successful inspection stops at `slicks_race_clear_dirty_rows` and reads
the list itself. Log: `tmp/regions2-20260927-0.log`. Its debugger-interrupted
timings are not benchmark results. CITY and the other worst transitions
still need equivalent inspection.

## Rejected four-byte emission slot scan (2026-09-27)

Tested the handoff's zero-byte detection formula on four aligned state bytes,
using scalar selection within a word containing a free byte. The initial
first-free-byte path stayed unchanged; occupied prefixes and short tails
used scalar reads, and no longword crossed the high-water boundary. It was
semantically correct but slower in the measured workloads.

| Track | Parent work / 603 | Candidate work / 603 | Worst: parent -> candidate |
| --- | ---: | ---: | --- |
| BASIC | 172409 | 172688 | 413 -> 413 |
| F1 | 220834 | 221466 | 550 -> 551 |
| CITY | 174249 | 174701 | 420 -> 422 |
| WHACKO | 178390 | 178868 | 445 -> 466 |

Parent is e888664 (`tmp/stationary-final-20260927-*`); candidate logs are
`tmp/emission-quad-20260927-*`. All final states match. Total work regresses
on every track, so the candidate was reverted without further in-game
shadow or display checks. The normal scalar production scan remains.

Kept `make verify-emission-scan`: it enters the actual production scan and
stops before allocation, comparing 100000 pools against scalar first-free
selection. It verifies signed state bytes, high-water tails, odd cursors,
word/longword alignment, no out-of-range reads, unchanged pool bytes,
preserved argument registers and balanced stack. Both candidate and restored
scalar code pass (33607 first-free and 66393 end exits).
