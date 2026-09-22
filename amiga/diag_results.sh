#!/usr/bin/env bash
# Prove that a one-lap race orders every finisher and draws the result panel.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-results.log
mkdir -p .run
SLICKS_AUTO_RACE=1 SLICKS_RESULTS_RACE=1 \
  ./debug.sh "$KICKSTART" diag_results.gdb | tee "$LOG"
grep -Fq 'SLICKS_RESULTS_OK' "$LOG"
echo "A1200 race finish/results flow passed"
