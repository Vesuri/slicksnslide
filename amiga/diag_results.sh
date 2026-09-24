#!/usr/bin/env bash
# Prove that a one-lap race orders every finisher and draws the result panel.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

LOG=.run/diag-results.log
EXPECTED='SLICKS_RESULTS_OK FRAME=618 POS=2,3,4,1 CHECKSUM=2f26e8ea DISPLAY=5ee09abf'
mkdir -p .run
SLICKS_AUTO_RACE=1 SLICKS_RESULTS_RACE=1 \
  ./debug.sh "$KICKSTART" diag_results.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 race finish/results flow passed: $EXPECTED"
