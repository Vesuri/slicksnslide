set $hit = 0
break slicks_diag_frame_ready
commands
  silent
  set $hit = $hit + 1
  printf "SLICKS_RACE_HIT=%u STAGE=%u INGAME=%u ERROR=%u\n", $hit, g_slicks_diag_race_stage, g_slicks_diag_ingame, g_slicks_diag_race_error
  if g_slicks_diag_ingame || g_slicks_diag_race_error
    printf "SLICKS_INGAME=%u ERROR=%u CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_ingame, g_slicks_diag_race_error, g_slicks_diag_checksum, g_slicks_diag_display_checksum
    quit
  end
  continue
end
continue
