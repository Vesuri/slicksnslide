set $race = 0
set $awards = 0
set $seen = 0
set $pending = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if !$race->finish_reward
    printf "FINISH_REWARD_NOT_CONNECTED\n"
    quit 1
  end
  set $i = 0
  while $i < 4
    if g_slicks_setup_session.points[$i] || g_slicks_setup_session.cash[$i] != g_slicks_setup_session.options.starting_cash
      printf "FINISH_REWARD_INITIAL_STATE_FAILED\n"
      quit 1
    end
    set $i = $i+1
  end
  continue
end
break *award_race_finish
commands
  silent
  set $driver = *(unsigned int *)($sp+8)
  set $rank = *(signed char *)($sp+15)
  if $pending || $driver >= 4 || $rank < 1 || $rank > 4 || ($seen & (1 << $driver))
    printf "FINISH_REWARD_EVENT_FAILED driver=%u rank=%d\n",$driver,$rank
    quit 1
  end
  set $cash = (short)(g_slicks_setup_session.cash[$driver]+(signed char)(g_slicks_setup_session.players.count-$rank)*g_slicks_setup_session.options.field_302e)
  set $points = (short)(g_slicks_setup_session.points[$driver]+(signed char)slicks_original_finish_points[$rank-1])
  set $pending = 1
  continue
end
break slicks_diag_finish_rewarded
commands
  silent
  if !$pending || g_slicks_setup_session.cash[$driver] != $cash || g_slicks_setup_session.points[$driver] != $points
    printf "FINISH_REWARD_VALUES_FAILED\n"
    quit 1
  end
  set $pending = 0
  set $seen = $seen | (1 << $driver)
  set $awards = $awards+1
  printf "FINISH_REWARD driver=%u rank=%d cash=%d points=%d\n",$driver,$rank,$cash,$points
  continue
end
break draw_results
commands
  silent
  if !$awards || $pending || !$race || !$race->race_complete || g_slicks_diag_race_error
    printf "FINISH_REWARD_RACE_END_FAILED\n"
    quit 1
  end
  set $i = 0
  while $i < 4
    if (($seen >> $i) & 1) != ($race->finish_ranks[$i] > 0)
      printf "FINISH_REWARD_RANK_COVERAGE_FAILED\n"
      quit 1
    end
    set $i = $i+1
  end
  printf "NATIVE_FINISH_REWARD_EVENTS_SESSION_CASH_POINTS_OK awards=%u\n",$awards
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "FINISH_REWARD_EARLY_EXIT\n"
  quit 1
end
continue
