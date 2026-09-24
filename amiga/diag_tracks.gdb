set $closed = 0
set $ready = 0
break slicks_diag_tracks_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error || $m->renderer.error || !g_slicks_diag_profile_platform->active
    printf "TRACKS_DRAW_FAILED\n"
    quit 1
  end
  set $ready = $ready+1
  if $ready == 1
    if g_slicks_track_playlist.count != g_slicks_diag_track_files
      printf "TRACKS_INITIAL_SELECTION_FAILED\n"
      quit 1
    end
    dump binary memory .run/tracks-v1/tracks-initial.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  if $closed == 1
    if g_slicks_track_playlist.count != 1 || g_slicks_track_playlist.tracks[0] != 0 || g_slicks_track_state.cursor != 0 || g_slicks_track_state.column
      printf "TRACKS_REOPEN_FAILED\n"
      quit 1
    end
    dump binary memory .run/tracks-v1/tracks.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    dump binary memory .run/tracks-v1/tracks.palette $m->palette $m->palette+768
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
      quit 1
    end
    dump binary memory .run/tracks-v1/tracks.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    printf "TRACKS_REOPEN_SELECTION_OK\n"
  end
  continue
end
break slicks_diag_tracks_closed
commands
  silent
  if g_slicks_track_menu || g_slicks_track_renderer.surface || g_slicks_track_playlist.count != 1 || g_slicks_track_playlist.tracks[0] != 0
    printf "TRACKS_CLOSE_FAILED\n"
    quit 1
  end
  set $closed = $closed+1
  continue
end
break prepare_race
commands
  silent
  printf "TRACKS_RACE_PATH %s\n", track_path
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $closed != 2 || $ready != 9 || g_slicks_diag_race_error
      printf "TRACKS_RACE_FAILED ready=%d closed=%d\n", $ready, $closed
      quit 1
    end
    printf "TRACKS_NATIVE_ENTRY_TOGGLE_RETURN_REOPEN_RACE_OK\n"
    quit
  end
  continue
end
continue
