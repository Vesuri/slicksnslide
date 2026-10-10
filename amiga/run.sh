#!/usr/bin/env bash
# Boot native Slicks on the target A1200 configuration, with audio enabled.
# Exit through the native menus to restore system state.
# Sound, window, warp and the rest: fsuae_options in the shared fsuae_common.sh.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "$FSUAE_COMMON"

ROM="${1:-$KICKSTART}"
EXE="${SLICKS_EXE:-out/SlicksDiag.exe}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
[ -f "$EXE" ] || { echo "not found: $EXE  (build first: make)"; exit 1; }
[ -f "$SETPATCH" ] || { echo "SetPatch not found: $SETPATCH"; exit 1; }

RUN="$FSUAE_RUN"; DH0="$RUN/dh0"; DH1="$RUN/dh1"
SCREENSHOT_DIR="${SCREENSHOT_DIR:-$PWD/../tmp/screenshots}"
mkdir -p "$SCREENSHOT_DIR"
mkdir -p "$DH0/c" "$DH0/s" "$DH1" "$RUN/state"
if [ "${SLICKS_AUTO_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag AUTO\n' > "$DH0/s/startup-sequence"
else
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag\n' > "$DH0/s/startup-sequence"
fi
cp -f "$SETPATCH" "$DH0/c/SetPatch"
cp -f "$EXE" "$DH1/SlicksDiag"
# Runtime-only personal key; never linked or packaged. Keep an installation's
# existing key if no local file was supplied.
REGISTRATION_KEY="${SLICKS_REGISTRATION_KEY:-../ref/slicks.rek}"
if [ -n "${SLICKS_REGISTRATION_KEY:-}" ] && [ ! -f "$REGISTRATION_KEY" ]; then
  echo 'Registration key file not found.' >&2; exit 2
fi
if [ -f "$REGISTRATION_KEY" ]; then cp -f "$REGISTRATION_KEY" "$DH1/SLICKS.REK"; fi
cp -f ../ref/SLICKS.000 "$DH1/SLICKS.000"
cp -f ../ref/SLICKS.DAT "$DH1/SLICKS.DAT"
mkdir -p "$DH1/TRACKS"
cp -f ../ref/TRACKS/*.SS "$DH1/TRACKS/"
rm -f "$RUN"/state/*.uss

AMIGA_MODEL=A1200 KICKSTART="$ROM" CHIP_KB=2048 FAST_KB=0 JOY0=mouse JOY1=nothing
fsuae_options
fsuae_stop_previous
fsuae_exec \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --window_width=720 --window_height=568 \
  --state_dir="$RUN/state" --screenshots_output_dir="$SCREENSHOT_DIR"
