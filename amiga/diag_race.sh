#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 STAGE=6 SKIDS=124 COLLISIONS=6 TRACKCOLL=0 TIMER=218 LAP=1 LTIME=218 WAYPOINTS=0,0,0,0 X=25164 Y=9630 C1X=24534 C1Y=10880 C2X=23667 C2Y=10906 C3X=24335 C3Y=9727 CHECKSUM=afb49ae5 DISPLAY=40f123e2'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=0 \
  ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
