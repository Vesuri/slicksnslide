# Separate bounded observation. Does NOT replace diag_damage_race.gdb's deadline.
define dump_extended_cars
  set $inspect_car = 0
  while $inspect_car < 4
    set $inspect = &$raceptr->cars[$inspect_car]
    printf "EXTENDED_STATE CAR=%u LAP=%u FINISHED=%u PLACE=%u XY=%d,%d VELOCITY=%d,%d FUEL=%d DAMAGE=%d AI=%d SERVICE=%d WAYPOINT=%u CHECKPOINT=%u WATCHDOG=%u RECOVERY=%d\n", $inspect_car, $inspect->lap, $inspect->finished, $inspect->finish_position, $inspect->x, $inspect->y, $inspect->velocity_x, $inspect->velocity_y, $inspect->fuel, $inspect->damage[0], $inspect->ai_state, $inspect->ai_service_state, $inspect->waypoint, $inspect->checkpoint, $inspect->ai_stuck_ticks, $inspect->ai_recovery_ticks
    set $inspect_car = $inspect_car + 1
  end
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "EXTENDED_LOAD_ERROR=%u\n", g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_race_start
commands
  silent
  set $raceptr = race
  if race->fuel_option != 10 || race->damage_scale != 300 || !race->damage_enabled
    printf "EXTENDED_SETUP_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "EXTENDED_COLLISION_ERROR=%u FRAME=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_frame
  quit 1
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "EXTENDED_STRICT_DEADLINE FRAME=%u COMPLETE=%u (strict gate remains separate)\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete
  dump_extended_cars
  continue
end
break slicks_diag_results_ready
commands
  silent
  set $car = 0
  set $places = 0
  set $repairs = 0
  set $damaged = 0
  while $car < 4
    set $state = &$raceptr->cars[$car]
    printf "EXTENDED_CAR=%u LAP=%u FINISHED=%u PLACE=%u DAMAGE_PEAK=%u REPAIRS=%u FUEL=%d\n", $car, $state->lap, $state->finished, $state->finish_position, g_slicks_diag_damage_peak[$car], g_slicks_diag_repair_frames[$car], $state->fuel
    if !$state->finished || $state->lap != 5 || $state->finish_position < 1 || $state->finish_position > 4
      printf "EXTENDED_FINISH_FAILED\n"
      quit 1
    end
    set $places = $places | (1 << $state->finish_position)
    set $repairs = $repairs + g_slicks_diag_repair_frames[$car]
    if g_slicks_diag_damage_peak[$car] >= 40
      set $damaged = $damaged + 1
    end
    set $car = $car + 1
  end
  printf "EXTENDED_RESULTS FRAME=%u COMPLETE=%u RESULTS=%u STATUS_CHECKS=%u DAMAGE_CHECKS=%u PIXEL_FAILURES=%u\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete, g_slicks_diag_results_drawn, g_slicks_diag_status_checks, g_slicks_diag_damage_status_checks, g_slicks_diag_status_failures
  if $places != 30 || !$repairs || !$damaged || !g_slicks_diag_race_complete || !g_slicks_diag_results_drawn || !g_slicks_diag_damage_status_checks || g_slicks_diag_status_checks < 3 || g_slicks_diag_status_failures || g_slicks_diag_race_frame > 7200
    printf "EXTENDED_RESULTS_FAILED\n"
    quit 1
  end
  printf "EXTENDED_NATURAL_FINISH_OK\n"
  quit
end
break slicks_diag_race_progress
commands
  silent
  set $state = &$raceptr->cars[3]
  printf "EXTENDED_PROGRESS FRAME=%u LAP=%u CHECKPOINT=%u XY=%d,%d STATE=%d SERVICE=%d FUEL=%d DAMAGE=%d\n", g_slicks_diag_race_frame, $state->lap, $state->checkpoint, $state->x, $state->y, $state->ai_state, $state->ai_service_state, $state->fuel, $state->damage[0]
  if g_slicks_diag_race_frame >= 7200 && !g_slicks_diag_race_complete
    printf "EXTENDED_OBSERVATION_LIMIT\n"
    dump_extended_cars
    quit 1
  end
  continue
end
continue
