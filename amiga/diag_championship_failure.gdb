set $warned = 0
set $returned = 0
break run_saved_game_dialog
commands
  silent
  dump binary memory .run/championship-failure-v1/session.before &g_slicks_setup_session &g_slicks_setup_session+1
  dump binary memory .run/championship-failure-v1/playlist.before &g_slicks_track_playlist &g_slicks_track_playlist+1
  continue
end
break championship_notice
commands
  silent
  set $warned = $warned+1
  printf "CHAMPIONSHIP_FAILURE_NOTICE %s\n",text
  if text[0] == 'N'
    printf "FAILURE_FIXTURE_NOT_VISIBLE\n"
    quit 1
  end
  continue
end
break prepare_race
commands
  silent
  printf "BAD_SAVE_REACHED_RACE_PREPARATION\n"
  quit 1
end
break redraw_title_configuration
commands
  silent
  if $warned
    dump binary memory .run/championship-failure-v1/session.after &g_slicks_setup_session &g_slicks_setup_session+1
    dump binary memory .run/championship-failure-v1/playlist.after &g_slicks_track_playlist &g_slicks_track_playlist+1
    set $returned = 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $warned != 1 || !$returned || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_REJECT_RETURN_EXIT_OK\n"
  quit
end
continue
