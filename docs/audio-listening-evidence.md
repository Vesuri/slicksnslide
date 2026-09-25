# Audio capture and listening evidence

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
