#!/usr/bin/env bash
# Muted outer benchmarks, one track after another, on the current build.
#   ./bench_tracks.sh LABEL [DETAIL] [TRACKS...]   (tracks 0..3, default all)
# Builds first and refuses to measure a stale executable. Logs go to
# ../tmp/LABEL-TRACK.log; one summary line per track is printed. Compare
# WORK_SUM and FINAL_STATE with a control run of the parent commit.
set -euo pipefail
cd "$(dirname "$0")"
label="${1:?label}"; shift
detail="${1:-0}"; [ $# -gt 0 ] && shift
tracks="${*:-0 1 2 3}"
. ./env.sh
case "$detail" in 0|8) inner_default=0;; 1|2|3|4|5|6|7) inner_default=1;; *) echo 'Invalid detail mode' >&2; exit 2;; esac
# Explicit override permits a matched instrumented control of outer timing.
inner="${INNER_PROFILE:-$inner_default}"
case "$inner" in 0|1) ;; *) echo 'INNER_PROFILE must be 0 or 1' >&2; exit 2;; esac
if [ "$inner_default" = 1 ] && [ "$inner" != 1 ]; then
  echo 'Detailed benchmarks require INNER_PROFILE=1.' >&2; exit 2
fi
make INNER_PROFILE="$inner" -q 2>/dev/null || make INNER_PROFILE="$inner" >/dev/null 2>&1 || { echo "build failed"; exit 1; }
for t in $tracks; do
  log="../tmp/$label-$t.log"
  FSUAE_RUN="$PWD/.run/bench-$label-$t" DEBUG_PORT="${DEBUG_PORT:-2394}" \
    SLICKS_GAMEPLAY_BENCHMARK=$t SLICKS_BENCHMARK_DETAIL=$detail \
    ./debug.sh "" diag_benchmark.gdb < /dev/null > "$log" 2>&1
  rm -rf ".run/bench-$label-$t"
  sum=$(grep -o 'WORK_SUM=[0-9]*' "$log" | cut -d= -f2)
  max=$(grep -o 'max_work_lines=[0-9]*' "$log" | cut -d= -f2)
  cad=$(grep -o 'cadence_lines=[0-9]*' "$log" | cut -d= -f2)
  fin=$(grep '^FINAL_STATE' "$log")
  printf '%s track=%s work=%s max=%s cadence=%s %s\n' "$label" "$t" "$sum" "$max" "$cad" "$fin"
done
