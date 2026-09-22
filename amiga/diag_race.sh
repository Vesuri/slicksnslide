#!/usr/bin/env bash
# Boot the strict target and prove that the BASIC.SS race is live.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_GAMEPLAY_OK FRAME=200 STAGE=6 SKIDS=84 COLLISIONS=43 TIMER=218 LAP=1 LTIME=218 WAYPOINTS=2,2,2,1 X=21333 Y=12834 C1X=22130 C1Y=12816 C2X=22686 C2Y=12310 C3X=23359 C3Y=12079 CHECKSUM=cebb9342 DISPLAY=0ba57554'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 ./debug.sh "$KICKSTART" diag_gameplay.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 live native gameplay passed: $EXPECTED"
