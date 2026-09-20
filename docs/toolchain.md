# Toolchain and pipeline

Host: macOS on Apple Silicon. Target: A1200, 68020, 2 MiB.

## Existing shared tools

- Ghidra 12.1 under `~/.local/share/ghidra`
- Amiga GCC/vasm/elf2hunk toolchain under `~/.local`
- FS-UAE and M68k GDB under `~/.local/fs-uae`
- The proven Amiga framework and diagnostic patterns in the three preceding
  port repositories

## Additional reference/translation tools

- Capstone: convenient 16-bit x86 decoding and output verification
- Intel XED: optional second decoder with explicit real-16 and chip controls
- Bochs 3.1: primary instruction/memory/branch/I/O reference instrumentation
- DOSBox Staging 0.83: interactive reference and independent subsystem behavior
- Unicorn 2.1.4: bounded Compack capture and isolated x86-16 versus M68k
  differential execution

Do not add LLVM or a generated-C backend before a measured need exists. The
production backend emits Motorola-syntax assembly into the established Amiga
toolchain.

## Current commands

```sh
make inspect       # parse and fingerprint the local MZ executable
make hash          # source identity
make unpack        # execute only Compack and capture its handoff image/state
make rebuild-mz    # reverse relocations and build a normalized local MZ
make ghidra        # regenerate the local 16-bit Ghidra listing
make todo          # immediate step and tracked work markers
```
