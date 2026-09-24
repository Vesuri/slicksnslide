set $warning = 0
if $_isvoid($expected_load)
  set $expected_load = 4
end
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || !$m->message || $m->track_lists || $m->picker || $m->name_dialog || $m->error || !g_slicks_diag_profile_platform->active
    printf "TRACK_LIST_WARNING_FAILED\n"
    quit 1
  end
  if g_slicks_track_lists_load.result != $expected_load || g_slicks_track_playlist.count != 1 || g_slicks_track_playlist.tracks[0] != 0
    printf "TRACK_LIST_RECOVERY_STATE_FAILED\n"
    quit 1
  end
  set $warning = $warning+1
  printf "TRACK_LIST_LOAD_WARNING result=%u path=%s\n", g_slicks_track_lists_load.result, g_slicks_track_lists_load.path
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $warning != 1 || g_slicks_diag_race_error || g_slicks_track_menu || g_slicks_track_playlist.count != 1 || g_slicks_track_playlist.tracks[0] != 0
      printf "TRACK_LIST_WARNING_RETURN_FAILED\n"
      quit 1
    end
    printf "TRACK_LIST_NATIVE_LOAD_WARNING_DISMISS_RACE_OK result=%u\n", $expected_load
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TRACK_LIST_WARNING_EARLY_EXIT\n"
  quit 1
end
continue
