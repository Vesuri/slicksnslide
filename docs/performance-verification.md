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
