#!/usr/bin/env bash
# Source this to expose the shared Amiga cross-toolchain and FS-UAE.
#   . amiga/env.sh
# or, from amiga/:
#   . env.sh
SLICKS_TC="$HOME/.local"
export PATH="$SLICKS_TC/opt/bin:$SLICKS_TC:$SLICKS_TC/fs-uae:$PATH"
export KICKSTART="${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}"
