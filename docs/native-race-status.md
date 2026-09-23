# Native race status

The Amiga race path is native 68020 code. It does not execute a translated
x86 CPU context and does not use captured DOS frames. Track graphics, cars,
fonts, start lights, palettes, sound samples, and music are decoded from the
original files at run time; surface trails are generated point actors just as
they are by the original race routine.

## Implemented race services

- All ten 34-byte `.omi` records and all forty directional car images load
  from `SLICKS.000`. The default DOS lineup is vehicles 5, 2, 0, and 0.
  Unproved bytes 4–6 remain explicitly numbered rather than carrying the old
  speculative top-speed/response/steering names; the live speed limit and
  steering recurrence use their separately traced per-driver values. Byte 32
  is now identified as the divisor in `1000:ed64`'s four-channel impact/damage
  update rather than the former speculative AI-speed field; byte 33 remains
  explicitly unlabelled.
- The start grid is derived from each track's recorded position and heading.
- Four cars use persistent fixed-point position, velocity, speed, and heading
  state. Throttle uses the traced `0xa0` increment. The normal tyre-force
  block uses the recovered signed 32-bit numerator, four-factor divisor,
  branch-specific Q15 velocity decay, and `velocity / 20` position step.
  Coasting first applies its recovered per-driver Q15 multiplier to the drive
  scalar. Ordinary braking applies the recovered Q15 factor 8 to both velocity
  components and clears that scalar before the active-force update. The seven
  force coefficients are produced by the original quarter-step interpolation
  table from the default 13-value driver setup rather than hard-coded outputs.
  Steering now preserves the original four-stage signed integer recurrence,
  including the traced human/AI input strengths and per-driver scales; the
  semantic trace proves 9,329 literal heading transitions.
- Saved-under restoration retains the full 0--319 car X coordinate.  This is
  covered by the strict BASIC A1200 checksum: the two cars initially beyond
  x=255 no longer restore their old pixels at the wrapped x=7 position.
- The normal computer-control path follows the original ten-byte navigation
  records. Its 16-sector vector quantizer, steering thresholds, aligned-
  velocity throttle restoration, ordinary coast, greater-than-five-sector
  braking, centre checkpoint comparisons, vehicle-specific opponent
  look-ahead distances, opponent-triggered brake suppression, 700-tick
  initial grace, 150-tick stationary watch, 40-tick accelerating
  escape turn, randomized turn side, and 100-tick post-escape watch are
  recovered from `e204` and `f09d`. The
  `analyze_ai_controls.py` replay reconstructs each pre-turn heading and
  matches the recovered normal drive decision on 98.81% of 12,287 traced AI
  samples; the remaining trace rows include the special destination states
  still listed below.
- Car contacts use the recovered `.omi` extent and weight ratios. For each
  updated car, `2000:2d27..31bd` projects a point ten fixed units along its
  velocity using `(abs(vx) + abs(vy)) / 2`, tests that point against the other
  car's centre using the current car's byte-2 extent times 50, and applies the
  original integer-percent component transfer. It does not separate positions.
  Each car has the original persistent contact latch, which suppresses another
  impulse until that car completes a scan with no overlap. A forced DOS overlap
  gives current velocity `(-885,172)` and other velocity `(-863,889)` for
  weights 18 and 20; `make verify-car-collision` fixes that oracle independently
  of the full race. The same oracle covers the per-car secondary impact values
  81 and 65 recovered from `2000:30b0..317f`; the maximum is exposed as the
  current frame's collision-effect strength. Track
  contacts walk every integer centre pixel from the old to proposed position,
  snap to the last clear pixel at the original 100-unit scale, apply the
  four-neighbour `c63e` velocity transform, and then use the original fixed
  centre clamps. Class 2 takes the original non-contact dispatch. A complete
  scan of all 195 supplied tracks proves that none persists animated classes
  22–26 in its static material map; those classes must be introduced by the
  still-unrecovered runtime actor path rather than by a missing track fixture.
- The immutable mode-zero `b089` material map is reconstructed independently
  of the visible track pixels. Its four classes match the captured DOS
  `BASIC.SS` map at all 60,800 pixels; the strict A1200 gate fixes its checksum
  at `80f1987a`. A second independently layered mode-one map now also matches
  all 60,800 captured pixels for both `BASIC.SS` (`aa8bc219`) and
  `BASICTRK.SS` (`e472d3a7`). It retains the packed upper surface bits which
  cannot be recovered from the visible low-three-bit pixels alone.
- Wheel effects use the original per-car/per-heading wheel offsets and the
  recovered speed scalar `(abs(vx) + abs(vy)) / 2`. The values 61--72 in the
  live dispatch are palette indices, not archive sprite handles: the native
  path now creates the same one-pixel actors instead of incorrectly expanding
  every family into a 4x4 `savu` image. Classes 3/4 select colours 67--69;
  5/6/9/10/13/14 select 61--63; and 11/12 select 64--66 with their recovered
  30-through-49-tick first-actor lifetime. Above speed 200 the shared helper
  scatters one component around each wheel, and above 250 it adds a moving
  component with the original random 15-through-24-tick lifetime and
  -11-through-11 fixed velocity. Classes 7/8 use colour 55 above speed 300,
  lifetime 20, and -10-through-9 velocity. Sampling suppresses classes 2, 15,
  and 22 through 26. The active-particle count remains 16-bit at the 256-entry
  capacity, so filling the pool cannot make all smoke vanish.
- Each wheel on road-like material classes 0, 1, 17, 19, and 31 uses the DOS
  acceleration/brake gate. Acceleration emits below the driver's DS:4ee0
  threshold times ten; braking emits below twice that threshold. An emission
  creates moving colour 218 for 5--19 ticks plus a three-tick colour 70--72
  point. Above speed 100 it then selects sample block 2--4 with flag 2 and
  priority 10. The native calls preserve this observed random-number order.
- The live 32-entry material jump table is recovered and active. Classes 0,
  1, 17, 19, and 31 use the road-point path; 3 and 4 use colours 67--69;
  5, 6, 9, 10, 13, and 14 use 61--63; 7 and 8 use the separate moving point
  path; and 11 and 12 use 64--66 through the long-lived helper. Classes 2,
  15, 16, 18, and 20--30 have no actor dispatch.
- Four original-font HUD rows show race time and lap/finishing position.
  Checkpoint wrap records current, previous, and best lap times. A race ends
  when all four cars finish and draws an ordered results panel.
- The title menu selects the player car, every `.SS` file discovered in the
  `TRACKS` directory, and one through nine laps. The bundled run/debug setup
  exposes all 195 supplied tracks. Their navigation tables contain 7 through
  46 regions; the native decoder accepts and renders every one, with
  `make verify-native-tracks` providing an exhaustive host gate. Escape
  returns from a race and Return returns after results.
- Title-menu redraws retain the ordinary and selected colours produced by the
  recovered palette matcher. The C/assembly bridge no longer inherits
  undefined `d0`/`d1` values from its caller, so unrelated code layout cannot
  change menu colours.
- Paula channel 0 loops the engine sample selected by the original ten-entry
  vehicle-to-sample table (blocks 17, 17, 21, 22, 19, 18, 20, 18, 23, and
  24) at DOS priority 100. Its frequency uses the original per-vehicle base
  and slope tables applied to `(abs(vx) + abs(vy)) / 2`; the result is
  converted to a PAL Paula period. The full 26-block
  bank is indexed in one pass. Channels 1 and 2 play natural-length effects:
  a new car contact dispatches block 6, while track impacts select blocks
  10--16 from the current 16-way heading through the original DS:0196 table.
  Both use DOS flag-2 duplicate suppression and priority 14. Ordinary lap
  wraps use block 25 at priority 18, entering the final lap uses block 8 at
  priority 19, and the first finisher uses block 9 with flag 2 at priority 30.
  Paula reloads forever while DMA remains enabled, so each effect and the
  channel-3 `intermed.wav` results cue repoint its reload registers to a
  two-byte chip-RAM silent word after the initial body has been latched. The
  handoff is deferred for two audio updates: a fixed two-raster-line delay is
  shorter than the first DMA-request interval at the samples' period and can
  accidentally leave the sample itself armed as the reload body. Effects and
  the results cue therefore play once; only the engine intentionally loops.
  Chip allocations and DMA are released before AmigaOS is restored.
- Live painters merge changed scanlines into a fixed interval list. Kalms C2P
  converts only those intervals; unchanged rows are skipped. The live race
  has one authoritative 64,000-byte linear painting surface after setup; the
  four-bank VGA store is retained for translated setup code and synchronized
  only when a diagnostic logical checksum is requested. A direct four-bank
  source was tested on the strict 68020 target: pixel painting was within one
  millisecond of the linear path, but gathering banked rows before an
  unmodified Kalms transpose added roughly ten milliseconds at the busy
  700-frame checkpoint. A competitive single-buffer version must fuse the
  bank permutation into the pipelined C2P transpose.
- The strict FS-UAE target is a stock 2 MiB A1200 (`fast_memory=0`). The race
  runtime—including its embedded particle array—and both CPU-side surfaces
  use `MEMF_ANY`, so they necessarily reside in Chip RAM on this target; only
  display bitmaps, copper lists, and Paula sample bodies explicitly require
  `MEMF_CHIP`.
- The archive directory is read once per open. A BASIC session now reads about
  269 KiB in total instead of performing thousands of 19-byte directory reads.

## Evidence and remaining fidelity work

The strict 2 MiB A1200 gates cover title startup, 200 live race frames, a full
lap, the 25-region alternate track, the object-heavy `ICE.SS`, the maximum
46-region `HEIKKI30.SS`, all four finishers, the displayed results frame,
engine audio, results music, and clean system restoration. Host-side
differential tests cover the native graphics primitives and exhaustively
decode all 195 supplied tracks.

The implementation is playable and complete as a race loop, but these details
still require instruction-level recovery before calling the simulation
bit-exact:

- exposing non-default values for the recovered driver setup, the
  contact/reverse brake branches, the trigger for the special car-state path,
  and the remaining
  `.omi` property semantics;
- the special destination AI modes beyond the recovered normal path,
  opponent probe, and stationary recovery cadence;
- animated boundary classes 22–26 and rendering the recovered collision-effect
  strength through the original actor system;
- the remaining exact sound-event selection, priority, pitch, and duration
  cases beyond the recovered engine, lap, finish, collision, impact, and
  road-slip paths.

`tools/patches/dosbox-x-slicks-race-state.patch` provides the semantic DOS
trace used to compare controls, navigation state, positions, headings, and raw
54-byte per-car state without introducing an emulated CPU into the Amiga build.
Its paired integrator hooks also preserve the velocity entering and leaving
each update together with the seven drive coefficients and ten Q15 factors;
the force hooks now also preserve both signed division operands and results.
Across 27,030 traced X/Y updates, `tools/analyze_race_velocity.py` proves the
closed recurrence with no mismatches: the force numerator is the low 32 bits
of `direction * drive_scalar * 200`; its divisor is the low 32 bits of
`drive3 * drive0 * (state20 / 70 + 10) * (23 or 38)`; and the previous
velocity is decayed by branch Q15 factor 4 or 3 over `32768 + drive1` before
the force is added. The coast branch first applies Q15 factor 5 to the drive
scalar. Ordinary braking applies Q15 factor 8 once per elapsed simulation
quantum before clearing the drive scalar. The opt-in
`dosbox-x-slicks-special-state.patch` fixture additionally proves that positive
car-state word `+29h` bypasses the normal integrator and multiplies both
velocity components by the zero-extended Q15 factor 7. Its countdown/target
maintenance and the event that enters this state are mapped, but the entering
event still needs a natural DOS capture. The native runtime now uses the
recovered normal-state block, including two-quantum throttle and braking at
the native 50 Hz cadence; dynamic setup choices, contact/reverse braking, and
that special-state trigger remain to recover.
