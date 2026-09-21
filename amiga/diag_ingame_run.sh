#!/usr/bin/env bash
# Boot the strict target and prove that the captured BASIC race frame reached AGA.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_IN_GAME=1 CHECKSUM=625354a5 DISPLAY=32409ea5'
LOG=.run/diag-ingame.log
mkdir -p .run

./debug.sh "$KICKSTART" diag_ingame.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 in-game checkpoint passed: $EXPECTED"
