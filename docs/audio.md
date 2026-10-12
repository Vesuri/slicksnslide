# Audio

Playback uses Paula's four DMA channels directly. There is no software
mixer: each channel plays one sample at a time, at the original pitch, and
the channels are allocated by a deliberate priority policy. The adapter is
`src/platform/amiga/amiga_audio.c`; channel allocation is
`src/game/audio_channels.h`, and the original arithmetic it reuses is in
`audio_pitch.h`, `audio_sample.h` and `audio_volume.h`.

## Sample bank

`samples.dat` holds **27** entries in archive order: 26 `tS` records plus one
mono 8-bit RIFF/WAVE record at ID 1. Both formats advance the ID, as the
original startup does; malformed or truncated records reject the whole bank.
Each sample is converted once at load into Chip RAM as signed PCM using the
full stored amplitude. The DOS header gain is consumed but ignored: Paula
plays each voice on a separate hardware channel, so the port does not retain
the DOS mixer attenuation. This user-requested adaptation raises gain-100
samples by approximately 2.55 times (8.1 dB); gain-255 samples are unchanged.
Conversion is only unsigned-to-signed PCM (XOR 0x80), with no multiplication
or division. The source archive is unchanged. Its Paula period comes from the big-endian frequency in its
header; the supplied samples are 11,025 Hz, period 322. The results music is
a separate WAV loaded by `slicks_amiga_audio_add_music`.

Engine samples are the blocks the original assigns per vehicle (IDs 17..24).
For each of them the loader also prepares half- and quarter-length loops by
box filtering (about 28 KiB of extra Chip RAM). This is load-time
resampling, not run-time mixing.

## Pitch and volume

The engine frequency is the original `base * 100 + slope * measured_speed`,
using the stored measured speed (before later collision/surface velocity
changes) and wrapping at 16 bits like the original driver argument. Paula's
period is `3546895 / frequency`, clamped to 124..65535. When the requested
rate would need a period below 124, the engine switches to the half or
quarter loop and scales the clock by the actual loop lengths; it returns to
a longer loop with downward hysteresis. Zero or unrepresentably low
frequencies are clamped, not claimed as exact.

Channel volumes map the original master volume and background gain to Paula's linear
0..64 range: effects and engines use gain 255, music uses the original
background-volume gain. Volume changes are written once when they differ.

## Channel allocation

1. Each active car owns a home channel (car *n* on channel *n*) for its
   engine, with its own sample bank and pitch. Its state is tracked even
   while an effect has borrowed the channel.
2. No channel is permanently reserved for effects or results during a race.
3. An effect request applies the original rules first: zero priority becomes
   1, and flag 2 rejects a duplicate of an already playing effect of the
   same priority. It then takes an idle channel, else borrows an engine
   channel (rotating only when borrowing), else replaces the first playing
   effect whose signed priority is nonnegative and no higher than the new
   one. Looping effects (flag 1) get the original protected priority -2.
   Engine borrowing is an explicit Paula adaptation, not a claim of the
   original's voice allocation.
4. When a one-shot effect expires, its channel returns to the home engine
   at that car's current pitch and bank.
5. Results music replaces race audio. The final race frame's effect batch is
   dispatched before stop-all and the results cue, as the original race-loop
   exit does, so late race sounds cannot resurface under the results.
   Pause stops all channels; resume restarts the engines only.

## Pre-race cues

**Known integration defect:** these sound requests currently run while the
initial light remains static. F20 has removed the extra stationary wait.
The complete DOS start sequence is not yet matched; see F21 in
[fidelity.md](fidelity.md#confirmed-startup-defects--2026-10-12).

Fresh race entry plays sample 1 four times, following original code
`25376..253f4`: priorities 70, 70, 80, 70 at 0, 400, 900, 900 ms. The final
voice is retuned to 13,000 Hz (PAL Paula period 272); the other voices keep
the sample rate of 11,025 Hz (period 322). The last two requests deliberately
overlap. Priority is channel allocation priority, not volume.

The 900 ms prelude runs before race updates and engine startup. Interrupts
remain enabled; the PAL VBI services audio while the caller waits 20 then
25 frames. All fresh-race routes use it; pause/resume starts engines without
replaying cues. Engines join the channel allocation without cutting off
unfinished cues: any occupied home channel returns to its engine when its
one-shot finishes. This uses the existing four-channel adaptation.

## VBI service and latency

Paula has no one-shot DMA mode, and DMA control must not run in the visible
display (see below). All channel restarts and one-shot bookkeeping
therefore run in the takeover vertical-blank interrupt
(`slicks_amiga_audio_vblank`), independently of the game update rate:

- a restart mutes the channel and clears its DMA bit in one VBI, then
  programs pointer, length, period and volume and sets DMA in the next;
- one VBI after the start, a one-shot channel's reload pointer becomes a
  two-byte silent loop, so the sample plays exactly once;
- expiry counters, in VBIs derived from sample length and period, return
  the channel to its engine.

Game code only queues requests; the main loop and the interrupt share state
under nested Exec `Disable`/`Enable`. Main-loop writes are limited to engine
period updates, changed volumes and the immediate stop-all. The design adds
**20..40 ms** dispatch latency instead of waiting a few raster lines for DMA
shutdown. `g_slicks_audio_vbi_spills` counts VBIs that end inside the
visible playfield; tested runs report zero.

## Why DMA control stays in blanking

Intermittent horizontal strips appeared in static race scenery for single
refreshes. They persisted with particles disabled and with audio DMA
disabled but channel programming and its raster waits active, and
vanished only when audio updates were removed entirely; bitmap audits
showed no stray framebuffer writes, so the fault was in scanout, not in
pixels. Moving audio DMA control into display blanking removed the strips,
and the later VBI staging keeps it there. Do not move channel programming
or DMA control back into display time.

## Verification

- `make verify-amiga-audio-volume` drives the real adapter against mocked
  custom registers: all 27 samples, every active-car mask, borrowing and
  resumption, duplicate and priority rules, staged restarts, pause/stop and
  results lifecycles, bank switching and allocation-failure cleanup.
- `make verify-audio-pitch` and `make verify-audio-volume` compare the pitch,
  sample-gain and volume arithmetic with the original x86 code.
- The user accepted the matched PC/Amiga listening comparison on 2026-09-26.
  That acceptance does not claim identical race timing, sample phase or the
  PC's richer simultaneous-voice mix.

## Full-amplitude Paula samples (2026-10-12)

The adapter volume/lifecycle checks pass with full-range PCM expectations for
both tS header formats. The original DOS gain helper remains oracle-tested
for reference, but production sample-bank loading no longer calls it.
The Amiga build and `OPTIONSV` menu-to-race volume smoke test also pass
(`AUDIO_NATIVE_MENU_TO_RACE_VOLUME_OK`, Paula 16/3 at options 25/20).

## Starting-cue restoration (2026-10-12)

- `make verify-audio-start` executes the original pre-race branch under the
  x86 oracle and compares all four requests, priorities, both waits and the
  final voice-specific pitch change. Sabotages changing 400 to 420 ms and
  13,000 to 11,025 Hz are both rejected.
- `make verify-amiga-audio-volume` verifies both final DMA periods, overlapping
  cues, eventual engine ownership for all 16 car masks, and cue-free resume,
  in addition to the existing volume, sample and lifecycle checks.
- The Amiga build and `diag_audio_start.gdb` with `SLICKS_AUTO_RACE=1` and
  `SLICKS_DEBUG_WARP=1` pass: `AUDIO_START_RACE_OK`, four cues at VBI
  0/20/45/45, final period 272, and successful engine/race startup.
- `OPTIONSV` menu-to-race volume check passes after the cue restoration.
- `SLICKS_PAUSE_LIVE=1` / `diag_audio_pause.gdb` passes: two engine starts,
  thirteen pause/dialog stops, exactly one cue sequence, full OS restoration
  and no audio VBI spills.
