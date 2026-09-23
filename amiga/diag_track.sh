#!/usr/bin/env bash
# Prove that a second original track runs through the generalized loader.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-track.log
EXPECTED='SLICKS_TRACK_OK FRAME=200 ZONES=25 SKIDS=0 TRACKCOLL=0 TIMER=218 WAYPOINTS=2,2,2,2 X=24271 Y=15305 CHECKSUM=656b2d0b DISPLAY=f33b9d14'
mkdir -p .run
SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=0 SLICKS_TRACK_RACE=1 \
  ./debug.sh "$KICKSTART" diag_track.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 alternate-track gameplay passed: $EXPECTED"
