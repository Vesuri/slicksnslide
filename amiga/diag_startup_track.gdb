set $display_name = 0
set $loaded = 0
break redraw_title_configuration
commands
  silent
  if !$display_name
    if g_slicks_diag_track_files < 2 || g_slicks_track_playlist.count != g_slicks_diag_track_files
      printf "STARTUP_TITLE_BEFORE_TRACK_SELECTION\n"
      quit 1
    end
    set $display_name = track_name
    printf "STARTUP_TITLE_TRACK %s\n", $display_name
  end
  continue
end
break prepare_race
commands
  silent
  if !$display_name
    quit 1
  end
  set $i = 0
  while $display_name[$i]
    if track_path[7+$i] != $display_name[$i]
      printf "STARTUP_TITLE_RACE_TRACK_MISMATCH\n"
      quit 1
    end
    set $i = $i+1
  end
  if track_path[7+$i]
    quit 1
  end
  set $loaded = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$loaded || g_slicks_diag_race_error
      quit 1
    end
    printf "NATIVE_STARTUP_TITLE_IMMEDIATE_GO_TRACK_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "STARTUP_TRACK_EARLY_EXIT\n"
  quit 1
end
continue
