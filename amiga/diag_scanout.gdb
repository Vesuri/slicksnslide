set $started = 0
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    set $initial_display = g_slicks_diag_display_checksum
    set $started = 1
    printf "SLICKS_SCANOUT_START DISPLAY=%08x\n", $initial_display
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  if !$started || !g_slicks_diag_scanout_only || g_slicks_diag_scanout_frames != 500 || g_slicks_diag_race_frame != 0 || g_slicks_diag_display_checksum != $initial_display
    printf "SLICKS_SCANOUT_FAILED\n"
    quit 1
  end
  printf "SLICKS_SCANOUT_UNCHANGED VBLANKS=500 DISPLAY=%08x\n", g_slicks_diag_display_checksum
  quit
end
continue
