#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 SKIDS=230 TIMER=400 WAYPOINTS=4,4,3,3 X=15450 Y=7849 C1X=15679 C1Y=7951 C2X=15891 C2Y=8333 C3X=16216 C3Y=8316 CHECKSUM=35bb8f32 DISPLAY=0068404e'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
