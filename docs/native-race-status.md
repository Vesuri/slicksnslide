# Native race status

The Amiga race path is native 68020 code. It does not execute a translated
x86 CPU context and does not use captured DOS frames. Track graphics, cars,
fonts, start lights, trails, palettes, sound samples, and music are decoded
from the original files at run time.

## Implemented race services

- All ten 34-byte `.omi` records and all forty directional car images load
  from `SLICKS.000`. The default DOS lineup is vehicles 5, 2, 0, and 0.
- The start grid is derived from each track's recorded position and heading.
- Four cars use persistent fixed-point position, velocity, speed, and heading
  state. Throttle uses the traced `0xa0` increment and `.omi` top-speed value.
  Coasting applies the recovered Q15 multiplier
  `0x7bdd - ((omi[23] - 100) * 2)` to the drive scalar.
  Steering now preserves the original four-stage signed integer recurrence,
  including the traced human/AI input strengths and per-driver scales; the
  semantic trace proves 9,329 literal heading transitions.
- The normal computer-control path follows the original ten-byte navigation
  records. Its steering thresholds, unconditional no-obstacle throttle, centre
  checkpoint comparisons, 150-tick stationary watch, and 40-tick recovery
  interval are recovered from `e204` and `f09d`.
- Car contacts use the recovered `.omi` extents and weight ratios. Track
  contacts preserve tangential velocity and reflect or stop the blocked axis.
- The immutable material map selects the `.omi` surface group and the original
  32-entry trail-velocity table. Grass, dirt, turning skids, and transient
  `savu` particles are produced by live movement.
- Four original-font HUD rows show race time and lap/finishing position.
  Checkpoint wrap records current, previous, and best lap times. A race ends
  when all four cars finish and draws an ordered results panel.
- The title menu selects player car, BASIC/BASICTRK, and one through nine laps.
  Escape returns from a race and Return returns after results.
- Paula channel 0 loops the selected car's engine sample with speed-dependent
  period. Channels 1 and 2 play contact and surface effects. Channel 3 plays
  `intermed.wav` after the race. Chip allocations and DMA are released before
  AmigaOS is restored.
- Live painters merge changed scanlines into a fixed interval list. Kalms C2P
  converts only those intervals; unchanged rows are skipped.
- The archive directory is read once per open. A BASIC session now reads about
  269 KiB in total instead of performing thousands of 19-byte directory reads.

## Evidence and remaining fidelity work

The strict 2 MiB A1200 gates cover title startup, 200 live race frames, a full
lap, the 25-region alternate track, all four finishers, the displayed results
frame, engine audio, results music, and clean system restoration. Host-side
differential tests cover the native graphics primitives.

The implementation is playable and complete as a race loop, but these details
still require instruction-level recovery before calling the simulation
bit-exact:

- the complete fixed-point tyre/velocity integrator and every use of the three
  values in each `.omi` surface group;
- the optional opponent-avoidance branches and special AI modes beyond the
  recovered normal path and stationary recovery cadence;
- the complete DOS boundary-contact resolver and secondary car-contact state;
- exact sound-event selection, priority, pitch, and duration rather than the
  current native event mapping.

`tools/patches/dosbox-x-slicks-race-state.patch` provides the semantic DOS
trace used to compare controls, navigation state, positions, headings, and raw
54-byte per-car state without introducing an emulated CPU into the Amiga build.
