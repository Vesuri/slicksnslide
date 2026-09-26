#!/usr/bin/env bash
# Build SHADOW=1, run native/reference dual execution (warp, muted), then
# rebuild the normal executable. Cases are benchmark tracks 0..3 or
# VAR=VALUE debug.sh fixtures (e.g. SLICKS_JUMP_TRACK=1).
#   . ./env.sh; ./shadow_check.sh LABEL [CASES...]
set -uo pipefail
cd "$(dirname "$0")"
LABEL="${1:?label}"; shift
CASES="${*:-0 1 2 3}"
rm -f obj/race_runtime.o obj/slicks_diag.o
make SHADOW=1 >/dev/null || exit 1
status=0
for c in $CASES; do
  name=$(printf '%s' "$c" | tr -c 'A-Za-z0-9' '_')
  log="../tmp/shadow-$LABEL-$name.log"
  case "$c" in
    [0-3]) set -- SLICKS_SHADOW_TRACK="$c";;
    *=*) set -- "$c";;
    *) echo "bad case $c"; exit 2;;
  esac
  env "$@" FSUAE_RUN="$PWD/.run/shadow-$LABEL-$name" DEBUG_PORT="${DEBUG_PORT:-2394}" \
    SLICKS_DEBUG_WARP=1 ./debug.sh "" diag_shadow_check.gdb < /dev/null > "$log" 2>&1 || status=1
  rm -rf ".run/shadow-$LABEL-$name"
  echo "$c:"; grep -E '^(SHADOW|RACE_ERROR|FINAL_STATE|DIFF|TRACK_HITS)' "$log" | head -20
  grep -q '^SHADOW calls' "$log" || status=1
done
rm -f obj/race_runtime.o obj/slicks_diag.o
make >/dev/null || exit 1
exit $status
