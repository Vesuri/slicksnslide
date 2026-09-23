#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED_START='SLICKS_GAMEPLAY_START FRAME=0 SKIDS=0 TIMER=0 LIGHTS=1 X=26326 Y=5274 MATERIAL=80f1987a CHECKSUM=ea8fd88d'
EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 STAGE=6 SKIDS=329 COLLISIONS=2 TRACKCOLL=0 TIMER=218 LAP=1 LTIME=218 WAYPOINTS=1,1,1,1 X=24703 Y=11638 C1X=24283 C1Y=12402 C2X=23793 C2Y=12493 C3X=24200 C3Y=11732 CHECKSUM=1646cbe9 DISPLAY=57addfc1'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=0 \
  ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED_START" "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
