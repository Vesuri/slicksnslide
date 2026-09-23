#!/usr/bin/env bash
# Exercise a supplied non-BASIC track with different surface/boundary content.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-ice.log
EXPECTED='SLICKS_ICE_OK FRAME=200 ZONES=11 ANIMATED=0 COUNTS=0,0,0,0,0 SKIDS=1040 TRACKCOLL=0 CHECKSUM=ed49462a DISPLAY=b7c815dc'
mkdir -p .run

SLICKS_ICE_RACE=1 ./debug.sh "$KICKSTART" diag_ice.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 ICE.SS gameplay smoke test passed: $EXPECTED"
