set $closed = 0
set $confirm = 0
set $empty = 0
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if $m->error || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $m->message
    set $confirm = $confirm+1
    if $m->message->renderer.painter.font != &$m->fonts[2][0]
      quit 1
    end
  end
  if $m->track_lists
    if $m->track_lists->catalogue.count != ($closed < 3)
      printf "TRACK_LIST_DELETE_CATALOGUE_FAILED closed=%u\n", $closed
      quit 1
    end
    if $closed == 3
      set $empty = $empty+1
    end
  end
  continue
end
break slicks_diag_track_lists_closed
commands
  silent
  set $closed = $closed+1
  if g_slicks_track_menu->track_lists || g_slicks_track_menu->message || g_slicks_track_playlist.count != 1 || g_slicks_track_playlist.tracks[0] != 0
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $closed != 4 || $confirm != 2 || $empty != 1 || g_slicks_diag_race_error || g_slicks_track_lists_save.result
      printf "TRACK_LIST_DELETE_FAILED closes=%u confirmations=%u empty=%u\n", $closed, $confirm, $empty
      quit 1
    end
    printf "TRACK_LIST_NATIVE_CANCEL_NAME_CANCEL_DELETE_CONFIRM_REOPEN_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TRACK_LIST_DELETE_EARLY_EXIT\n"
  quit 1
end
continue
