#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 SKIDS=228 COLLISIONS=67 TIMER=400 WAYPOINTS=4,3,3,3 X=15033 Y=7822 C1X=15699 C1Y=8242 C2X=16370 C2Y=8677 C3X=17045 C3Y=8419 CHECKSUM=f08e2f05 DISPLAY=e4ad9b38'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
