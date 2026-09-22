#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_RESTORE_OK STATUS=1f'
LOG=.run/diag-restore.log
mkdir -p .run

SLICKS_RESTORE_TEST=1 ./debug.sh "$KICKSTART" diag_restore.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 hardware state restore passed: $EXPECTED"
