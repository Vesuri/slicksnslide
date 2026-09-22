#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 STAGE=6 SKIDS=166 COLLISIONS=20 TRACKCOLL=0 TIMER=218 LAP=1 LTIME=218 WAYPOINTS=1,2,2,2 X=22917 Y=11622 C1X=22362 C1Y=12639 C2X=20120 C2Y=12611 C3X=21128 C3Y=12655 CHECKSUM=01a02172 DISPLAY=35b75583'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=0 \
  ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
