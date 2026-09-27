# CONFIGD reaches the maximum-damage/fuel race through normal menu handlers.
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "DAMAGE_RACE_LOAD_ERROR=%u\n", g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_diag_results_ready
break slicks_diag_gameplay_ready
commands 2 3
  silent
  set $car = 0
  set $repairs = 0
  set $damaged = 0
  set $finished = 0
  set $positions = 0
  while $car < 4
    printf "DAMAGE_CAR=%u PEAK=%u REPAIR_FRAMES=%u FUEL=%d LAP=%u FINISHED=%u\n", $car, g_slicks_diag_damage_peak[$car], g_slicks_diag_repair_frames[$car], g_slicks_diag_fuel[$car], g_slicks_diag_lap[$car], g_slicks_diag_finished[$car]
    set $state = &$raceptr->cars[$car]
    printf "DAMAGE_AI CAR=%u XY=%d,%d VELOCITY=%d,%d WAYPOINT=%u CHECKPOINT=%u STATE=%d SERVICE=%d TARGET=%d,%d WATCHDOG=%d RECOVERY=%d CONTROLS=%u SURFACE=%u\n", $car, $state->x, $state->y, $state->velocity_x, $state->velocity_y, $state->waypoint, $state->checkpoint, $state->ai_state, $state->ai_service_state, $state->ai_target_x, $state->ai_target_y, $state->ai_stuck_ticks, $state->ai_recovery_ticks, $state->ai_control_latch, $state->effective_surface
    set $repairs = $repairs + g_slicks_diag_repair_frames[$car]
    if g_slicks_diag_damage_peak[$car] >= 40
      set $damaged = $damaged + 1
    end
    # Finish ranks, not a fixed visible lap, are the DOS completion contract.
    if g_slicks_diag_finished[$car]
      set $finished = $finished + 1
    end
    set $position = g_slicks_diag_finish_position[$car]
    if $position >= 1 && $position <= 4
      set $positions = $positions | (1 << $position)
    end
    set $car = $car + 1
  end
  printf "DAMAGE_RACE FRAME=%u COMPLETE=%u STATUS_CHECKS=%u DAMAGE_CHECKS=%u PIXEL_FAILURES=%u\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete, g_slicks_diag_status_checks, g_slicks_diag_damage_status_checks, g_slicks_diag_status_failures
  printf "DAMAGE_COLLISIONS CARS=%u TRACK=%u\n", g_slicks_diag_collisions, g_slicks_diag_track_collisions
  printf "DAMAGE_SETUP SCALE=%d ENABLED=%u PEAK_IMPACT=%u\n", $raceptr->damage_scale, $raceptr->damage_enabled, g_slicks_diag_damage_peak_impact
  if !$damaged || !$repairs || $finished != 4 || $positions != 30 || !g_slicks_diag_race_complete || !g_slicks_diag_results_drawn || !g_slicks_diag_damage_status_checks || g_slicks_diag_status_failures
    printf "DAMAGE_RACE_FAILED\n"
    quit 1
  end
  printf "DAMAGE_RACE_OK\n"
  quit
end
break slicks_race_start
commands
  silent
  set $raceptr = race
  if race->fuel_option != 10 || race->damage_scale != 300 || !race->damage_enabled
    printf "DAMAGE_SETUP_FAILED\n"
    quit 1
  end
  continue
end
# The entry breakpoint is disabled once racing; retain a separate failure trap.
break slicks_diag_collision_failed
commands
  silent
  printf "DAMAGE_RACE_COLLISION_ERROR=%u FRAME=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_frame
  quit 1
end
break slicks_diag_race_progress
commands
  silent
  set $state = &$raceptr->cars[3]
  printf "CAR4_PROGRESS FRAME=%u LAP=%u CHECKPOINT=%u WAYPOINT=%u XY=%d,%d SPEED=%d STATE=%d SERVICE=%d FUEL=%d DAMAGE=%d TARGET=%d,%d\n", g_slicks_diag_race_frame, $state->lap, $state->checkpoint, $state->waypoint, $state->x, $state->y, $state->measured_speed, $state->ai_state, $state->ai_service_state, $state->fuel, $state->damage[0], $state->ai_target_x, $state->ai_target_y
  continue
end
continue
