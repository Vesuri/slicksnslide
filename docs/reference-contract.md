# PC reference contract

The original distribution, not the translated program, defines observable
correctness. Runs occur from a writable ignored copy under `tmp/pc-root` so the
source files in `ref/` remain unchanged.

## CPU personalities

Two reference personalities are intentional:

- DOSBox-X with `cputype=286` is the primary semantic trace. It selects the
  older-CPU Borland runtime paths and matches the translation scope.
- DOSBox Staging with `cputype=386` is an independent compatibility comparison.
  It also reveals behavior hidden behind the runtime CPU probe.

The same named milestone must behave equivalently in both unless a difference
is explicitly attributed to the CPU-dependent library implementation.

## Milestones

| ID | Positive completion signal |
|---|---|
| R0 | **Proved:** independent normalized handoff images match byte-for-byte |
| R1 | **Proved on 286 and 386:** program requests VGA mode 13h |
| R2a | **Proved on 286:** stable title framebuffer at video time 8 s |
| R2b | **Proved on 286:** loading splash reaches the main menu unattended |
| R3 | **Proved on 286:** delayed Enter selects the sole `BASIC.SS` track |
| R4 | **Proved on 286:** the BASIC race framebuffer advances at three checkpoints |

The R2a 640x400 RGB24 framebuffer hash is
`a4910f29ea74c8b7a790d7b56ed5eac25b1b870cf642b2d1e0028aac6e37d9cc`.
Two separate bounded DOSBox-X runs produced it. Later testing showed that this
is a loading splash, not an input wait. At 12,000 cycles the program reaches the
main menu unattended, and a delayed Enter starts a race. A disposable reference
root with only `TRACKS/BASIC.SS` removes the program's random track selection.

Two fixed-track runs matched these stable 640x400 RGB24 checks:

- loading splash at 8 s: `ce0b18140fffbf11454f39bed9fc2a278f8ae7269373598932abd8b1f52b95cf`;
- BASIC selected at 15 s: `cd4fe6bea3852d6a47e99165c8e417d096639234af976f5ab349630c007b3298`;
- BASIC label crop during the race: `b2e5e678e8a49ef080739d38ea8470594cfa55925e7ad65eecece4a1b5e023d0`.

Full race frames at 20, 25, and 30 seconds differ within each run, positively
proving simulation advancement. Their complete hashes intentionally are not a
cross-run gate because AI/timer state diverges slightly.

## Trace products

For every milestone retain locally:

- executed instruction addresses and basic-block entries;
- direct and indirect control-flow edges;
- interrupt number plus relevant input/output registers;
- port input/output events;
- writes into classified executable regions;
- DOS file open/read/write/seek events and filenames;
- selected framebuffer and application-state hashes.

Addresses are normalized relative to the DOS load segment before comparison
with the reconstructed MZ and Ghidra database. Traces and captures are ignored
game-derived material and are never committed.

## Stop conditions

The trace harness must stop with a distinct success reason, instruction/time
limit, unsupported event, or guest failure. Reaching a time limit is never
reported as a successful milestone.
