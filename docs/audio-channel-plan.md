# Direct Paula audio

User direction: no software mixing. Use direct playback on Paula's four
channels, keeping all active cars' engines audible when possible and lending
channels to effects. Retain original pitches and use deliberate arbitration.

## Evidence inspected

- Before this change, `amiga_audio.c` reserved channel 0 for one engine, 1/2 for effects,
  and 3 for the results cue. It ignores the engine priority argument. This
  is not a four-engine implementation.
- Original runtime offset 2979b..29999 allocates ordinary sample requests.
  DS:17c1 initially contains 16. Its search visits indices 0..14 and returns
  one-based handles 1..15; it excludes the final slot. This is voice capacity,
  not proof that sixteen samples play simultaneously or sixteen physical
  Sound Blaster outputs. The GUS path at 282bb can replace the configured
  count from DS:17ed. Driver-specific actual mixing needs separate inspection.
- Allocation first refreshes voice state (call 281a2), applies flag-2
  duplicate suppression by priority byte, prefers the first free slot, then
  the first nonnegative signed priority no higher than the new priority.
  This is first eligible, not a lowest-priority search. Zero incoming priority
  becomes 1. If no eligible slot exists, the request returns zero.
- Flag 1 stores priority marker FE (-2) after starting the sample. Ordinary
  effect allocation cannot replace these protected loop voices. Race engine
  calls at 1539b pass flags=1 and priority=100 for each car. Four engines on
  Paula therefore cannot simply inherit the PC protected-voice rule unchanged.
- Existing DOS sound traces include disabled-driver captures (sample handles
  zero). They establish calls, not audible concurrency; do not use them as
  evidence of an active PCM mixer.

## Implemented Amiga allocation policy

1. Track each active car's engine independently, including its original sample
   selection and pitch state, even while that engine is temporarily inaudible.
2. Share all four hardware channels; do not permanently reserve a results or
   effect channel during a race.
3. Effects may borrow engine channels. Among effects, preserve original event
   priorities and duplicate rules. The engine-borrowing rule is an explicit
   platform adaptation, not a claim of identical original voice allocation.
4. Resume a displaced engine at its current pitch when an effect finishes;
   use stable ownership/tie-breaking to avoid unnecessary engine swapping.
5. Keep one-shot completion and looping separate. Preserve display-blanking
   safety for DMA control changes. Normal runs keep audio; debug runs remain
   muted unless an audible test is requested.

## Implementation and verification record

- Matched-pitch listening investigation found the bank has **27** entries:
  26 tS records plus a RIFF/WAVE record at ID 1. The previous scan-to-next-tS
  loader skipped that WAV and shifted every subsequent ID. Original startup
  1000:a118 increments the ID for both formats. A live PC driver observation
  selects handle 18 / logical ID 17 with 2,050 PCM bytes; the previous Amiga
  ID 17 incorrectly contained 2,700 bytes. The loader now parses consecutive
  tS and mono 8-bit PCM WAV records, preserving IDs and rejecting malformed
  records instead of scanning past them. Tests cover all 27 entries, ID 1
  (5,534 bytes), engine ID 17 (2,050), ID 18 (2,700), final ID 26 (5,960),
  truncation, invalid WAV format and complete partial-allocation cleanup.
  The historical claim below that the bank has only 26 samples was incorrect.

- The pitch oracle now also executes original 3000:95b8..96a8 with the
  Sound Blaster branch enabled: 210 vehicle/speed/output-rate cases verify
  the driver's 8-bit fractional sample step is `frequency*256/output_rate`.
  There is no hidden octave multiplier in that conversion. This does not
  itself verify the DSP output clock or an audible emulator recording.

- Added 48 host-adapter lifecycle cases covering stop/pause before either
  staged VBI, during engine/effect/music playback, after silent reload and
  after engine resumption. They check that stopped requests cannot restart
  during 32 further VBIs, resume uses current pitches, and results discard
  racing owners then finish without resurrecting engines. These are adapter
  tests, not proof of every game's menu/event call site.

- Four independent engine samples/pitches now feed four direct Paula channels.
  Every active-car mask is supported. Idle channels are preferred, then engine
  borrowing rotates; each borrowed channel returns to its original car. No PCM
  software mixer was introduced. Results music replaces race audio.
- Host adapter tests cover all 16 active-car masks, simultaneous effects,
  rejection/duplicate rules, resumption at updated pitch, 65,536 priority
  combinations, protected loops, and 10,201 volume configurations.
- Muted A1200/68020, 2 MiB chip/no Fast RAM test on debug port 25133 passed
  300 race updates: engines=f, borrowed=f, resumed=f, blank_spills=0. This
  checks runtime ownership/state, not an auditory comparison.
- Pitch audit found the original uses stored DS:684e measured speed (before
  subsequent collision/surface velocity changes), not final current velocity.
  Its frequency argument wraps at 16 bits rather than saturating. Native race
  audio now uses `measured_speed`. `make verify-audio-pitch` executes original
  2000:2776..27a5 and its multiplication helper: all 655,360 vehicle/speed
  comparisons pass. Zero/unrepresentably low frequencies have a defensive
  Paula-period bound; that boundary is not claimed as exact PC behavior.
- The corrected-pitch build passed another 300-update native run on port
  25135, again engines=f, borrowed=f, resumed=f and blank_spills=0. A preceding
  run on port 25134 failed the combined state guard without detailed values;
  the observer now prints every guard field. On port 25137 this reproduced as
  a main-loop blanking spill at frame 244, with engine mask/state otherwise
  correct. Channel restarts have since moved out of the main loop entirely.
- One-shot durations, silent reload and engine resumption now advance in the
  takeover VBI, independently of simulation. Main/interrupt state changes are
  protected with nested Exec Disable/Enable. Restart commands stop/mute a
  channel in one VBI and program/start its replacement in the next. This
  deliberately adds 20..40 ms dispatch latency rather than relying on a
  two-raster-line DMA shutdown wait. The sample plays once and then reloads
  silence; expiration returns the channel to its engine at the latest pitch.
- Port 25138 passed 300 race updates plus a 120-VBI stalled-game check. Port
  25139 repeated these checks with interrupt-side instrumentation: main spills
  0, VBI spills 0, last VBI exit line 1. Neither staged-restart test emitted
  the UAE DMA-wait-hack warnings seen with the previous implementation.
- The sample loader now reads the big-endian frequency at the end of each
  recognized header. All 26 supplied samples specify 11025 Hz, giving nearest
  PAL period 322. Host tests verify every supplied sample exceeds the 20 ms
  silent-reload delay (also verified against the stricter 40 ms bound).
- Verification limits: no audible comparison or extreme engine-frequency
  hardware-limit proof. Allocation/arithmetic/runtime checks are not an auditory fidelity claim.
  Debug runs remain host-muted; normal `run.sh` keeps audio enabled.

Actionable follow-up work is maintained separately in [open-work.md](open-work.md).
