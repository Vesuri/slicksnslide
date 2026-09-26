#!/usr/bin/env bash
# Source-level debugging through FS-UAE's single-client GDB stub.
set -euo pipefail
cd "$(dirname "$0")"
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"

FSUAE="${FSUAE:-fs-uae}"
GDB="${GDB:-m68k-amiga-elf-gdb}"
# Match Revs: silence host output without changing emulated audio/DMA.
# This FS-UAE build does not implement the volume=0 path.
# Opt in only when audible output is needed for a sound investigation.
AUDIO_ARGS=(--audio_driver=dummy)
if [ "${FSUAE_SOUND:-0}" = 1 ]; then AUDIO_ARGS=(); fi
# Installed FS-UAE uses SDL2. Record its actual emulated Paula output through
# SDL's disk backend, never the host speakers. SDL3 uses different variables.
if [ -n "${SLICKS_AUDIO_CAPTURE_FILE:-}" ]; then
  case "$SLICKS_AUDIO_CAPTURE_FILE" in /*) ;; *)
    echo 'SLICKS_AUDIO_CAPTURE_FILE must be an absolute path' >&2; exit 2;; esac
  [ ! -e "$SLICKS_AUDIO_CAPTURE_FILE" ] || {
    echo "Audio capture already exists: $SLICKS_AUDIO_CAPTURE_FILE" >&2; exit 2;
  }
  export SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$SLICKS_AUDIO_CAPTURE_FILE"
  AUDIO_ARGS=(--audio_driver=sdl)
fi
LUA_ARGS=()
if [ -n "${SLICKS_DEBUG_LUA:-}" ]; then LUA_ARGS=("--uae_lua=$SLICKS_DEBUG_LUA"); fi
DEBUG_JOYSTICK=nothing
if [ "${SLICKS_DEBUG_JOYSTICK_KEYS:-0}" = 1 ]; then
  # The installed FSEMU build ignores legacy keyboard_key_* overrides.
  # Use its built-in keyboard joystick: arrows and right Ctrl/Alt fire.
  DEBUG_JOYSTICK=keyboard
fi
ROM="${1:-${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}}"
SETPATCH="${SETPATCH:-../tmp/SetPatch}"
DEBUG_BUILD="${SLICKS_DEBUG_BUILD:-out/SlicksDiag}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
[ -f "$DEBUG_BUILD.elf" ] || { echo "build first: make"; exit 1; }
[ -f "$SETPATCH" ] || { echo "SetPatch not found: $SETPATCH"; exit 1; }

RUN="$FSUAE_RUN"; DH0="$RUN/dh0"; DH1="$RUN/dh1"; GDBHOME="$RUN/gdbhome"
mkdir -p "$DH0/c" "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
if [ -n "${SLICKS_MODE_TRANSITION:-}" ]; then
  case "$SLICKS_MODE_TRANSITION" in 0|1|2|3|4|5) ;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONST%s\n' "$SLICKS_MODE_TRANSITION" > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_PITCH_TEST:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag AUDIOFALL\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_PITCH_TEST:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag AUDIOHIGH\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_PITCH_TEST:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag AUDIOPITCH\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_ACTOR_TEST:-0}" = 1 ]; then
  actor_case="${SLICKS_TRACK_ACTOR_CASE:-0}"
  case "$actor_case" in 0|1|2|3) ;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALO%s\n' "$actor_case" > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_PCM_TEST:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALQB\n' > "$DH0/s/startup-sequence"
elif [ -n "${SLICKS_TRAJECTORY:-}" ]; then
  case "$SLICKS_TRAJECTORY" in mixed) trajectory=T;; identical) trajectory=I;; pc-audio) trajectory=Q;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURAL%s\n' "$trajectory" > "$DH0/s/startup-sequence"
elif [ -n "${SLICKS_WEAPON_TRANSITION:-}" ]; then
  case "$SLICKS_WEAPON_TRANSITION" in P|R|E|C|A) ;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURAL%s\n' "$SLICKS_WEAPON_TRANSITION" > "$DH0/s/startup-sequence"
elif [ -n "${SLICKS_WEAPON_CASE:-}" ]; then
  case "$SLICKS_WEAPON_CASE" in 1|2|3|4|5|6|7|8|9) ;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALW%s\n' "$SLICKS_WEAPON_CASE" > "$DH0/s/startup-sequence"
elif [ "${SLICKS_NATURAL_RESULTS:-}" = shop ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALW\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_NATURAL_RESULTS:-}" = damage ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_NATURAL_RESULTS:-}" = fuel ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NATURALF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_RECORD_RECOVERY:-}" = retry ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSBR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_RECORD_RECOVERY:-}" = skip ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSBS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_RECORD_RECOVERY:-}" = read-skip ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSBL\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_CHAMPIONSHIP:-}" = save ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CHAMPSAVE\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_CHAMPIONSHIP:-}" = weapons ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CHAMPLOADW\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_CHAMPIONSHIP:-}" = load ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CHAMPLOAD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_CHAMPIONSHIP:-}" = edit ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CHAMPEDIT\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_CHAMPIONSHIP:-}" = fail ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CHAMPFAIL\n' > "$DH0/s/startup-sequence"
elif [ -n "${SLICKS_PROFILE_DIALOG_FAILURE:-}" ]; then
  case "$SLICKS_PROFILE_DIALOG_FAILURE" in NF|NL|CF|CL) ;; *) exit 2;; esac
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERS%s\n' "$SLICKS_PROFILE_DIALOG_FAILURE" > "$DH0/s/startup-sequence"
elif [ "${SLICKS_INTERMISSION_LIVE:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSTJ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_INTERMISSION_LIVE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSTI\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_INTERMISSION_SURFACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag UIMENU2\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PAUSE_LIVE:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag LIVEMENUN\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PAUSE_LIVE:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag LIVEMENUF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PAUSE_LIVE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag LIVEMENU\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PAUSE_SURFACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag UIMENU\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 10 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSN\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 9 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSM\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 8 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSK\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 7 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSJ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 6 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSI\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 5 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 4 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKSL\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_MENU:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACKS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 8 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSN\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 7 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSM\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 6 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag HELPF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 5 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSL\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 4 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag HELP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSK\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSJ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_HELP_MENU:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSH\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 16 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 15 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSE\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 14 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 13 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 12 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONST\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 11 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSU\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 10 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSQ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 9 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSZ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 8 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSB\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 7 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSA\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 6 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSW\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 5 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSV\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 4 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONSC\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_OPTIONS_MENU:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag OPTIONS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 18 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSB\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 17 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSZ\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 16 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSY\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 15 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSX\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 14 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSH\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 13 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSW\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 12 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSV\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 11 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSU\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 10 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERST\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 9 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 8 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSC\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 7 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSN\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 6 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSE\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 5 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 4 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSG\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSK\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERSR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_PLAYER_MENU:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PLAYERS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_ABORT:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUPA\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_FAILURE:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUPG\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_FAILURE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUPF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_RELOAD:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUPR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_INPUT:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUPI\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SETUP_SESSION:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SETUP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_WEAPON_HUD:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag WEAPONHUD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_DAMAGE_RACE:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CONFIGDR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_FUEL_RACE:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag FUELR\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_DAMAGE_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CONFIGD\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SERVICE_MENU:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag CONFIG\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_FUEL_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag FUEL\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_JUMP_TRACK:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag UPPERJUMP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SHADOW_TEST:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag JUMP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_BENCHMARK:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag MEASURE\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_IN_BLANK:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag VBLANKAUDIO\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUDIO_NO_DMA:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag DMAOFF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_NO_AUDIO:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag NOAUDIO\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_NO_PARTICLES:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag PARTICLESOFF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SCANOUT_ONLY:-0}" = 3 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SCANOUTF\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SCANOUT_ONLY:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SCANOUTC\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_SCANOUT_ONLY:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag SCANOUT\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_BITMAP_AUDIT:-0}" = 2 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag BITMAPFAULT\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_BITMAP_AUDIT:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag BITMAPAUDIT\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_RESTORE_TEST:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag EXIT\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_RESULTS_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag RESULTS\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_TRACK_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag TRACK\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_ICE_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag ICE\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_ZONE_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag HIGHZONES\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_LAP_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag LAP\n' > "$DH0/s/startup-sequence"
elif [ "${SLICKS_AUTO_RACE:-0}" = 1 ]; then
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag AUTO\n' > "$DH0/s/startup-sequence"
else
  printf 'C:SetPatch QUIET\ncd dh1:\nSlicksDiag\n' > "$DH0/s/startup-sequence"
fi
cp -f "$SETPATCH" "$DH0/c/SetPatch"
cp -f "$DEBUG_BUILD.exe" "$DH1/SlicksDiag"
cp -f ../ref/SLICKS.000 "$DH1/SLICKS.000"
cp -f ../ref/SLICKS.DAT "$DH1/SLICKS.DAT"
mkdir -p "$DH1/TRACKS"
cp -f ../ref/TRACKS/*.SS "$DH1/TRACKS/"

fsuae_claim_port
"$FSUAE" \
  "${AUDIO_ARGS[@]}" \
  "${LUA_ARGS[@]}" \
  --amiga_model=A1200 --chip_memory=2048 --fast_memory=0 \
  --kickstart_file="$ROM" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --hard_drive_1_read_only="${SLICKS_DEBUG_READ_ONLY:-0}" \
  --joystick_port_0=mouse --joystick_port_1="$DEBUG_JOYSTICK" \
  --automatic_input_grab=0 --fullscreen=0 --window_width=720 --window_height=568 \
  --remote_debugger=20 --remote_debugger_port="$DEBUG_PORT" \
  --remote_debugger_trigger=SlicksDiag \
  --ntsc_mode=0 --state_dir="$RUN/state" > "$RUN/fsuae-dbg.log" 2>&1 &
FSUAE_PID=$!
fsuae_track "$FSUAE_PID"
# Scripted tests own their emulator only until the debugger exits, whether
# the check passes or fails. Keep the shell alive so the EXIT trap can run.
if [ "${2:-}" ] && [ -f "$2" ]; then trap fsuae_stop EXIT; fi

for _ in $(seq 1 60); do
  kill -0 "$FSUAE_PID" 2>/dev/null || {
    echo "FS-UAE exited early; see $RUN/fsuae-dbg.log"; exit 1;
  }
  lsof -nP -iTCP:"$DEBUG_PORT" -sTCP:LISTEN >/dev/null 2>&1 && break
  sleep 1
done

PREAMBLE="$RUN/connect.gdb"
{
  printf 'set pagination off\nset confirm off\nset remotetimeout 90\n'
  printf 'target remote 127.0.0.1:%s\n' "$DEBUG_PORT"
  if [ -n "${SLICKS_WEAPON_CASE:-}" ]; then
    case "$SLICKS_WEAPON_CASE" in 1|2|3|4|5|6|7|8|9) ;; *) exit 2;; esac
    printf 'set $weapon_case = %s\n' "$SLICKS_WEAPON_CASE"
  fi
  printf 'echo \\n>>> connected. `continue` runs; Ctrl-C breaks in. <<<\\n\n'
} > "$PREAMBLE"

if [ "${2:-}" ] && [ -f "$2" ]; then
  env HOME="$GDBHOME" XDG_CACHE_HOME="$GDBHOME" \
    "$GDB" -q -l 10 -x "$PREAMBLE" -x "$2" "$DEBUG_BUILD.elf"
  exit $?
fi
exec env HOME="$GDBHOME" XDG_CACHE_HOME="$GDBHOME" \
  "$GDB" -q -l 10 -x "$PREAMBLE" "$DEBUG_BUILD.elf"
