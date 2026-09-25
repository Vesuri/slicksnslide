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

## Idle/slowdown investigation

The user reports that the corrected active-racing engine still sounds like
the high-RPM sample when cars slow down, and recalls a distinct idle sound
in PC captures and the older Amiga build. This remains unresolved. Falling
pitch alone does not establish that the correct PCM sample is playing.

`AUDIOFALL` (`SLICKS_AUDIO_PITCH_TEST=3`) plays one vehicle-0 engine at
speeds 0/1000/500/0, five seconds each, through the production adapter.
The 2 MiB/no-Fast A1200 run passed `diag_audio_deceleration.gdb`, restoring
the system with status 31, no VBI spills, final frequency 2200, period 1612
and only the normal waveform bank visited. This is a pitch/state diagnostic,
not acceptance of the reported idle-sample discrepancy.

The pitch oracle additionally executes the whole original engine-update
block, including pan and sound-driver calls: 440 vehicle/car/speed cases
preserve an initially supplied sample handle while updating pitch through
acceleration and deceleration. It does not establish which sample the
original race originally allocated or what other sounds play alongside it.

### Read-only PC PCM observation

`tools/patches/dosbox-x-slicks-engine-state.patch` adds an opt-in
`SLICKS_TRACE_ENGINE=1` observer to the already instrumented reference core.
It records engine state after the original update call at runtime offset
126b3, including the actual PCM far pointer, sample length/current cursor,
and FNV-1a hash of every PCM byte. It dumps each car's initial PCM for an
independent byte comparison. Unlike `SLICKS_PITCH_PROBE`, it does not replace
sample requests, frequencies, controls or emulated memory.

The fresh local-only run in `tmp/audio-pcm-identity` ran for 60 emulated
seconds, with the original executable, dummy speaker output and WAV capture.
It observed 2,740 engine updates (685 per car), starting at tick 38620.
Cars used vehicles 0/0/1/6. The first three used driver handle 18, logical
sample 17, length 2050, PCM hash d7f7d505. Vehicle 6 used handle 21,
logical sample 20, length 6346, hash 85b8f8b8. Every observed pointer,
length and hash stayed unchanged, including deceleration. Observed maximum
speeds were 1058/1734/1824/1259; subsequent minima were 0/338/320/365.
This establishes no waveform switch in these observations, not in every
possible vehicle, sound-driver mode or original capture.

All four PC dumps match their corresponding production Amiga sample bytes
exactly. The optional vehicle/path arguments to `verify_amiga_audio_volume`
also check the programmed DMA source and length through acceleration,
slowdown, idle, an extreme-rate excursion and return to idle. Reproduce:

```sh
build/verify_amiga_audio_volume \
  0 tmp/audio-pcm-identity/slicks-engine-car-0.raw \
  0 tmp/audio-pcm-identity/slicks-engine-car-1.raw \
  1 tmp/audio-pcm-identity/slicks-engine-car-2.raw \
  6 tmp/audio-pcm-identity/slicks-engine-car-3.raw
```

The first comparison attempt failed on the fourth dump when
it assumed all cars used sample 17; the trace identifies the actual different
vehicle. The earlier active Amiga capture used four vehicle-0 cars and is
therefore not a matched-fleet comparison. Other original requests in this
run include samples 2/3/4 (road effects), 5/6, 8 and 25. Their contribution
to the perceived idle sound has not been established.

A separate per-frame native pitch-tracking run was stopped before its
600-frame completion check; it is not counted as a passing regression.

### Matching-vehicle listening excerpts

`SLICKS_TRAJECTORY=pc-audio` selects the opt-in `NATURALQ` fixture with
vehicles 0/0/1/6, four high-skill computer drivers, on BASIC. It supplies
setup inputs only; production race and audio code perform the simulation.
The recording's first effect marker was frame 116, speeds 237/237/230/236,
and confirmed vehicles 0/0/1/6 at raw byte 2,211,840 (12.54 s).

Delivered local-only excerpts under `tmp/audio-pcm-identity`:

- `pc-traced-fleet.wav`: seconds 39..59 of `capture/slicks_000.wav`,
  the actual PC run described above, 48 kHz stereo PCM.
- `amiga-matched-fleet.wav`: seconds 35..55 of `amiga-matched-fleet.raw`,
  actual PAL A1200/2 MiB/no-Fast output, 44.1 kHz stereo PCM. The SDL log
  confirms file-only disk output; no per-frame debugger stops were used.
- Neither excerpt is normalized, filtered or resampled. They match vehicle
  types, not input, speed or event timing. The native capture was deliberately
  stopped after recording enough audio; it is not a full-race restoration test.

The user was asked whether the matching-vehicle clip contains the reportedly
missing engine sound, or to identify a time in a PC excerpt for closer tracing.

### Actual A1200 PCM and restoration

The separate `SLICKS_AUDIO_PCM_TEST=1` / `NATURALQB` fixture exits through
ordinary cleanup at update 150. `diag_audio_pcm_identity.gdb` only reads
target state and dumps loaded samples 17 and 20 after the first racing
effect. On PAL A1200/68020, 2 MiB chip/no Fast, it passed with fleet 0/0/1/6,
lengths 2050/6346, restore status 31 and zero audio-VBI blanking spills.
Byte comparisons of these actual target-memory dumps against all four PC
engine dumps pass, independently of the host adapter test.

The initial version attempted to exit by writing the diagnostic flag through
GDB. That run kept racing and was interrupted, not counted as a restoration
pass. As already recorded in `player-setup-completion.md`, this debugger's
target writes are unreliable. The passing replacement sets its bound in
native code and uses no debugger target writes.

## Extreme engine frequencies

The production adapter now prepares half- and quarter-length engine loops
at load time, with box filtering. Above the normal waveform's safe DMA rate
it selects a shorter waveform and scales the sample clock by the actual
loop lengths. Playback is still direct Paula DMA, without software mixing.
Bank changes use the existing blanking-safe restart and downward hysteresis.
Normal-rate playback retains the original waveform.
The sixteen reduced loops occupy 28,826 additional bytes of chip RAM.

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

### Corrected active-race take

`tmp/audio-compare/amiga-corrected-racing.wav` contains seconds 35..55 of
`amiga-final-active.raw`, recorded after the bank-ID, sample-gain,
high-frequency and results-order fixes. It uses the same `NATURALI`
four-computer/vehicle-0 fixture. The first effect marker was frame 116,
racing=1, measured speeds 237/237/237/237, at raw byte 2,277,376 (12.91 s).
The excerpt is well after that one-off debugger stop. The owned emulator
log confirms SDL disk output; no per-frame debugger stops were used.
Export retained 44,100 Hz stereo PCM, without normalization or resampling.
It was delivered alongside the original PC race excerpt for the remaining
effects/looping/engine-interruption listening check, not a matched race.

### Procedure

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
