set $loads = 0
set $awards = 0
set $pending = 0
set $race = 0
break prepare_race
commands
  silent
  set $loads = $loads+1
  if !session || $loads > 2 || new_game != ($loads == 1)
    quit 1
  end
  if $loads == 2
    set $i = 0
    while $i < 4
      if session->cash[$i] != $after.cash[$i] || session->points[$i] != $after.points[$i]
        printf "TRACK_REWARDS_LOST_BETWEEN_RACES\n"
        quit 1
      end
      set $i = $i+1
    end
  end
  continue
end
break *award_race_track
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $pending || !$race->race_complete || !$race->track_rewarded || $awards != $loads-1
    printf "TRACK_REWARD_BOUNDARY_FAILED\n"
    quit 1
  end
  set $before = g_slicks_setup_session
  set $best = (short)29999
  set $i = 0
  while $i < 4
    if (int)$best > (int)$race->cars[$i].best_lap_time_units
      set $best = (short)$race->cars[$i].best_lap_time_units
    end
    set $i = $i+1
  end
  set $pending = 1
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  if !$pending
    quit 1
  end
  set $i = 0
  while $i < 4
    set $fast = ((int)$best == (int)$race->cars[$i].best_lap_time_units)
    set $cash = (short)($before.cash[$i]+$before.options.field_302c+$fast*$before.options.field_302e)
    set $points = (short)($before.points[$i]+$fast*(signed char)slicks_original_fastest_points)
    if g_slicks_setup_session.cash[$i] != $cash || g_slicks_setup_session.points[$i] != $points
      printf "TRACK_REWARD_VALUES_FAILED driver=%u\n",$i
      quit 1
    end
    set $i = $i+1
  end
  set $after = g_slicks_setup_session
  set $awards = $awards+1
  set $pending = 0
  printf "TRACK_REWARD track=%u best=%d cash=%d,%d,%d,%d points=%d,%d,%d,%d\n",$loads,$best,$after.cash[0],$after.cash[1],$after.cash[2],$after.cash[3],$after.points[0],$after.points[1],$after.points[2],$after.points[3]
  continue
end
break redraw_title_configuration
commands
  silent
  if $awards
    if $awards != 2 || $loads != 2 || $pending || g_slicks_diag_race_error
      printf "TRACK_REWARD_SEQUENCE_FAILED\n"
      quit 1
    end
    printf "NATIVE_TWO_TRACK_REWARDS_ONCE_PRESERVED_RETURN_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TRACK_REWARD_EARLY_EXIT\n"
  quit 1
end
continue
