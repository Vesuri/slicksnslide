#!/usr/bin/env bash
# Target-side PC profile of one native benchmark race (muted, no mid-race
# debugger stops). Usage: . ./env.sh; ./pc_profile.sh TRACK LABEL
# Writes ../tmp/pcprof-LABEL.{log,bin}; analyse with ../tools/prof_summary.py.
set -uo pipefail
cd "$(dirname "$0")"
TRACK="${1:?track 0..3}"; LABEL="${2:?label}"
RUN="$PWD/.run/pcprof-$LABEL"
rm -f ../tmp/pc-samples-latest.bin
FSUAE_RUN="$RUN" DEBUG_PORT="${DEBUG_PORT:-2394}" SLICKS_GAMEPLAY_BENCHMARK="$TRACK" \
  SLICKS_BENCHMARK_DETAIL=8 ./debug.sh "" diag_pc_sample.gdb > "../tmp/pcprof-$LABEL.log" 2>&1
status=$?
rm -rf "$RUN"
[ -f ../tmp/pc-samples-latest.bin ] && mv ../tmp/pc-samples-latest.bin "../tmp/pcprof-$LABEL.bin"
cp -f out/SlicksDiag.elf "../tmp/pcprof-$LABEL.elf"   # symbols for this exact layout
grep -E '^(BENCHMARK|WORK_SUM|PC_SAMPLES|FINAL_STATE)' "../tmp/pcprof-$LABEL.log"
exit $status
