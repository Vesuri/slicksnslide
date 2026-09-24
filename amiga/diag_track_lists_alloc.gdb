set $warnings = 0
set $dismissed = 0
set $ends = 0
set $attempts = 0
break slicks_amiga_track_lists_picker
commands
  silent
  set $attempts = $attempts+1
  if $attempts <= 2 && g_slicks_diag_list_alloc_fault != $attempts
    printf "LIST_ALLOCATION_FAULT_NOT_ARMED attempt=%u\n", $attempts
    quit 1
  end
  if $attempts == 1
    set $pixels = g_slicks_track_menu->renderer.ui.pixels
    dump binary memory .run/track-lists-alloc-v1/before.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error
    quit 1
  end
  if $m->message
    set $warnings = $warnings+1
    if $warnings > 2 || $m->track_lists || $m->picker || g_slicks_track_lists_load.result != 1 || g_slicks_track_lists_load.io_error != 103 || g_slicks_diag_list_alloc_fault
      printf "LIST_ALLOCATION_REPORT_FAILED\n"
      quit 1
    end
  end
  if $m->picker && $m->picker->renderer.state.selected == 2847
    set $ends = $ends+1
  end
  continue
end
break slicks_diag_track_info_warning_closed
commands
  silent
  set $dismissed = $dismissed+1
  set $m = g_slicks_track_menu
  if !$m || $m->message || $m->picker || $m->track_lists || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $pixels = $m->renderer.ui.pixels
  if $dismissed == 1
    dump binary memory .run/track-lists-alloc-v1/after1.chunky $pixels $pixels+64000
  else
    dump binary memory .run/track-lists-alloc-v1/after2.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $warnings != 2 || $dismissed != 2 || $ends != 1 || g_slicks_diag_race_error || g_slicks_track_lists_load.result || g_slicks_track_playlist.count != 2
      printf "LIST_ALLOCATION_RETRY_FAILED warnings=%u dismissed=%u ends=%u\n", $warnings,$dismissed,$ends
      quit 1
    end
    printf "LIST_TWO_ALLOCATION_FAILURES_DISMISS_RETRY_RACE_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "LIST_ALLOCATION_EARLY_EXIT\n"
  quit 1
end
continue
