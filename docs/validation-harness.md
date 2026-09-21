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
and wrapped starting offsets. A further 256 readback states compare the full
64 KiB output segment, including header, rotated payloads, right-edge padding,
and untouched bytes surrounding the result. Another 256 sub-rectangle states
compare all four destination planes across source cropping, independent source
and destination row strides, phase rotation, and 16-bit destination wrap.
The direct plot boundary adds 512 states across all four explicit native write
planes, checking the original byte effect and native live-register contract.
Composed caller/callee tests add 33 title-page and 34 title-crop cases. The
palette nearest-colour utility adds 512 cases, including high requested bytes
that expose the original's asymmetric signed/unsigned component handling. The
composed title UI prefix adds 256 cases covering every initial animation
counter value and its complete caller/callee graph. The colour-slot helper adds
512 cases spanning every signed low-byte slot twice, including in-range,
out-of-range, and fallback mutations with endian-sensitive word results. The
native span filler adds another 256 whole-framebuffer cases covering all
four boundary phases, empty rectangles, both observed page bases, and the
original multi-plane VGA write masks. The complete native differential corpus
also includes 64 composed bevel cases spanning the observed title-menu control
and randomized legal geometry, colour, and page states. The shared title tail
adds 256 composed phase, palette, and render-state cases. The corpus currently
contains 6,011 cases.

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
