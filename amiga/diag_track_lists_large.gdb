set $opened = 0
set $ends = 0
set $closed = 0
break slicks_amiga_track_lists_picker
commands
  silent
  set $opened = $opened+1
  if $opened == 1
    set $pixels = g_slicks_track_menu->renderer.ui.pixels
    dump binary memory .run/track-lists-large-v1/before.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error || $m->message || g_slicks_track_lists_load.result
    printf "LARGE_LIST_OPEN_FAILED\n"
    quit 1
  end
  if $m->picker
    set $p = $m->picker
    if $p->renderer.state.count != 2848 || $p->owned_names_size != 59808 || !$p->scrollbar_saved.pixels
      printf "LARGE_LIST_STORAGE_FAILED\n"
      quit 1
    end
    if $p->renderer.state.selected == 2847
      set $ends = $ends+1
      printf "LARGE_LIST_END title=%s\n", $p->owned_names+2847*21
    end
  end
  continue
end
break slicks_diag_track_lists_closed
commands
  silent
  set $closed = $closed+1
  if g_slicks_track_menu->picker || g_slicks_track_menu->track_lists || g_slicks_track_menu->message
    quit 1
  end
  if $closed == 1
    set $pixels = g_slicks_track_menu->renderer.ui.pixels
    dump binary memory .run/track-lists-large-v1/after.chunky $pixels $pixels+64000
  end
  if $closed == 2
    if g_slicks_track_playlist.count != 2 || g_slicks_track_playlist.tracks[0] != 0 || g_slicks_track_playlist.tracks[1] <= 0
      printf "LARGE_LIST_LOAD_FAILED\n"
      quit 1
    end
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $opened != 2 || $ends != 2 || $closed != 2 || g_slicks_diag_race_error
      printf "LARGE_LIST_RACE_FAILED opened=%u ends=%u closed=%u\n", $opened,$ends,$closed
      quit 1
    end
    printf "LARGE_LIST_CANCEL_REOPEN_END_LOAD_RACE_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "LARGE_LIST_EARLY_EXIT\n"
  quit 1
end
continue
