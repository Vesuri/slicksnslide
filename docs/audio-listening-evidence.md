# Audio capture and listening evidence

## Controlled pitch probe and sample-bank correction

The opt-in `AUDIOPITCH` native diagnostic plays one vehicle-0 engine through
the production loader/adapter/VBI at measured speeds 0, 500 and 1,000 for
250 PAL vertical blanks each: requested sample rates 2,200, 5,700, 9,200 Hz.
`SLICKS_AUDIO_PITCH_TEST=1` selects it in the debug launcher.
`diag_audio_pitch_capture.gdb` observes restoration only, not each frame.
The corrected 2 MiB/no-Fast A1200 run returned restore=31, VBI spills=0,
and final requested frequency/Paula period 9,200/385.

The local DOSBox-X patch `dosbox-x-slicks-pitch-probe.patch` is an explicitly
modified-input listening fixture, enabled only by `SLICKS_PITCH_PROBE=1`.
It admits one original engine allocation, substitutes original engine ID 17,
and rejects other sample allocations through the original invalid-sample
path. It substitutes the same three frequencies at the original driver's
input, five emulated seconds apart; the original mixer and emulated DSP
perform playback. It does not synthesize a PC recording on the host.
The trace reports voice 1, mixer rate 15,000, and all three frequencies.

This investigation exposed the skipped RIFF/WAVE entry described in
`audio-channel-plan.md`. The live PC mixer sample has 2,050 bytes and its
waveform correlates 0.99964 with the correct unsigned-to-signed archive
source (amplitude differs because of original sample gain/quantization).
The old Amiga slot contained a different 2,700-byte source.

Delivered local-only clips after the ID fix, three seconds at each rate:

- `tmp/audio-pitch/pc-fixed-pitches.wav`: intervals 20..23, 25..28, 30..33
  from the second controlled DOS capture, `pc/capture/slicks_001.wav`.
- `tmp/audio-pitch/amiga-fixed-pitches.wav`: intervals 5.5..8.5, 10.5..13.5,
  15.5..18.5 from `amiga-correct-sample.raw` (44,100 Hz S16LE stereo).
- No normalization, pitch shifting or resampling was applied to these clips.
  The dominant spectral peaks are respectively about 33/87/140 Hz on both
  platforms. These are audible spectral components, not sample-clock rates.
- Loop autocorrelation (`tools/analyze_audio_pitch.py`) is strong on the PC
  recording (over 0.996). Amiga file-backend output fails the tool's 0.90
  confidence threshold, so its precise loop-rate estimates are **not** used
  as passing evidence. Matching spectrum peaks do not remove that limitation.
- User listening feedback on the corrected controlled pair: “They sound
  exactly the same.” This accepts the three tested pitches, not all race events.

## Extreme engine frequencies

The production adapter now prepares half- and quarter-length engine loops
at load time, with box filtering. Above the normal waveform's safe DMA rate
it selects a shorter waveform and scales the sample clock by the actual
loop lengths. Playback is still direct Paula DMA, without software mixing.
Bank changes use the existing blanking-safe restart and downward hysteresis.
Normal-rate playback retains the original waveform.

The conservative minimum period is 124; PAL hardware allows 123 and NTSC
124 according to the Commodore hardware reference. Host tests cover all
655,360 vehicle/speed combinations: every period stays in 124..65535,
all three banks are exercised, and worst representable loop-pitch error is
0.8052%. Wrapped requested rates below 55 Hz are outside Paula's range and
remain clamped, not claimed as pitch matches. Borrowed channels resume the
latest selected bank. All 44 allocation-failure positions clean up completely.

The muted `AUDIOHIGH` A1200/2 MiB/no-Fast diagnostic traversed full, half,
quarter, half, full at requested rates 2200/30200/61700/30200/2200.
`diag_audio_extreme.gdb` passed: restore=31, spills=0, banks=7, final
frequency=2200 and period=1612. This is a hardware-state test, not a listening
acceptance of bank transitions.

## 2026-09-25: first race comparison

Actual emulator output, not reconstructed PCM or microphone/system capture:

- PC: installed DOSBox-X 2026.08.31, normal 286 core, 12,000 cycles,
  `mixer nosound=false`, SDL dummy output, `DX-CAPTURE /V /A /-D`.
  Original executable/data copied into an isolated reference directory with
  BASIC as the only track. The existing reference autotype sequence starts a
  race. Captured video confirms cars moving on BASIC. WAV/video audio is
  signed 16-bit stereo at 48 kHz.
- Amiga: existing native build, PAL A1200, 2 MiB chip and no Fast RAM,
  `NATURALO0` bounded BASIC fixture. Installed FS-UAE uses **SDL2**, not the
  SDL3 version in the newer local source tree. Its log confirms `disk`,
  44,100 Hz, S16LSB, two channels. The successful take uses
  `diag_audio_capture.gdb`, without per-update debugger stops.
- First take used SDL3 environment names against SDL2. They were ignored and
  normal host audio opened; that run was stopped and is not the comparison.
- Local-only deliverables: `tmp/audio-compare/pc-race.wav` is 15 seconds
  starting at 27 seconds in `pc/capture/slicks_001.avi`;
  `tmp/audio-compare/amiga-race.wav` is 15 seconds starting at 8 seconds in
  `amiga-uninterrupted.raw`. No gain change, normalization or filtering.
- These are **separate races**, not matching input/vehicle/pitch fixtures.
  They support an initial check of unwanted loops, missing sounds, clicks and
  engine borrowing. They do not prove exact pitch or matching event timing.
  User feedback: the Amiga excerpt is too early and the engines are idling.
  It is **not accepted** as an active-racing listening comparison.
- This capture reached results with zero audio-VBI blanking spills and then
  passed the clean system-restoration check (`restore=31`).

## Replacement active-race excerpt

`tmp/audio-compare/amiga-active-racing.wav` is the 20-second interval
35..55 seconds from `amiga-fast-effects.raw`. Native `NATURALI` setup uses
four high-skill computer drivers in vehicle 0 on BASIC. A one-time breakpoint
at the first race-effect call reported update 116, `racing=1`, and measured
speeds 237/237/237/237. The selected interval is after that marker and has no
per-update debugger stops. File-only SDL2 output was verified again.
The excerpt has unchanged gain (peak -6.9 dBFS, mean -22.1 dBFS); it is not
normalized to the PC clip. The user was asked to assess this replacement.

An intermediate take used per-update motion checks and is not delivered.
The initially suggested connection between a large observed timestep and
debugger stops was not established: `next_physics_ticks` uses the native
clock accumulator, not host wall time. Per-update stops are still unsuitable
for audio listening because they interrupt emulation/playback. The final
capture observer therefore uses only one temporary first-effect breakpoint.

## Repeating Amiga capture

From the repository root, with a fresh absolute output filename:

```sh
. amiga/env.sh
SLICKS_AUDIO_CAPTURE_FILE="$PWD/tmp/audio-compare/new-take.raw" \
FSUAE_RUN="$PWD/amiga/.run/audio-compare" DEBUG_PORT=2394 \
SLICKS_TRAJECTORY=identical \
bash amiga/debug.sh "$KICKSTART" diag_audio_capture.gdb
```

The capture option takes precedence over `FSUAE_SOUND` and routes SDL2
playback to disk, not speakers. Default debug runs still use the dummy
backend; `run.sh` is unchanged. Existing capture paths are rejected to avoid
overwriting a take. Check the emulator's audio-device log before converting:
the installed version produces headerless S16LE stereo at 44,100 Hz.
The current fixture uses four high-skill computer drivers in vehicle 0. The
observer stops once at the first race-effect call, prints the race state and
four measured speeds, then deletes that temporary breakpoint. There are no
per-update stops. The file byte count divided by 176,400 is the approximate
raw-stream time; choose a clip at least a couple of seconds later to exclude
that debugger pause. Do not mistake idle starting-grid audio for active
racing again.

```sh
ffmpeg -f s16le -ar 44100 -ac 2 -i new-take.raw new-take.wav
```

Stop only the emulator recorded in this run directory after the fixture
exits. Keep recordings, original assets and other byte-derived evidence out
of Git.
