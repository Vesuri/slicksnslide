# Validation harness

Correctness is established at three independent levels.

## Instruction semantics

Random legal x86 states execute in an independent x86 oracle and in the IR.
Compare defined registers, flags, memory effects, faults, and external events.
Undefined flags are masked rather than accidentally canonized.

## Native block lowering

The same initial guest state executes in the x86 oracle and in generated 68020
code under an independent M68k engine. Compare the complete live-out contract,
not merely RAM.

The first concrete gate is `make verify-native-graphics`. It executes the
unpacked pixel and sprite-blit machine-code bodies in Unicorn's x86-16 engine,
including their VGA sequencer and graphics-controller port writes, and runs
the assembled native replacements in Unicorn's 68020 engine. A deterministic
corpus of 2,040 pixel states compares selected plane, wrapped 16-bit offset,
returned value, and memory effect. Another 256 states for each opaque and
transparent blitter compare the complete four-plane 256 KiB result, covering
plane-phase rotation, odd and even widths, zero transparency, row stepping,
and wrapped starting offsets.

## Program behavior

The original reference loop and the translated target record named milestones
and comparable outputs: guest/application state, video state, sound/device
events, files, and control-flow landmarks. Every negative observation requires
a positive control proving that execution reached the surrounding path.

## Performance invariants

Correct output alone is insufficient. Native blocks are also audited for guest
context traffic, helper calls, state materializations, dispatcher exits, and
fallback coverage. A functionally correct regression into CPU-structure
emulation fails the build.
