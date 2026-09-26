# Whole-update distributions and selective profiling

## 2026-09-26 four-track baseline

Stock PAL A1200, 68020, 2 MiB Chip RAM, no Fast RAM. Game audio processing
enabled, host audio muted. The same native-menu scenario selects each track;
603 racing updates are measured after the countdown, ending at frame 700.
These short deterministic scenarios include substantial particle activity,
but are not exhaustive races, vehicle combinations or weapon-enabled loads.

The diagnostic keeps 704 bounded work-time/particle-count samples, written
after the work timer stops. GDB reads them only at the final checkpoint and
checks their sum against the existing aggregate. No mid-race debugger stops.
`sh tools/summarize_benchmark.sh tmp/distribution-{0,1,2,3}.log` reports
nearest-rank percentiles using the existing 312-line/20-ms convention.

| Track | Mean ms | p50 | p95 | p99 | Max | Over 20 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| BASIC | 20.297 | 20.192 | 25.705 | 27.756 | 29.038 | 318/603 |
| F1 | 28.072 | 27.115 | 34.423 | 36.667 | 37.372 | 603/603 |
| CITY | 22.554 | 22.051 | 27.051 | 28.846 | 30.385 | 555/603 |
| WHACKO | 20.912 | 20.321 | 27.628 | 29.936 | 32.628 | 312/603 |

Measured cadence sums over 602 intervals: 242778, 373783, 290176 and
251508 raster lines respectively. Work excludes synchronization waits;
cadence is a separate required metric. Neither metric establishes 50 FPS.

With 0..31 retained particles, mean work is BASIC 17.176 ms (185 samples),
F1 26.043 ms (377), CITY 21.457 ms (387), WHACKO 18.390 ms (292).
BASIC at 160..191 particles averages 27.139 ms (8 samples). Particle count
correlates with cost but does not isolate emission, collision or painting.
F1 has substantial overhead even with few particles.

Compared with the preceding no-sample build, BASIC mean changes from 20.28
to 20.30 ms and F1 remains 28.07 ms. Sampling is not a production optimization.
Its storage and post-timer bookkeeping can still change placement/beam phase;
subsequent comparisons must use same-build controls.

## Selective modes

`SLICKS_BENCHMARK_DETAIL` selects:

| Value | Instrumentation |
| --- | --- |
| 0 | Outer work/audio/C2P only |
| 1 | Historical full profiling, including continuous diagnostic snapshots |
| 2 | Main restore/update/advance/HUD/draw boundaries |
| 3 | Actor ordering and restoration/drawing layers |
| 4 | Motion preparation, weapon update and per-car tails |
| 5 | Wheel effects, collision/surface, smoke/contact and finish tails |
| 6 | Car drawing versus priority-3 point drawing |
| 7 | Track-object motion, point advancement and sprite advancement |

Modes 2..7 disable unrelated nested callbacks at the call sites. Like mode 0,
they omit continuous diagnostic snapshots. Each requires a matching mode-0
control from the same binary. Unselected stage fields are zero, not absent
game work. The opt-in per-general-sprite compile-time probe remains separate.

Early `selective-f1-*` runs were interrupted during instrumentation review;
use `isolated-f1-*` for the completed isolated-mode measurements instead.

### F1 isolated measurements

Same-build outer control: 265158 work lines across 603 updates (28.19 ms).
The instrumentation-enabled binary layout slightly differs from the earlier
28.07-ms baseline; that difference is not an optimization or regression claim.

| Probe | Mean work ms | Added vs control ms | Measured components, mean ms |
| --- | ---: | ---: | --- |
| Main stages | 28.55 | 0.37 | Restore **plus track motion** 6.21; car update 6.85; actor advance 1.81; HUD 0.38; drawing 8.27; C2P 2.88 |
| Actor layers | 28.78 | 0.59 | Restore index 0.22, high 2.43, cars/priority3 0.91, low 0.43; draw index 0.75, low 0.69, cars/priority3 2.26, high 4.67 |
| Simulation | 28.41 | 0.22 | Preparation 3.60; weapons 0.08; car tails 3.15 |
| Car tails | 29.35 | 1.17 | Wheel effects 1.62; collision/surface 1.53; smoke/contact 0.35; finish 0.35 |
| Car drawing | 28.72 | 0.53 | Cars 1.86; priority-3 points 0.42 |

The tail probe is relatively intrusive: do not add these independently
measured subcomponents to claim an uninstrumented frame budget. Different
callback sets change cache/beam phase. The former restore label overstated
background restoration by including track motion, boundary work and setup;
the dedicated motion probe separates those next.

Dedicated motion probe (`tmp/motion-f1-{0,7}.log`): track-object motion
21484 lines = 2.28 ms/update; point advancement 9248 = 0.98 ms; sprite
advancement 8432 = 0.90 ms. Total work 268397 versus same-build control
265248, a 0.33-ms probe cost. Both finish with 2296 marks and identical
four-car X/Y positions. Scope/checkpoint gates, dirty tracking, surface
effects and original track-actor suites pass (`tmp/verify-profile-scopes.log`,
`tmp/verify-selective-profile.log`).

The largest measured drawing group is priorities 4 and above. Its contents
include track sprites and points, not just static images. Avoiding necessary
saved-under operations or reducing effects is not an acceptable optimization.
