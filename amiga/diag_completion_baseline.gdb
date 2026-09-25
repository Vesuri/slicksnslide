# CONFIGD: real menu setup and natural race; no writes to game state.
set $race = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  printf "COMPLETION_SETUP fuel=%d damage=%d\n",$race->fuel_option,$race->damage_scale
  continue
end
define completion_cars
  set $i = 0
  while $i < 4
    set $c = &$race->cars[$i]
    printf "CAR %u xy=%d,%d lap=%u finish=%u rank=%d fuel=%d damage=%d service=%d ai=%d speed=%d repairs=%u refuels=%u\n",$i,$c->x,$c->y,$c->lap,$c->finished,$race->finish_ranks[$i],$c->fuel,$c->damage[0],$c->ai_service_state,$c->ai_state,$c->measured_speed,g_slicks_diag_repair_frames[$i],g_slicks_diag_refuel_frames[$i]
    set $i = $i+1
  end
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "COMPLETION_CHECKPOINT frame=%u\n",$race->frame_count
  completion_cars
  continue
end
break slicks_diag_race_progress
commands
  silent
  printf "COMPLETION_PROGRESS frame=%u finished=%u\n",$race->frame_count,$race->finished_count
  if $race->frame_count >= 14400
    completion_cars
    printf "COMPLETION_TIMEOUT\n"
    quit 1
  end
  continue
end
break slicks_diag_results_ready
commands
  silent
  printf "NATURAL_RESULTS frame=%u ticks=%u deadline=%u finished=%u errors=%u\n",$race->frame_count,$race->game_clock_ticks,$race->finish_deadline,$race->finished_count,g_slicks_diag_race_error
  completion_cars
  if !$race->race_complete || !$race->results_drawn || $race->game_clock_ticks <= $race->finish_deadline || g_slicks_diag_race_error
    quit 1
  end
  quit
end
break slicks_diag_collision_failed
commands
  silent
  printf "COMPLETION_COLLISION_ERROR\n"
  quit 1
end
continue
