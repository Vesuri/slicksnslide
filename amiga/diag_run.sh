#!/usr/bin/env bash
# Boot the target build and prove that its first converted frame was displayed.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_DIAG_READY=1 CHECKSUM=20255ca3 DISPLAY=52c2bd12'
LOG=.run/diag-ready.log
mkdir -p .run

./debug.sh "$KICKSTART" diag_ready.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 diagnostic passed: $EXPECTED"
