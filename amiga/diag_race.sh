#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 STAGE=6 SKIDS=163 COLLISIONS=40 TRACKCOLL=0 TIMER=218 LAP=1 LTIME=218 WAYPOINTS=1,2,2,2 X=23028 Y=11503 C1X=22505 C1Y=12527 C2X=20397 C2Y=12678 C3X=21285 C3Y=12349 CHECKSUM=fe05fde3 DISPLAY=5329f76b'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=0 \
  ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
