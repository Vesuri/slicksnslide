#!/usr/bin/env bash
# Native build regression. Close all sessions using out/SlicksDiag.elf first.
# Leaves the normal (INNER_PROFILE=0) executable ready on success.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
if pgrep -f 'fs-uae.*--remote_debugger_trigger=SlicksDiag' >/dev/null; then
  echo 'Close Slicks debug sessions before checking build modes.' >&2
  exit 2
fi
make INNER_PROFILE=1
make INNER_PROFILE=1 -q obj/race_runtime.o
profile_object_hash=$(shasum obj/race_runtime.o)
# A failed switch must not certify the old object as the new mode.
if make INNER_PROFILE=0 CC=false obj/race_runtime.o; then
  echo 'Expected compiler failure did not occur.' >&2
  exit 1
fi
test "$profile_object_hash" = "$(shasum obj/race_runtime.o)"
test -f obj/inner-profile-1.stamp
test ! -e obj/inner-profile-0.stamp
make INNER_PROFILE=1 -q obj/race_runtime.o
# Rapid switches must work even on one-second timestamp resolution.
for profile_mode in 0 1 0; do
  make INNER_PROFILE="$profile_mode"
  make INNER_PROFILE="$profile_mode" -q obj/race_runtime.o
  test -f "obj/inner-profile-$profile_mode.stamp"
done
printf 'INNER_PROFILE_FAILURE_RECOVERY_AND_SWITCHES_OK\n'
