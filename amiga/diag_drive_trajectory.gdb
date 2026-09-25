# Read-only observation of native setup, all updates and a bounded pit stall.
set $race=0
set $complete=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_race_progress
commands
  silent
  printf "TRAJECTORY frame=%u clock=%u seed=%lu finished=%u\n",$race->frame_count,$race->game_clock_ticks,$race->random_state,$race->finished_count
  set $i=0
  while $i<4
    set $c=&$race->cars[$i]
    printf "CAR %u vehicle=%u xy=%ld,%ld velocity=%ld,%ld heading=%d lap=%u fuel=%u service=%d target=%d,%d\n",$i,$c->vehicle,$c->x,$c->y,$c->velocity_x,$c->velocity_y,$c->heading,$c->lap,$c->fuel,$c->ai_service_state,$c->ai_target_x,$c->ai_target_y
    set $i=$i+1
  end
  if $race->frame_count>=7200
    set print elements 0
    set print pretty on
    print $race->cars
    print $race->navigation
    print $race->properties
    printf "TRAJECTORY_OBSERVATION_LIMIT_NOT_COMPLETION\n"
    detach
    quit
  end
  continue
end
break slicks_diag_results_ready
commands
  silent
  set $complete=1
  printf "TRAJECTORY_NATURAL_COMPLETION frame=%u finished=%u\n",$race->frame_count,$race->finished_count
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$complete || g_slicks_diag_race_error || g_slicks_diag_restore_status!=0x1f
    quit 1
  end
  printf "TRAJECTORY_RESULTS_RESTORE_OK\n"
  quit
end
continue
