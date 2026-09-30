set $failed = 0
set $dismissed = 0
set $opened = 0
set $closed = 0
set $resident_guard = 0
break slicks_amiga_platform_end
commands
  silent
  if $resident_guard
    printf "TRACK_INFO_CLOSE_UNEXPECTED_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $resident_guard
    printf "TRACK_INFO_CLOSE_UNEXPECTED_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
break open_track_info
commands
  silent
  if !$failed
    dump binary memory .run/track-info-faults-v1/before.chunky menu->renderer.ui.pixels menu->renderer.ui.pixels+64000
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
  set $resident_guard = 1
  set $m = g_slicks_track_menu
  if !$m || $m->error || !g_slicks_diag_profile_platform->active || g_slicks_diag_track_info_fault
    printf "TRACK_INFO_READY_INVALID menu=%p error=%d active=%u fault=%u stage=%u\n",$m,$m->error,g_slicks_diag_profile_platform->active,g_slicks_diag_track_info_fault,g_slicks_diag_track_info_stage
    printf "TRACK_INFO_ALLOCATION free=%lu largest=%lu required=%lu\n",g_slicks_diag_track_info_free,g_slicks_diag_track_info_largest,sizeof(struct SlicksAmigaTrackInfo)
    quit 1
  end
  if $failed < 5
    if $m->track_info || !$m->message || $failed != $dismissed
      printf "TRACK_INFO_FAULT_WARNING_FAILED\n"
      quit 1
    end
    set $failed = $failed+1
  else
    if $dismissed != 5 || !$m->track_info || $m->message
      printf "TRACK_INFO_REOPEN_FAILED dismissed=%u info=%p message=%p\n",$dismissed,$m->track_info,$m->message
      quit 1
    end
    set $opened = $opened+1
  end
  continue
end
break slicks_diag_track_info_warning_closed
commands
  silent
  if !$resident_guard || !g_slicks_diag_profile_platform->active
    printf "TRACK_INFO_WARNING_GUARD_FAILED guard=%u active=%u\n",$resident_guard,g_slicks_diag_profile_platform->active
    quit 1
  end
  set $resident_guard = 0
  set $m = g_slicks_track_menu
  if !$m || $m->message || $m->track_info || $m->fonts[0][6] != $font0 || $m->fonts[1][6] != $font1 || g_slicks_track_playlist.count != $count || g_slicks_track_state.cursor != $cursor
    printf "TRACK_INFO_FAULT_RESTORE_FAILED\n"
    quit 1
  end
  set $dismissed = $dismissed+1
  if $dismissed == 1
    dump binary memory .run/track-info-faults-v1/after1.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  if $dismissed == 2
    dump binary memory .run/track-info-faults-v1/after2.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  if $dismissed == 3
    dump binary memory .run/track-info-faults-v1/after3.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  if $dismissed == 4
    dump binary memory .run/track-info-faults-v1/after4.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  if $dismissed == 5
    dump binary memory .run/track-info-faults-v1/after5.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  continue
end
break slicks_diag_track_info_closed
commands
  silent
  if !$resident_guard || !g_slicks_diag_profile_platform->active
    printf "TRACK_INFO_CLOSE_GUARD_FAILED guard=%u active=%u\n",$resident_guard,g_slicks_diag_profile_platform->active
    quit 1
  end
  set $resident_guard = 0
  set $closed = $closed+1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $failed != 5 || $dismissed != 5 || $opened != 2 || $closed != 2 || g_slicks_diag_race_error
      printf "TRACK_INFO_FAULT_GATE_FAILED %u %u %u %u\n",$failed,$dismissed,$opened,$closed
      quit 1
    end
    printf "TRACK_INFO_FIVE_FAULTS_DISMISS_RETRY_REOPEN_RACE_OK\n"
    quit
  end
  continue
end
continue
