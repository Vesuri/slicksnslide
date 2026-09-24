set $failed = 0
set $dismissed = 0
set $opened = 0
set $closed = 0
break open_track_info
commands
  silent
  if !$failed
    if name[0] != 'R' || name[1] != 'A' || name[7] != 'D'
      quit 1
    end
    dump binary memory .run/track-info-failure-v1/before.chunky menu->renderer.ui.pixels menu->renderer.ui.pixels+64000
    set $font0 = menu->fonts[0][6]
    set $font1 = menu->fonts[1][6]
    set $count = g_slicks_track_playlist.count
    set $cursor = g_slicks_track_state.cursor
  end
  continue
end
break slicks_diag_track_info_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if !$failed
    if $m->track_info || !$m->message
      printf "TRACK_INFO_EXPECTED_WARNING_MISSING\n"
      quit 1
    end
    set $failed = 1
  else
    if !$dismissed || !$m->track_info || $m->message || g_slicks_track_state.cursor
      quit 1
    end
    set $opened = $opened+1
  end
  continue
end
break slicks_diag_track_info_warning_closed
commands
  silent
  set $m = g_slicks_track_menu
  if !$failed || !$m || $m->message || $m->track_info || $m->fonts[0][6] != $font0 || $m->fonts[1][6] != $font1 || g_slicks_track_playlist.count != $count || g_slicks_track_state.cursor != $cursor
    printf "TRACK_INFO_ERROR_RESTORE_FAILED\n"
    quit 1
  end
  dump binary memory .run/track-info-failure-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  set $dismissed = $dismissed+1
  continue
end
break slicks_diag_track_info_closed
commands
  silent
  set $closed = $closed+1
  if g_slicks_track_menu->track_info || g_slicks_track_menu->message
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $failed != 1 || $dismissed != 1 || $opened != 2 || $closed != 2 || g_slicks_diag_race_error
      printf "TRACK_INFO_FAILURE_GATE_FAILED %u %u %u %u\n", $failed,$dismissed,$opened,$closed
      quit 1
    end
    printf "TRACK_INFO_NATIVE_REJECTION_DISMISS_RETRY_REOPEN_RACE_OK\n"
    quit
  end
  continue
end
continue
