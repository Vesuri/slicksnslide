# Slicks 'N' Slide for the Amiga

A native A1200 port of Timo Kauppinen's DOS shareware racing game
Slicks 'N' Slide 1.51. The original 16-bit x86 game was recovered from its
executable and rebuilt as native 68020 code: game and UI logic recovered into C
and C++, with hot routines translated to hand-checked 68020 assembly, on a
direct AGA/Paula platform layer. There is no x86 emulator and no generated-C
CPU emulation; the original executable running under a PC reference emulator
is the ground truth that every routine is tested against.

Release **0.90 (01.10.2026)**. The end-user package `SlicksNSlide-0.90.lha`
contains the native executable, a WHDLoad slave and an Installer script that
extracts the original data from the publisher's freely available
[Slix151.zip](https://www.slicksnslide.com/webapi/download.php?p=dos-slix&v=Slix151.zip).
No original game data, registration key, ROM or OS component is in this
repository or the package. See [release/ReadMe](release/ReadMe) for end-user
instructions.

## Target

| | |
|---|---|
| Machine | PAL A1200: 68020, AGA, 2 MiB Chip RAM, no Fast RAM needed |
| OS | AmigaOS 3.x with SetPatch (standalone), or WHDLoad 17+ with 2 MiB Fast RAM |
| Display | original 320x200 picture on a PAL 50 Hz eight-plane AGA screen |
| Sound | four-channel Paula with the original samples |
| Timing | original real-time race clock at any frame rate |

## Building

Host tools are listed in [docs/toolchain.md](docs/toolchain.md). The Amiga
cross-toolchain and FS-UAE set-up follow the sibling ports (Rescue on Fractalus,
Revs, Vette).

```sh
make amiga            # debug/diagnostic game build: amiga/out/SlicksDiag.exe
make -C whdload       # production and test WHDLoad slaves
make release          # stripped game, slave and installer helper
make dist             # dist/SlicksNSlide-$(cat VERSION).lha
make release-check    # host checks, determinism, package build and audit
```

The game executable is a single build; `amiga/` scripts run it under FS-UAE
with GDB fixtures (`amiga/debug.sh ROM fixture.gdb`). Reference work needs the
original `SLICKS.EXE` and data under the ignored `ref/` directory.

## Repository layout

```text
src/game/              recovered game state, tracks, cars, actors, race logic
src/ui/                title, menus, dialogs, help, shop, fonts
src/graphics/          planar/chunky graphics primitives and conversion
src/platform/amiga/    AmigaOS/hardware platform, storage, audio, framework
src/util/              shared low-level utilities
amiga/                 target build, FS-UAE/GDB run scripts and fixtures
whdload/               WHDLoad slave and test slaves
release/               Installer script, ReadMe and icon sources
tools/                 reference capture, oracles, host verifiers, packaging
tools/install-data/    SlicksNSlideInstallData ZIP extraction helper
tools/patches/         DOSBox instrumentation used for reference captures
ghidra_scripts/        headless Ghidra analysis scripts
reference/             DOSBox reference configurations
docs/                  design and reference documentation
```

## Documentation

- [docs/open-work.md](docs/open-work.md): the current work list.
- [docs/architecture.md](docs/architecture.md),
  [docs/phases.md](docs/phases.md): translation design and how the port was
  built.
- [docs/reference-contract.md](docs/reference-contract.md),
  [docs/validation-harness.md](docs/validation-harness.md),
  [docs/external-surface.md](docs/external-surface.md),
  [docs/source-inventory.md](docs/source-inventory.md): the PC reference,
  the oracle tests and the original's DOS/BIOS/hardware surface.
- [docs/fidelity.md](docs/fidelity.md): intentional differences from the
  original.
- [docs/rendering.md](docs/rendering.md),
  [docs/native-graphics-abi.md](docs/native-graphics-abi.md),
  [docs/audio.md](docs/audio.md), [docs/memory.md](docs/memory.md),
  [docs/frame-pacing.md](docs/frame-pacing.md),
  [docs/performance.md](docs/performance.md): runtime design.
- [docs/registration-support.md](docs/registration-support.md): keyfile
  support.
- [docs/install-original-data.md](docs/install-original-data.md),
  [docs/whdload.md](docs/whdload.md), [docs/release.md](docs/release.md):
  installer, WHDLoad and the release record.
- [docs/development-verification.md](docs/development-verification.md):
  working rules and acceptance gates.

## Original executable

| Property | Value |
|---|---|
| File | `SLICKS.EXE` from Slix151.zip, 100,412 bytes |
| SHA-256 | `17e5a2a2daba0f3fcc280dc1b3711a753497bdb956ba74ee88b7e3db095d98eb` |
| Container | DOS MZ, 1991 W. Collis Compack wrapper, entry `0000:0000` |
| Unpacked runtime | 214,048 bytes, entry `1010:0000`, stack `4442:0100`, Borland C++ 1994 runtime |
| Runtime SHA-256 | `d6717daa23f40f0e968e610ee901ce8075c0f92b58240361b5541eedc8b65f0e` |
| Normalized runtime SHA-256 | `c4b8ecdc9e350d782de1cac0019a0a0ad8feb8290fa29f545ae5115b41c88408` (4,192 relocations reversed) |

`tools/unpack_compack.c` runs only the Compack wrapper under Unicorn and stops
at its far transfer to the game; an instrumented DOSBox Staging run captures
the same image independently. The game is 16-bit with a CPU probe and an
optional 386 library path, so the reference is a 286-class DOSBox-X.

## Credits

Original game and data: Timo Kauppinen. Amiga port: Vesuri. Third-party
components and their notices are listed in
[docs/release-credits.txt](docs/release-credits.txt) and
[tools/install-data/README.md](tools/install-data/README.md).
