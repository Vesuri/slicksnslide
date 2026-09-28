#!/usr/bin/env bash
# Boot native Slicks on the target A1200 configuration, with audio enabled.
# Exit through the native menus to restore system state.
set -euo pipefail
cd "$(dirname "$0")"
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"

FSUAE="${FSUAE:-fs-uae}"
ROM="${1:-${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}}"
EXE="${SLICKS_EXE:-out/SlicksDiag.exe}"
SETPATCH="${SETPATCH:-../tmp/SetPatch}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
[ -f "$EXE" ] || { echo "not found: $EXE  (build first: make)"; exit 1; }
[ -f "$SETPATCH" ] || { echo "SetPatch not found: $SETPATCH"; exit 1; }

RUN="$FSUAE_RUN"; DH0="$RUN/dh0"; DH1="$RUN/dh1"
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
REGISTRATION_KEY="${SLICKS_REGISTRATION_KEY:-../tmp/slicks.rek}"
if [ -n "${SLICKS_REGISTRATION_KEY:-}" ] && [ ! -f "$REGISTRATION_KEY" ]; then
  echo 'Registration key file not found.' >&2; exit 2
fi
if [ -f "$REGISTRATION_KEY" ]; then cp -f "$REGISTRATION_KEY" "$DH1/SLICKS.REK"; fi
cp -f ../ref/SLICKS.000 "$DH1/SLICKS.000"
cp -f ../ref/SLICKS.DAT "$DH1/SLICKS.DAT"
mkdir -p "$DH1/TRACKS"
cp -f ../ref/TRACKS/*.SS "$DH1/TRACKS/"
rm -f "$RUN"/state/*.uss

fsuae_stop_previous
fsuae_track_self
exec "$FSUAE" \
  --amiga_model=A1200 --chip_memory=2048 --fast_memory=0 \
  --kickstart_file="$ROM" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=mouse --joystick_port_1=nothing \
  --automatic_input_grab=0 --fullscreen=0 --window_width=720 --window_height=568 \
  --ntsc_mode=0 --state_dir="$RUN/state"
