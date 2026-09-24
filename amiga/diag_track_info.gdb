set $opened = 0
set $closed = 0
break slicks_diag_track_info_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || !$m->track_info || $m->message || $m->error || !g_slicks_diag_profile_platform->active
    printf "TRACK_INFO_OPEN_FAILED\n"
    quit 1
  end
  set $opened = $opened+1
  set $font0 = $m->track_info->font_colours[0]
  set $font1 = $m->track_info->font_colours[1]
  set $selection_count = g_slicks_track_playlist.count
  if $opened == 1
    dump binary memory .run/track-info-v1/before.chunky $m->track_info->saved $m->track_info->saved+64000
    dump binary memory .run/track-info-v1/info.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    dump binary memory .run/track-info-v1/info.palette $m->palette $m->palette+768
  end
  printf "TRACK_INFO_OPEN %u cursor=%d\n", $opened, g_slicks_track_state.cursor
  continue
end
break slicks_amiga_track_info_close
commands
  silent
  if m->track_info && m->track_info->updates < 256
    printf "TRACK_INFO_ANIMATION_OR_OPEN_FAILED phase=%u updates=%lu\n", m->track_info->phase, m->track_info->updates
    quit 1
  end
  continue
end
break slicks_diag_track_info_closed
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->track_info || !g_slicks_diag_profile_platform->active || $m->fonts[0][6] != $font0 || $m->fonts[1][6] != $font1 || g_slicks_track_playlist.count != $selection_count || g_slicks_track_state.cursor != 0
    quit 1
  end
  set $closed = $closed+1
  dump binary memory .run/track-info-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $opened != 2 || $closed != 2 || g_slicks_diag_race_error
      printf "TRACK_INFO_GATE_FAILED opened=%u closed=%u\n", $opened,$closed
      quit 1
    end
    printf "TRACK_INFO_NATIVE_OPEN_ANIMATE_CLOSE_REOPEN_RACE_OK\n"
    quit
  end
  continue
end
continue
