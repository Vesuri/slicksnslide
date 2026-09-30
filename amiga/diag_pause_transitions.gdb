set $loads = 0
set $starts = 0
set $awards = 0
set $menus = 0
set $closed = 0
set $returned = 0
set $intermissions = 0
set $retries = 0
break slicks_diag_intermission_checkpoint
commands
  silent
  set $intermissions = $intermissions+1
  set $im = g_slicks_diag_intermission_menu
  if $intermissions != 1 || !$im || !$im->intermission || $im->intermission->state.selected != 2 || $im->intermission->content.track_index != 0 || $im->intermission->content.track_total != 2
    quit 1
  end
  printf "LIVE_INTERMISSION_NEXT_TRACK %s labels=%s / %s\n", $im->intermission->content.track_name, $im->intermission->content.labels[2], $im->intermission->content.labels[3]
  if $retries
    dump binary memory .run/pause-transitions-v1/retry.after &g_slicks_setup_session (char *)&g_slicks_setup_session+sizeof(g_slicks_setup_session)
  end
  continue
end
break prepare_race
commands
  silent
  set $loads = $loads+1
  if !session || $loads > 2 || new_game != ($loads == 1) || g_slicks_track_playlist.count != 2 || g_slicks_track_state.random_order
    printf "PAUSE_TRANSITION_LOAD_STATE_FAILED load=%u session=%p new=%u count=%u\n",$loads,session,new_game,g_slicks_track_playlist.count
    quit 1
  end
  if track_path[7] != 'B' || track_path[8] != 'A' || track_path[9] != 'S' || track_path[10] != 'I' || track_path[11] != 'C' || ($loads == 1 && track_path[12] != '.') || ($loads == 2 && track_path[12] != 'T')
    printf "PAUSE_TRANSITION_LOAD_PATH_FAILED load=%u path=%s\n",$loads,track_path
    quit 1
  end
  if $loads == 2
    if $awards != 1 || $closed != 1 || $starts != 1
      quit 1
    end
    set $d = 0
    while $d < 4
      set $profile = session->players.selected[$d]
      if $profile > 0 && g_slicks_profiles.setup[$profile].vehicle >= 11
        set $seed = ($seed*0x015a4e35+1)&0xffffffff
      end
      set $d = $d+1
    end
    if session->random_state != $seed
      printf "PAUSE_SKIP_SELECTION_REFRESH_NOT_ONCE\n"
      quit 1
    end
  end
  printf "PAUSE_TRANSITION_LOAD %u %s\n", $loads, track_path
  continue
end
break *slicks_race_start
commands
  silent
  set $starts = $starts+1
  if $starts != $loads || $starts > 2
    quit 1
  end
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  set $menus = $menus+1
  set $m = g_slicks_diag_pause_menu
  set $r = g_slicks_diag_paused_race
  if $menus != $starts || !$m || !$r || $m->race_menu->state.row != $menus+3 || $r->race_complete || $r->frame_count != 100
    printf "PAUSE_TRANSITION_MENU_FAILED menus=%u starts=%u menu=%p race=%p\n", $menus, $starts, $m, $r
    if $m && $r
      printf "row=%u complete=%u frame=%u\n", $m->race_menu->state.row, $r->race_complete, $r->frame_count
    end
    quit 1
  end
  set $clock = $r->game_clock_ticks
  set $seed = $r->random_state
  eval "dump binary memory .run/pause-transitions-v1/before%u.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)", $menus
  eval "dump binary memory .run/pause-transitions-v1/before%u.chunky $m->saved $m->saved+64000", $menus
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  set $closed = $closed+1
  if $closed != $menus || $m->race_menu || $r->game_clock_ticks != $clock || $r->random_state != $seed || $r->track_rewarded
    quit 1
  end
  eval "dump binary memory .run/pause-transitions-v1/after%u.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)", $menus
  eval "dump binary memory .run/pause-transitions-v1/after%u.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000", $menus
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  set $awards = $awards+1
  if $awards != $starts || $awards != $closed || $awards > 2
    printf "PAUSE_TRANSITION_REWARD_NOT_ONCE\n"
    quit 1
  end
  continue
end
break redraw_title_configuration
commands
  silent
  if $awards
    if $awards != 2 || $starts != 2 || $loads != 2 || $closed != 2 || g_slicks_diag_race_error
      quit 1
    end
    set $returned = 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$_isvoid($expected_retries) && $retries!=$expected_retries
    printf "INTERMISSION_RETRY_COUNT_FAILED actual=%u expected=%u\n",$retries,$expected_retries
    quit 1
  end
  if !$returned || $intermissions != 1 || g_slicks_diag_restore_status != 0x1f
    printf "PAUSE_TRANSITION_FAILED\n"
    quit 1
  end
  printf "NATIVE_PAUSE_SKIP_END_TWO_RACES_REWARDS_ONCE_OK\n"
  quit
end
continue
