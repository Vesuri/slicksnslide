#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 SKIDS=222 TIMER=400 WAYPOINTS=4,3,3,3 X=15450 Y=7849 C1X=15346 C1Y=8160 C2X=16212 C2Y=8512 C3X=16769 C3Y=8341 CHECKSUM=bcff35d7 DISPLAY=e9716191'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
