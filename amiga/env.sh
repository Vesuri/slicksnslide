#!/usr/bin/env bash
# Source this for the shared Amiga environment that AmigaXDev installs (~/.local/share/amiga):
# the toolchain, FS-UAE and the shared Python on PATH, and the Kickstarts, Workbench, SetPatch,
# WHDLoad and LHa paths.  Every variable can be overridden from the environment.
#   . amiga/env.sh
# or from the amiga/ directory:
#   . env.sh
. "${AMIGA_ENV:-$HOME/.local/share/amiga/env.sh}"
# Run every emulator on AmigaXDev's native build, which make setup keeps current with its
# FS-UAE patches (gdb stub, determinism, 040/060 MMU write attributes).
export FSUAE="${FSUAE:-$FSUAE_ARM}"
