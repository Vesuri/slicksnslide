# Read-only snapshot for replay against the original AI. Same CONFIGD setup.
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SERVICE_STALL_LOAD_ERROR=%u\n", g_slicks_diag_race_error
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
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "SERVICE_STALL_COLLISION_ERROR=%u\n", g_slicks_diag_race_error
  quit 1
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "SERVICE_STALL_FRAME=%u\n", g_slicks_diag_race_frame
  set print pretty on
  set print elements 0
  print $raceptr->cars[2]
  print $raceptr->properties[$raceptr->cars[2].vehicle]
  print $raceptr->navigation.zones[$raceptr->cars[2].waypoint]
  print $raceptr->navigation.pits
  print $raceptr->fuel_option
  print $raceptr->damage_scale
  print $raceptr->laps_to_run
  print $raceptr->random_state
  print $raceptr->boundary_level
  quit
end
continue
