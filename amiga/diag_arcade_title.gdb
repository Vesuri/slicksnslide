set $starts = 0
set $options = 0
break slicks_diag_options_closed
commands
  silent
  set $options = $options+1
  continue
end
break slicks_race_start
commands
  silent
  set $starts = $starts+1
  if g_slicks_setup_session.players.count != 4 || setup_resources.override_count != 2 || g_slicks_setup_session.players.participation[0] != -1 || g_slicks_setup_session.players.participation[1] != -1 || g_slicks_setup_session.players.participation[2] != 1 || g_slicks_setup_session.players.participation[3] != 1
    printf "ARCADE_TITLE_PLAYER_HANDOFF_FAILED count=%u override=%d roles=%d/%d/%d/%d\n",g_slicks_setup_session.players.count,setup_resources.override_count,g_slicks_setup_session.players.participation[0],g_slicks_setup_session.players.participation[1],g_slicks_setup_session.players.participation[2],g_slicks_setup_session.players.participation[3]
    quit 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "ARCADE_TITLE counts=%u draws=%u checks=%lu errors=%lu options=%u starts=%u restore=%u\n",g_slicks_title_arcade_counts,g_slicks_title_arcade_draws,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors,$options,$starts,g_slicks_diag_restore_status
  if g_slicks_title_arcade_counts != 15 || !g_slicks_title_arcade_draws || g_slicks_title_dirty_errors || $options != 1 || $starts != 1 || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "ARCADE_TITLE_OPTIONS_PLAYERS_RACE_OK\n"
  quit
end
continue
quit 1
