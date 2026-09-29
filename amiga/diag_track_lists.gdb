set $closed = 0
set $name = 0
source diag_track_lists_resident.gdb
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error || !g_slicks_diag_profile_platform->active
    printf "TRACK_LIST_MODAL_FAILED\n"
    quit 1
  end
  if $m->track_lists
    set $resident_guard = 1
  end
  if $m->name_dialog
    set $name = $name+1
    if $m->name_dialog->renderer.x != 185 || $m->name_dialog->renderer.y != 40
      quit 1
    end
  end
  continue
end
break slicks_diag_track_lists_closed
commands
  silent
  set $resident_guard = 0
  set $closed = $closed+1
  if g_slicks_track_menu->track_lists || g_slicks_track_menu->message || g_slicks_track_playlist.count != 2 || g_slicks_track_playlist.tracks[0] != 0 || g_slicks_track_playlist.tracks[1] != 1
    printf "TRACK_LIST_SELECTION_FAILED count=%d\n", g_slicks_track_playlist.count
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $closed != 1 || g_slicks_diag_race_error || g_slicks_track_lists_load.result || g_slicks_track_lists_save.result
      printf "TRACK_LIST_RACE_FAILED\n"
      quit 1
    end
    printf "TRACK_LIST_NATIVE_SELECTION_RACE_OK name_frames=%u disk_commits=%u\n", $name,$commits
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TRACK_LIST_EARLY_EXIT\n"
  quit 1
end
continue
