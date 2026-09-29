source diag_menu_rectangles.gdb
set $visits = 0
set $closed = 0
break *slicks_diag_tracks_closed
commands
  silent
  set $closed = $closed+1
  if g_slicks_track_menu || g_slicks_track_renderer.surface
    quit 1
  end
  continue
end
break *slicks_diag_tracks_ready
commands
  silent
  set $visits = $visits+1
  set $s = &g_slicks_track_state
  set $count = g_slicks_diag_track_files
  if !g_slicks_track_menu || g_slicks_track_menu->error || !g_slicks_diag_profile_platform->active || $count < 43 || $s->column
    printf "TRACK_SCROLL_OWNER_FAILED\n"
    quit 1
  end
  set $expected = 0
  if $visits == 2 || $visits == 4
    set $expected = 21
  end
  if $visits == 3
    set $expected = 42
  end
  if $visits == 6 || $visits == 7 || $visits == 10 || $visits == 11
    set $expected = $count-1
  end
  if $s->cursor != $expected || $s->top > $s->cursor || $s->cursor-$s->top >= 22 || g_slicks_track_playlist.count != $count
    printf "TRACK_SCROLL_STATE_FAILED visit=%d cursor=%d expected=%d top=%d\n", $visits, $s->cursor, $expected, $s->top
    quit 1
  end
  if $visits == 1
    dump binary memory .run/track-scroll-v1/before.playlist &g_slicks_track_playlist (&g_slicks_track_playlist)+1
  end
  printf "TRACK_SCROLL visit=%d cursor=%d top=%d\n", $visits, $s->cursor, $s->top
  if $visits == 11
    dump binary memory .run/track-scroll-v1/after.playlist &g_slicks_track_playlist (&g_slicks_track_playlist)+1
    if $closed != 1
      quit 1
    end
    printf "TRACK_SCROLL_BOUNDARIES_REOPEN_OK publications=%u\n", $menu_publications
    quit
  end
  continue
end
continue
