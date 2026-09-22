#!/usr/bin/env bash
# Run long enough on the strict target to prove checkpoint wrap and lap timing.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-lap.log
mkdir -p .run
SLICKS_AUTO_RACE=1 SLICKS_LAP_RACE=1 \
  ./debug.sh "$KICKSTART" diag_lap.gdb | tee "$LOG"
grep -Fq 'SLICKS_LAP_OK FRAME=700 ' "$LOG"
echo "A1200 full-lap gameplay passed"
