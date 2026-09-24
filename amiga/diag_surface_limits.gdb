break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break *slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_error || g_slicks_diag_race_frame != 200 || $race->collision_error
    printf "SURFACE_LIMITS_RUN_FAILED FRAME=%u ACTUAL=%u TARGET=%u ERROR=%u COLLISION=%u\n",g_slicks_diag_race_frame,$race->frame_count,g_slicks_diag_target_frame,g_slicks_diag_race_error,$race->collision_error
    quit 1
  end
  set $car = 0
  while $car < 4
    set $profile = g_slicks_setup_session.players.selected[$car]
    if $profile < 0 || $profile >= g_slicks_profiles.count || $race->cars[$car].position_scale != g_slicks_profiles.setting[$profile]
      printf "PROFILE_POSITION_SCALE_FAILED CAR=%u\n",$car
      quit 1
    end
    printf "PROFILE_POSITION_SCALE CAR=%u PROFILE=%u SCALE=%u\n",$car,$profile,$race->cars[$car].position_scale
    set $steering_input = $race->cars[$car].position_scale
    if !$race->participation_ready || $race->participation[$car] != g_slicks_setup_session.players.participation[$car]
      printf "PARTICIPATION_HANDOFF_FAILED CAR=%u\n",$car
      quit 1
    end
    printf "PARTICIPATION CAR=%u ROLE=%d\n",$car,$race->participation[$car]
    if $race->participation[$car] > 0
      set $steering_input = $steering_input * 7 / 5
    end
    if $race->cars[$car].steering_amount != $steering_input
      printf "PROFILE_STEERING_INPUT_FAILED CAR=%u\n",$car
      quit 1
    end
    printf "SURFACE_LIMITS CAR=%u VEHICLE=%u SURFACE=%u STEERING=%d SPEED_LIMIT=%u\n",$car,$race->cars[$car].vehicle,$race->cars[$car].effective_surface,$race->cars[$car].steering_scale,$race->cars[$car].maximum_speed
    set $car = $car + 1
  end
  set $bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  dump binary memory .run/participants-v1/frame200.chunky $race->chunky $race->chunky+64000
  dump binary memory .run/participants-v1/frame200.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  printf "SURFACE_LIMITS_RUN_OK FRAME=%u\n",g_slicks_diag_race_frame
  quit
end
continue
