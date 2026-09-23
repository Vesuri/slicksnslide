#!/usr/bin/env bash
# Run long enough on the strict target to prove checkpoint wrap and lap timing.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-lap.log
EXPECTED='SLICKS_LAP_OK FRAME=700 LAP=2 LTIME=184 TIMER=1218 WAYPOINT=1 TRACKCOLL=0 X=24443 Y=14250 CHECKSUM=fc0744d6 DISPLAY=607eb4f9'
mkdir -p .run
SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=1 \
  ./debug.sh "$KICKSTART" diag_lap.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 full-lap gameplay passed: $EXPECTED"
