#!/usr/bin/env bash
# Exercise the supplied track with the largest observed navigation table.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-zone.log
EXPECTED='SLICKS_ZONE_OK FRAME=200 ZONES=46 SKIDS=1112 TRACKCOLL=0 CHECKSUM=cc686167 DISPLAY=823431ce'
mkdir -p .run

SLICKS_ZONE_RACE=1 ./debug.sh "$KICKSTART" diag_zone.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 46-zone HEIKKI30.SS gameplay smoke test passed: $EXPECTED"
