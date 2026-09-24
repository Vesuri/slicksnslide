# Intermittent horizontal strips (2026-09-23)

Historical investigation log. The original strip report was addressed by
moving audio DMA control to display blanking; the user reported no strips in
that test. Later audio work moved restart handoffs and one-shot service into
VBI, with zero spill checks on the tested runs. See
[audio-channel-plan.md](audio-channel-plan.md) for that evidence and limits.
The diagnostic instructions below describe old, binary-specific experiments;
they are not instructions to hold or modify the current executable.

## Initial report and experiments (historical)

Initial status was unresolved. The user reported brief strips in static scenery, with
different Y coordinates on different frames. Absence of cars or particles in
the affected area rules out assuming that it is an ordinary dirty update.

The earlier C2P-only diagnostic build omitted `slicks_chunky_pixels_to_amiga` entirely.
Only Kalms C2P rectangles update the race bitplanes. This deliberately leaves
some particle and HUD changes stale. The user is testing whether the strips
persist. Do not re-enable the sparse writer or replace that executable before
this comparison is complete. Disappearance would implicate the sparse path
or the timing change; it would not alone prove a memory-addressing defect.

Current source now restores the sparse writer after the comparisons below:
the user observed no strips with audio processing moved into display blanking.
Blanking-timed audio is now the default. The old separate test executables are
unchanged. Normal sparse updates read authoritative chunky pixels; particles
are written to chunky before either display conversion. The C2P-only diagnostic
deliberately left sparse updates/removals stale until an enclosing rectangle
converted them, so its particle appearance is not a fidelity reference.
An added production-assembly regression writes sparse pixels and then converts
an overlapping rectangle from the same source; the pixels survive. All 1,394
writer cases pass. Particle lifetime/persistence is a separate gameplay rule;
this test does not establish that those rules match DOS.
The restored-rendering A1200 run reached frame 700 with 98 particles and the
prior exact logical/display checksums `6f6768dd` / `5e292a80`. Its sampled
processing cost was 309 raster lines (about 19.8 ms), excluding blanking idle.
Audio work stayed within blanking through that point: 0 spills, maximum 10
raster lines. This is one sampled performance result, not a worst-case bound.

The facade BitMap stores BytesPerRow=320, including all eight interleaved
40-byte plane rows. Both conversion paths use that stride. An experimental
extra multiplication by eight was incorrect and was reverted. It is not a fix.

`bash tools/verify_planar_writes.sh` assembles the production routines and runs
them on Unicorn's 68020 CPU. An independent bit-by-bit reference checks the
whole 64,000-byte bitmap. A memory-write hook checks every destination write,
including writes subsequently overwritten. It covers all 32-aligned horizontal
rectangles, several top coordinates and heights, and sparse lists through
512 entries. All 1,392 cases passed, including full-height rectangles. This establishes bounded writes for those
inputs; it does not validate live dirty records, interrupt interactions, or
AGA DMA and palette behavior.

The same test measures 32 bytes of packed-source lookahead in Kalms' modulo
path. Full-frame chunky and packing buffers now include 32 extra allocated
bytes, with matching free sizes. These are discarded pipeline reads: the
independent output checks still pass. This fixes an allocation-boundary issue,
not the intermittent strip diagnosis. This padding is included in the freshly
rebuilt named C2P-only comparison executable requested by the user.

`make verify-dirty-tracking` also exposed dropped updates when the shared
512-entry particle/HUD pixel list fills. The regression failed at the first
overflow pixel before the fix. The C marker now sends overflow pixels into
the existing bounded, merged rectangle list. It passes along with the surface,
physics, and car-collision host oracles. Particle-expiry assembly runs before
HUD/draw markers and can append at most 256 entries to the cleared list in
the normal frame sequence. This overflow fix is included in the refreshed
comparison build; it addresses stale pixels, not proven writes into static scenery.

Prior FS-UAE replay checks at frames 200 and 700 passed their existing full
logical/display checksums with the sparse writer enabled. A fixed-address
watchpoint and dirty-list traces did not reproduce the screenshot corruption.
They do not exclude an intermittent error at another address. The hardware
fetch setup in CopperList::setPlayfield matches the supplied DanceDiverse3
framework; this source comparison is not proof of correct scanout at runtime.

Next evidence needed: the user's comparison result, then a live check of
bitmap writes and scanout during an affected frame. Preserve the distinction
between framebuffer corruption, palette/register changes, and fetch artifacts.

An opt-in live audit is enabled by the `BITMAPAUDIT` executable argument and
checked by `amiga/diag_bitmap_audit.gdb`. Assigning its byte flag through the
current debugger returned zero immediately, so debugger assignment is not
used as activation evidence. The GDB check requires flag readback at frame 0.
The audit reuses the title-source buffer for a diagnostic
snapshot, comparing all final changed bitplane bits against the frame's declared
rectangles and individual pixels before clearing their lists. The first
unauthorized change records frame, x/y, plane, byte offset, and before/after
values at a debugger breakpoint. This is disabled by default and is not used
for dirty tracking. Enabling it perturbs timing substantially; it cannot detect
a transient write repaired before the comparison, nor a scanout-only fault.
The isolated audit completed at frame 700 with activation confirmed at frame 0
and no unauthorized final changes. The user observed the original intermittent
strips in this audit run: each lasted one display refresh even with game updates
slowed to approximately 2 FPS by the audit. Thus the strips occur with the
sparse writer disabled and without an unauthorized framebuffer change remaining
at the frame boundary. This does not exclude transient writes repaired within
an update, or palette/register/fetch behavior.

The `BITMAPFAULT` positive control flips plane 0 at x=0, y=100 after the first
update. `amiga/diag_bitmap_fault.gdb` verified that the audit stopped at exactly
frame 1, x=0, y=100, plane 0. Both audit runs used separate boot directories and
ports. The named user comparison executable and its boot copy remain unchanged.

`amiga/debug.sh` now respects the existing `FSUAE_RUN` override for boot files
as well as process bookkeeping, and accepts `SLICKS_DEBUG_BUILD` as an ELF/EXE
prefix. An isolated run uses `.run/bitmap-audit`, a separately checked unused
`DEBUG_PORT`, `SLICKS_DEBUG_BUILD=../tmp/SlicksBitmapAudit`, and
`SLICKS_BITMAP_AUDIT=1`. Build that executable with
`make -C amiga OUT=../tmp/SlicksBitmapAudit` after sourcing `amiga/env.sh`.

The separate `SCANOUT` mode holds the initial native race screen without race
steps, audio updates, or conversions. Input and vertical blanks remain active.
`SLICKS_SCANOUT_ONLY=1` with `diag_scanout.gdb` completed 500 vertical blanks at
race frame 0 with identical initial/final bitmap checksum `59574816`. This is
memory stability evidence only; no visual absence of flashes was established.
Its build is `tmp/SlicksScanout.exe`; it does not replace the user's named
C2P-only executable. This first run was not visually observed.

For a human-observed frozen test, omit `diag_scanout.gdb`: that script exits
after its 500-refresh check and is not a sustained visual test. Launch with
`FSUAE_RUN=.run/scanout-watch DEBUG_PORT=24868
SLICKS_DEBUG_BUILD=../tmp/SlicksScanout SLICKS_SCANOUT_ONLY=1
./debug.sh "$KICKSTART"` from `amiga/` after sourcing `env.sh` (first check
that the chosen port is unused). In GDB, break at
`slicks_diag_gameplay_ready` and continue. At the breakpoint verify
`g_slicks_diag_scanout_only == 1`, `g_slicks_diag_scanout_frames == 500`, and
`g_slicks_diag_race_frame == 0`; disable that breakpoint and continue again.
Leave the emulator and debugger running for the observer. Do not count an
unwatched run as visually clean. The user-requested repeat reached these
values and was resumed for sustained observation (result below).

The user subsequently reported no strips during this sustained frozen-grid
run. This narrows the reproduction to activity absent from the frozen mode;
it does not uniquely implicate C2P, since race and audio updates were also
disabled. Next controlled comparison: preserve the frozen scene while
repeatedly converting unchanged pixels.

The isolated writer verifier now also checks the value of every rectangle
destination store against the final reference byte, not just its address.
All 1,392 cases still pass: the tested C2P rectangles do not temporarily store
transpose intermediates in the visible bitmap. Sparse per-pixel updates are
excluded from this stronger value assertion because multiple pixels may
legitimately update the same byte sequentially.

`SCANOUTC` (`SLICKS_SCANOUT_ONLY=2` in the launcher) keeps the same frozen
scene but repeatedly runs production rectangle C2P on unchanged chunky data.
It sweeps 64-pixel-wide, up-to-eight-row rectangles through the screen, starting
each conversion in display blank as the race path does. The separate
`tmp/SlicksScanoutC2P` build was launched in `.run/scanout-c2p-watch` on port
24869. Mode 2 was confirmed at entry; after 500 conversions race frame remained
0 and bitmap checksum remained `59574816`. Both breakpoints were disabled and
execution resumed for visual observation. This exercises small conversions,
not the timing/load of full race updates. Its visual result is recorded below.

The user reported no strips in this small-rectangle comparison either.
This does not exclude scanout overlap: these small calls start in the lower
blanking interval. `SCANOUTF` (`SLICKS_SCANOUT_ONLY=3`) instead repeats full
320x200 conversions after the VBI without waiting for display blank. It keeps
the source and gameplay frozen and checks its bitmap after 50 conversions.
The isolated build is `tmp/SlicksScanoutFull`, boot directory
`.run/scanout-full-watch`, debugger port 24870. This is a stress comparison,
not a proposed production full-frame conversion strategy.
Runtime verification confirmed mode 3, 50 conversions, race frame 0, and
unchanged bitmap checksum `59574816`. The breakpoint was disabled and execution
resumed; visual observation is pending.
The user reported no flashing strips in this full-screen conversion test.
They also tentatively observed that normal gameplay only starts flashing once
the cars reach a particular part of the track. The location has not yet been
identified. Investigate gameplay-dependent source changes/events; these
negative frozen tests do not prove that conversion timing is irrelevant under
live gameplay, but unchanged conversion alone has not reproduced the symptom.

Live gameplay was then resumed with dummy host audio output (emulated audio
still active), using `diag_display_events.gdb` on port 24871 in
`.run/display-events`. The user again observed stripes. The trace sampled
all car positions/materials, particle counts, and cumulative sound events
every 50 updates through frame 700, then disabled its breakpoint. A later
manual pause after the report found frame 1377, 56 particles, car X values
`17970,14576,15503,14650`, Y values `6922,6448,6371,6508`, and material 2 for
car 0 (0 for the other cars). This records the later observation state, NOT
the onset frame or the frame containing a stripe. Execution was resumed.
The original first-flash location remains unknown. The trace also shows sound
events already occurring by frame 150, before the first sampled surface
changes near frame 350; mere presence of audio/particles is not an onset proof.

The user places the likely first onset at the bottom-right first corner and
requested a complete particle-disable comparison. `PARTICLESOFF` (launcher
`SLICKS_NO_PARTICLES=1`) sets `slicks_race_disable_particles` before race
preparation. It bypasses pool insertion, restoration, advancement, and drawing;
surface emission decisions, random-number calls, sound events, cars, and the
existing C2P-only display path remain active. Smoke and skidmarks are therefore
absent. This necessarily also changes rendering workload/timing, so absence of
stripes would implicate the particle path or its load, not prove a specific
particle memory bug. The default switch is zero and normal startup is unchanged.
The separate executable is `tmp/SlicksNoParticles.exe`, boot directory
`.run/no-particles`, debugger port 24872; host audio is silent via dummy output.
At frame 700 the debugger confirmed the disable flag was 1, active particles
and total inserted skidmarks were both 0, and all four car X/Y positions matched
the particle-enabled trace exactly. The breakpoint was disabled and the test
left running for the user. Visual outcome is pending.
The first run completed without being watched and was restarted at the user's
request. The user then observed stripes with particles disabled. Particle
simulation/restoration/drawing is therefore not necessary to reproduce this
glitch. Next comparison restores particles and disables emulated audio
playback, distinct from the dummy host audio output used by all debug runs.

`NOAUDIO` (`SLICKS_NO_AUDIO=1`) retains sample allocations but clears audio
readiness before starting the race, so playback/update entry points return
without audio register writes. Normal startup remains unchanged. This runs
with particles enabled in `tmp/SlicksNoAudio.exe`, `.run/no-audio`, port 24873.
At race frame 0, audio readiness was 0, particle-disable was 0, and the low
four audio-DMA enable bits of DMACONR were 0. The breakpoint was disabled and
the race resumed for observation; its visual result is pending. Host output
also remains on the dummy driver. This comparison changes audio work/timing
as well as DMA, so a clean result would require further isolation.
The first audio-disabled run was missed and was restarted at the user's
request. The user then reported NO stripes in the repeat and suggested that
audio writes might accidentally address other custom registers. This is the
first clean live-gameplay comparison. Inspect actual audio-register accesses;
do not infer that muting host output or disabling particles solved it.

Static inspection of the compiled engine/music/update/effect routines found
audio pointer/length/period/volume writes in `$dff0a0..$dff0d8` and audio-only
DMACON masks; the effect selector yields channels 1 or 2, while engine/music
use 0/3. This is not a live bus trace and does not exclude runtime corruption.
`DMAOFF` (`SLICKS_AUDIO_NO_DMA=1`) keeps audio routines, register writes, and
their raster waits active, but replaces the DMA-enable mask with zero. It
retains sample allocations and particles. The separate build is
`tmp/SlicksAudioNoDMA.exe`, boot directory `.run/audio-no-dma`, port 24874.
This distinguishes register programming with no DMA from the broader NOAUDIO
test; altered bus load/timing remains a confounder.

The user corrected their response: stripes DO appear in DMAOFF. Audio DMA
is not necessary for reproduction; audio programming and its timing remain
suspects.

`diag_audio_addresses.gdb` instruments the compiled store instructions in
`tmp/SlicksAudioNoDMA` without rebuilding that executable. It records effective
CPU addresses and values before each selected audio store, validates audio
register offsets and DMA masks, and checks instruction opcodes before applying
its binary-specific offsets. It is not a chipset bus trace. During setup, two
GDB expression-parsing issues were corrected; the second run paused before the
channel-0 volume store, which was logged manually before resuming. Through
frame 700 it recorded 1,026 writes, all valid. Addresses were `$dff096` and
`$dff0a0/a4/a6/a8`, `$dff0b0/b4/b6/b8`, `$dff0c0/c4/c6/c8`; DMACONR audio bits
were still zero. No music path or post-race shutdown was reached. Log:
`amiga/.run/audio-addresses/writes.log`. Debugger timing is perturbed and visual
reproduction during this particular trace is unconfirmed. This weighs against
a simple incorrect effective-address calculation, not against the user's
observed audio-dependent glitch. Further isolate DMACON strobes versus channel
register writes and raster waits.
