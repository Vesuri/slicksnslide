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
| R2b | Title accepts deterministic mouse input and reaches the menu |
| R3 | A fixed bundled track is loaded and the pre-race screen is stable |
| R4 | Race simulation advances for a fixed number of ticks |

The R2a 640x400 RGB24 framebuffer hash is
`a4910f29ea74c8b7a790d7b56ed5eac25b1b870cf642b2d1e0028aac6e37d9cc`.
Two separate bounded DOSBox-X runs produced it. The trace records activation of
the INT 33h mouse interface. Scheduled Space, Enter, and Escape keys did not
dismiss the title, so R2b requires mouse injection rather than keyboard-only
AUTOTYPE scripting.

R2b onward need deterministic input scripting and frame/state capture.

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
