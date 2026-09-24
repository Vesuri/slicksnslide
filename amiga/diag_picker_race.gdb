set $pickers = 0
set $accepted = -1
set $initial = -1
break *slicks_diag_profile_picker_ready
commands
  silent
  set $pickers = $pickers + 1
  set $picker = &g_slicks_player_menu->picker->renderer
  if $pickers == 1
    set $initial = $picker->state.selected
  end
  if $pickers == 2
    set $accepted = $picker->state.selected
  end
  if $pickers == 3
    if $picker->state.selected != $accepted
      quit 1
    end
  end
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $pickers != 4 || $accepted == $initial || g_slicks_player_menu || slicks_race_disable_particles
    printf "PICKER_RACE_HANDOFF_FAILED\n"
    quit 1
  end
  if g_slicks_setup_session.players.selected[0] != $accepted
    printf "PICKER_RACE_PROFILE_FAILED\n"
    quit 1
  end
  set $i = 0
  while $i < 4
    if $race->participation[$i] != g_slicks_setup_session.players.participation[$i]
      quit 1
    end
    if $race->participation[$i] && $race->cars[$i].vehicle != g_slicks_setup_session.players.vehicle[$i]
      quit 1
    end
    set $i = $i + 1
  end
  printf "PICKER_RACE_ENTERED selected=%d\n", $accepted
  continue
end
break *slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_frame != 200 || g_slicks_diag_race_error || $race->collision_error
    printf "PICKER_RACE_FAILED frame=%lu error=%u collision=%u\n",g_slicks_diag_race_frame,g_slicks_diag_race_error,$race->collision_error
    quit 1
  end
  set $bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
    quit 1
  end
  dump binary memory .run/picker-race-v1/race200.chunky $race->chunky $race->chunky+64000
  dump binary memory .run/picker-race-v1/race200.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  printf "PICKER_RACE_OK PROFILE_PARTICIPATION_VEHICLES_FRAME200\n"
  quit
end
break *slicks_diag_collision_failed
commands
  silent
  printf "PICKER_RACE_COLLISION_FAILED\n"
  quit 1
end
continue
