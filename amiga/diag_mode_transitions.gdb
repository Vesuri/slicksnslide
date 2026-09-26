# SLICKS_MODE_TRANSITION=0..5: menu-selected mode, two races, native exit.
# All debugger assignments are host convenience variables, never target writes.
set $races=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  if $race->race_mode!=g_slicks_diag_mode_case || !g_slicks_diag_audit_bitmap
    printf "MODE_TRANSITION_WRONG_MODE actual=%d expected=%u\n",$race->race_mode,g_slicks_diag_mode_case
    quit 1
  end
  set $races=$races+1
  printf "MODE_TRANSITION_START mode=%u race=%u\n",g_slicks_diag_mode_case,$races
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "MODE_TRANSITION_BITMAP_FAILED\n"
  quit 1
end
break slicks_diag_collision_failed
commands
  silent
  printf "MODE_TRANSITION_COLLISION_FAILED\n"
  quit 1
end
# Reuse the audio lifecycle and native restoration assertions.
source diag_audio_restart.gdb
