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
unpacked `live_vga_plot_plane` and `live_vga_read_pixel` machine-code bodies in
Unicorn's x86-16 engine, including their VGA plane-select port writes, and runs
the assembled native replacements in Unicorn's 68020 engine. A deterministic
corpus of 2,040 observed-shape, edge, and random states compares selected
plane, wrapped 16-bit offset, returned value, and memory effect.

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
