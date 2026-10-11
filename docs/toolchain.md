# Toolchain and pipeline

Host: macOS on Apple Silicon. Target: A1200, 68020, 2 MiB.

## Existing shared tools

Installed by AmigaXDev (`make setup`; its `docs/environment.md` lists them):

- Ghidra under `~/.local/share/ghidra`, with OpenJDK 21 (exported by the Makefile
  for headless Ghidra)
- the Amiga GCC/vasm/elf2hunk toolchain, FS-UAE and m68k GDB under `~/.local`
- the `amigaxdev` Python package: `amigaxdev.release` builds the LH5 archive and installer
  icons for `tools/package_release.py`, `amigaxdev.dos.mz_info` (`make inspect`) and
  `amigaxdev.dos.verify_amiga_runtime` (`make -C amiga verify-runtime`)
- the shared Ghidra scripts (`$AMIGA_GHIDRA_SCRIPTS`): `ExportAuditListing` writes the
  listings; `MarkEntries` and `SeedLiveMap` in `ghidra_scripts/` are Slicks' own
- the smoke slave (`whdload/smoke_name.i` names it)
- the dA JoRMaS framework and diagnostic patterns are vendored here; the generic
  lessons behind them are in AmigaXDev `docs/reference/`, and AmigaXDev's canonical
  framework is compared with in `src/platform/amiga/framework/UPSTREAM.md`

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
make reference-race # add a human player and run a bounded BASIC.SS race
make reference-trace # same path through the locally built instrumented DOSBox-X
make reference-frame-hash REFERENCE_VIDEO=tmp/pc-root/capture/slicks_001.avi \
  # hash an RGB24 frame (8 seconds by default)
make verify-reference-race REFERENCE_RACE_VIDEO=tmp/pc-fixed/capture/slicks_001.avi \
  # verify stable BASIC.SS states and live race advancement
make verify-execution-trace \
  # validate and summarize normalized instruction starts and transitions
make verify-interrupt-trace \
  # validate vectors, runtime boundaries, and software-service functions
make verify-port-trace \
  # validate and summarize compact direct port-I/O aggregates
make verify-memory-trace \
  # summarize VGA/runtime memory and reject writes to executed code bytes
make verify-primitive-trace \
  # validate measured native-candidate argument tuples and memory bounds
make analyze-execution-trace \
  # decode control flow and correlate asynchronous interrupt entries
make unpack        # execute only Compack and capture its handoff image/state
make rebuild-mz    # reverse relocations and build a normalized local MZ
make verify-runtime # compare the Unicorn and independent emulator captures
make verify-native-graphics \
  # assemble native 68020 graphics cores and differentially run original x86
  # and new M68k bytes in independent Unicorn engines
make verify-car-collision \
  # check recovered DOS pairwise collision response and fixed-point updates
make verify-surface-effects \
  # check material dispatch, point-actor tuples, thresholds, and RNG order
make verify-native-tracks \
  # parse every supplied track and verify the native scene/surface contracts
make amiga        # build the game (amiga/out/SlicksDiag.exe)
make amiga-run    # display it in FS-UAE; mouse button or Escape exits
make amiga-debug  # attach M68k GDB through FS-UAE's debugger stub
make trace-summary # summarize the bounded DOSBox-X DOS/file trace
make ghidra        # regenerate the local 16-bit Ghidra listing
make ghidra-normalized # analyze the proved, relocation-normalized MZ
make ghidra-live   # seed observed blocks and curated VGA symbols, export listing
make ghidra-live-normalized # apply the same live map to the normalized MZ
make todo          # open work and tracked work markers
make release       # stripped game, production slave and installer helper
make dist          # dist/SlicksNSlide-$(cat VERSION).lha
make release-check # release host checks, determinism, package build and audit
make -C whdload    # production and test WHDLoad slaves (see whdload.md)
```

### Local Amiga inputs

Common, locally owned Amiga files come from the shared `~/.local/share/amiga`
that AmigaXDev installs; each has an environment override. `amiga/env.sh` sources
its `env.sh` (which `run.sh` and `debug.sh` source too), and the Python tests
(`tools/amiga_inputs.py`) use the same names and defaults:

| Variable | Default under `~/.local/share/amiga` | Used by |
| --- | --- | --- |
| `KICKSTART` | `Kickstarts/kick40063.A600` (KS 3.1 40.063) | every FS-UAE launch |
| `SETPATCH` | `SetPatch` | AGA launchers, standalone test |
| `WORKBENCH_ADF` | `Workbenchv2.04rev37.67Workbench.adf` | installer, standalone, WHDLoad tests |
| `INSTALLER43` | `Installer43/Installer` | installer script test |
| `WHDLOAD_BINARY` | `WHDLoad/C/WHDLoad` (19.2; `amiga/env.sh` sets `WHDLOAD`, the SDK the slave builds with, to the same version) | installer and WHDLoad tests |

`tools/test_whdload.py` also takes `--rom`/`--rtb` for the Kickstart pair that
WHDLoad itself loads (default `Kickstarts/kick40063.A600` and `.RTB`).
The AGA launchers copy `SetPatch` and the original `ref/SLICKS.000` resource
archive to the generated boot volume; neither is linked into the native
executable. The installer tests read the publisher's archive from
`ref/Slix151.zip`. FS-UAE screenshots from `run.sh`/`debug.sh` go to the
ignored `tmp/screenshots` (`SCREENSHOT_DIR` overrides); all other scratch
output also stays under `tmp/`.
