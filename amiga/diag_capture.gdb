break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_CAPTURE_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  dump binary memory ../work/a1200-gameplay.logical g_slicks_diag_logical g_slicks_diag_logical+0x40000
  printf "SLICKS_CAPTURED FRAME=%u CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
