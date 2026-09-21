#!/usr/bin/env bash
# Boot the strict target, enter the native BASIC.SS path, and verify its frame.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"
trap fsuae_stop_previous EXIT

EXPECTED='SLICKS_INGAME=1 ERROR=0 CHECKSUM=815c70ca DISPLAY=024f572b'
LOG=.run/diag-race.log
mkdir -p .run

SLICKS_AUTO_RACE=1 ./debug.sh "$KICKSTART" diag_ingame.gdb | tee "$LOG"
grep -Fqx "$EXPECTED" "$LOG"
echo "A1200 native race scene passed: $EXPECTED"
