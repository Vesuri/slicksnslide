# Toolchain and pipeline

Host: macOS on Apple Silicon. Target: A1200, 68020, 2 MiB.

## Existing shared tools

- Ghidra 12.1 under `~/.local/share/ghidra`
- OpenJDK 21 under `/opt/homebrew/opt/openjdk@21` (exported by the Makefile for
  headless Ghidra)
- Amiga GCC/vasm/elf2hunk toolchain under `~/.local`
- FS-UAE and M68k GDB under `~/.local/fs-uae`
- The proven Amiga framework and diagnostic patterns in the three preceding
  port repositories

## Additional reference/translation tools

- Capstone: convenient 16-bit x86 decoding and output verification
- Intel XED: optional second decoder with explicit real-16 and chip controls
- Bochs 3.1: primary instruction/memory/branch/I/O reference instrumentation
- DOSBox Staging 0.83: interactive reference and independent subsystem behavior
- DOSBox-X: selectable 286 CPU identity and primary 286 reference tracing
- Unicorn 2.1.4: bounded Compack capture and isolated x86-16 versus M68k
  differential execution

Do not add LLVM or a generated-C backend before a measured need exists. The
production backend emits Motorola-syntax assembly into the established Amiga
toolchain.

## Current commands

```sh
make inspect       # parse and fingerprint the local MZ executable
make hash          # source identity
make prepare-reference # make an ignored writable copy of the distribution
make reference-286 # launch the primary DOSBox-X 286 reference
make reference-staging # launch the independent 386 comparison
make reference-race # run a bounded BASIC.SS race with scripted Enter
make reference-trace # same path through the locally built instrumented DOSBox-X
make reference-frame-hash REFERENCE_VIDEO=tmp/pc-root/capture/slicks_001.avi \
  # hash an RGB24 frame (8 seconds by default)
make verify-reference-race REFERENCE_RACE_VIDEO=tmp/pc-fixed/capture/slicks_001.avi \
  # verify stable BASIC.SS states and live race advancement
make verify-execution-trace \
  # validate and summarize normalized instruction starts and transitions
make verify-interrupt-trace \
  # validate vectors, runtime boundaries, and software-service functions
make analyze-execution-trace \
  # decode control flow and correlate asynchronous interrupt entries
make unpack        # execute only Compack and capture its handoff image/state
make rebuild-mz    # reverse relocations and build a normalized local MZ
make verify-runtime # compare the Unicorn and independent emulator captures
make trace-summary # summarize the bounded DOSBox-X DOS/file trace
make ghidra        # regenerate the local 16-bit Ghidra listing
make ghidra-normalized # analyze the proved, relocation-normalized MZ
make todo          # immediate step and tracked work markers
```
