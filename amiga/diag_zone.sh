#!/usr/bin/env bash
# Exercise the supplied track with the largest observed navigation table.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-zone.log
mkdir -p .run

SLICKS_ZONE_RACE=1 ./debug.sh "$KICKSTART" diag_zone.gdb | tee "$LOG"
grep -Fq 'SLICKS_ZONE_OK ' "$LOG"
echo "A1200 46-zone HEIKKI30.SS gameplay smoke test passed"
