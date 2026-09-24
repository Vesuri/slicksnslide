break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break *slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    set $start_x0 = $race->cars[0].x
    set $start_y0 = $race->cars[0].y
    set $start_x2 = $race->cars[2].x
    set $start_y2 = $race->cars[2].y
    disable 2
  end
  continue
end
break *slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_frame != 200 || g_slicks_diag_race_error || $race->collision_error
    printf "DRIVER_INPUT_RACE_FAILED\n"
    quit 1
  end
  if $race->participation[0] != -1 || $race->participation[1] != 0 || $race->participation[2] != -1 || $race->participation[3] != 1
    printf "DRIVER_INPUT_ROLES_FAILED\n"
    quit 1
  end
  if $race->driver_controls[0] != 1 || $race->driver_controls[2] != 1
    printf "DRIVER_INPUT_KEYS_FAILED\n"
    quit 1
  end
  if ($race->cars[0].x == $start_x0 && $race->cars[0].y == $start_y0) || ($race->cars[2].x == $start_x2 && $race->cars[2].y == $start_y2)
    printf "DRIVER_INPUT_MOVEMENT_FAILED\n"
    quit 1
  end
  if $race->cars[1].saved_valid || $race->cars[1].elapsed_time_units || $race->cars[1].lap || $race->hud_valid[1]
    printf "DRIVER_INPUT_INACTIVE_FAILED\n"
    quit 1
  end
  set $bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  dump binary memory .run/driver-input-v1/frame200.chunky $race->chunky $race->chunky+64000
  dump binary memory .run/driver-input-v1/frame200.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  printf "DRIVER_INPUT_OK FRAME=200 ROLES=-1,0,-1,1 HUMAN_CONTROLS=1,1 INACTIVE_UNDRAWN\n"
  quit
end
continue
