# Stationary low-point retention prototype

This experiment was rejected on 2026-09-27: the narrow stationary-point
rule saved too few redraws to repay its scans. Production code is unchanged.
The broader moving-point retention proposal remains open.

## Current conflict-grid screening (2026-09-28)

Read-only pre/post captures on code 2ec6cae inspect BASIC 209, F1 481,
CITY 700 and WHACKO 685. `amiga/diag_point_retention_screen.gdb` requires
`$target`; it includes colours, occlusion keys, car priorities and shadows.
All four inspections completed and their emulators exited. Debugger-stopped
runs are not timing evidence. Reproduce the analysis with:

```
python3 tools/point_retention_screen.py --self-test tmp/point-screen-20260928-{0,1,2,3}.log
```

| Track/update | Unchanged candidates | Exact pixel | 2x2 | 4x4 | 8x8 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BASIC 209 | 109 | 58 | 44 | 28 | 15 |
| F1 481 | 40 | 13 | 9 | 9 | 5 |
| CITY 700 | 45 | 11 | 8 | 8 | 8 |
| WHACKO 685 | 131 | 71 | 55 | 33 | 16 |

The screen matches handles and includes old/new point footprints, car and
shadow rectangles, all sprite rectangles and permanent bakes. Equal-priority
conflicts are conservatively rejected. Stable overlapping candidates can
remain a group here; a runtime must preserve their saved-under ordering.
All sprites are treated as writers, even when retained. Conversely, future
geometry is supplied for free. These are design estimates, **not runtime
correctness proofs, universal upper bounds or measured speed gains**.

One-byte priority grids need 58880/14720/3680/920 bytes for 1/2/4/8-pixel
cells (encode priority+1, zero for baking, 255 for empty). In track order,
2x2 conflict insertion visits 244/861/489/340 cells; 4x4 visits
136/334/222/157. That excludes clearing, candidate scans, future-geometry
prediction and late invalidation. At the handoff's estimated 250-350 cycles
saved per retained point, retaining 28-33 points yields only roughly 8-13
raster lines gross. Dense clearing can consume much of that. A prototype
needs sparse clearing, generation tags or an exact sparse pixel index, plus
adaptive bypass on low-yield updates. Coarse 8x8 retention is unattractive
for these spikes; broader sampling and target measurement remain necessary.
Do not repeat the priority-zero-only experiment or assume particle retention
alone closes the 50 FPS gap. Captures and summary remain local-only.

## Scope and order proof

The first candidate handles only stationary priority-zero points, not moving
smoke. The original draws these points before the cars, shadows and higher
point/sprite priorities, and restores those higher layers before low points.
Thus higher layers' saved backgrounds already contain an unchanged low
point. Keeping that low point does not require inspecting every car rectangle.

A 256-entry, generation-tagged pixel hash is rebuilt from current visible low
points. It is not a framebuffer shadow or a row boolean array. The shared
actor pool has fewer than 256 live handles, so probing always finds an empty
entry. Duplicate pixels disqualify **all** involved points. Expiring points
participate in duplicate detection even though they cannot be kept: a bake
must never change the background under a retained point. Generation wrap
explicitly clears the table.

Preparation precedes the original restoration pass. Eligible points keep
their saved background and receive a private flag in `saved_valid`. Every
other actor restores normally. Car simulation and emission stay in their
original order. Before advancement/baking, newly emitted low points are
looked up in the hash; any older kept point at the same pixel is restored
then and rejoins the normal drawing path. The old indices remain valid
because compaction has not happened yet. All kept pixels were unique, so
these late restores cannot depend on each other's order.

Original lifetime/word-coordinate updates still execute for every point.
The kept flag survives compaction and is cleared by the drawing shortcut,
leaving the same particle record and saved background as restoration and
redrawing would have produced. A candidate with lifetime one is allowed
only on page zero, where the original defers expiry. Permanent marks still
bake on the original frame and stay in the authoritative chunky surface.

Material/occlusion maps and point colours do not change during the lifetime
of production low marks. Low points end above the HUD, so its restoration
cannot overwrite them. The prototype is disabled with weapons enabled,
without a prior complete draw chain, on non-shared/countdown paths, for a
priority-zero sprite, or for moving low/higher-priority permanent points.
Noncanonical word coordinates and inconsistent saved coordinates also
disable it. New moving low or higher-priority permanent points release all
kept points before advancement. No effects or simulation steps are omitted.

## Verification and measured rejection

The host lifecycle oracle compares full 320x200 surfaces and every particle
record with the unchanged restore/advance/draw route over 2400 cases. It
covers duplicates, emission conflicts, page-dependent expiry and baking,
occlusion, higher-priority moving points, fallback conditions and generation
wrap. The native drawing oracle now includes the kept flag in single,
ordered-batch and handle-chain cases, including dirty-list overflow.

The opt-in Amiga retention checker additionally compares active particle
records byte-for-byte after every reference/optimized update. Its existing
surface hash remains a separate check. It reports the number of points
actually kept, so a disabled optimization cannot masquerade as coverage.
Its debugger script fails on zero checked updates, surface/state mismatches
or race errors. These stronger checks are retained; the prototype-specific
kept-point counter is not. All extra bookkeeping is absent from normal builds.

BASIC, F1 and WHACKO each completed 603 in-game reference/optimized checks
with zero surface-hash or particle-record mismatches. They retained only
1450, 757 and 1311 points in total respectively (roughly one or two per
update). Logs: `tmp/point-retcheck-20260927-{0,1,3}.log`. CITY retention
checks and display audits were not run after the timing rejection.

| Track | Parent work / 603 | Prototype work / 603 | Worst: parent -> prototype |
| --- | ---: | ---: | --- |
| BASIC | 172347 | 179512 | 396 -> 428 |
| F1 | 220839 | 227236 | 550 -> 567 |
| CITY | 174168 | 178215 | 403 -> 412 |
| WHACKO | 178284 | 184321 | 444 -> 498 |

All four final states match the control. Total work regressed 2.3-4.2%.
Logs: `tmp/low-point-retain-20260927-{0,1,2,3}.log`; control is the HUD-copy
benchmark at 7be80df (unchanged game code through 5a07bc4). The prototype,
host oracle and application patch are archived locally under `tmp/`.
