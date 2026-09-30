# STARTGO, STARTF9 or STARTF9[ROW]: ordinary input, no automatic race launch.
set $starts=0
break slicks_race_start
commands
  silent
  set $starts=$starts+1
  set $started_race=race
  if title_demo.active || g_slicks_setup_session.players.count<1
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $starts!=1 || g_slicks_diag_race_error || title_demo.active || g_slicks_options_menu || g_slicks_track_menu || g_slicks_player_menu || $started_race->race_mode!=title_configuration->options[0]
      quit 1
    end
    if g_slicks_diag_title_start_row!=65535 && g_slicks_diag_title_start_seen!=g_slicks_diag_title_start_row
      printf "TITLE_INPUT_WRONG_START_ROW expected=%u actual=%u\n",g_slicks_diag_title_start_row,g_slicks_diag_title_start_seen
      quit 1
    end
    printf "TITLE_INPUT_RACE_START_OK mode=%d row=%u\n",$started_race->race_mode,g_slicks_diag_title_start_seen
    detach
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TITLE_INPUT_EARLY_EXIT error=%u\n",g_slicks_diag_race_error
  quit 1
end
continue
