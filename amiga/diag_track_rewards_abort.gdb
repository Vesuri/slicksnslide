set $awards = 0
set $race = 0
break *award_race_track
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $awards || $race->race_complete || !$race->track_rewarded || $race->frame_count < 10 || $race->racing || $race->participation[0] != 1 || $race->participation[1] != 1 || $race->participation[2] != -1 || $race->participation[3]
    printf "EARLY_TRACK_REWARD_BOUNDARY_FAILED\n"
    quit 1
  end
  set $before = g_slicks_setup_session
  set $i = 0
  while $i < 4
    if $race->cars[$i].best_lap_time_units != 30000
      printf "EARLY_TRACK_BEST_LAP_FAILED driver=%u\n",$i
      quit 1
    end
    set $i = $i+1
  end
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  set $awards = $awards+1
  set $i = 0
  while $i < 4
    if g_slicks_setup_session.cash[$i] != (short)($before.cash[$i]+$before.options.field_302c) || g_slicks_setup_session.points[$i] != $before.points[$i]
      printf "EARLY_TRACK_REWARD_VALUES_FAILED\n"
      quit 1
    end
    set $i = $i+1
  end
  continue
end
break redraw_title_configuration
commands
  silent
  if $race
    if $awards != 1 || g_slicks_diag_race_error
      quit 1
    end
    printf "NATIVE_EARLY_EXIT_TRACK_PAYMENT_INACTIVE_SENTINEL_RETURN_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "EARLY_TRACK_REWARD_UNEXPECTED_EXIT\n"
  quit 1
end
continue
