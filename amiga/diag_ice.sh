#!/usr/bin/env bash
# Exercise a supplied non-BASIC track with different surface/boundary content.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-ice.log
mkdir -p .run

SLICKS_ICE_RACE=1 ./debug.sh "$KICKSTART" diag_ice.gdb | tee "$LOG"
grep -Fq 'SLICKS_ICE_OK ' "$LOG"
echo "A1200 ICE.SS gameplay smoke test passed"
