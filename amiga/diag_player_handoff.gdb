set $draws = 0
set $closed = 0
set $initial = 0
break *slicks_diag_frame_ready
set $handoff_initial_break=$bpnum
commands
  silent
  if !$initial
    set $initial = 1
    set $platform = g_slicks_diag_profile_platform
    dump binary memory .run/player-handoff-v1/title-before.copper $platform->views[0].copper $platform->views[0].copper+558
    disable $handoff_initial_break
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  set $draws = $draws + 1
  set $menu = g_slicks_player_menu
  if !$menu || $menu->error || $menu->renderer.error
    printf "PLAYER_HANDOFF_MENU_FAILED\n"
    quit 1
  end
  set $chunky = $menu->renderer.ui.pixels
  if $draws == 6
    set $profile = g_slicks_setup_session.players.selected[1]
    set $vehicle = g_slicks_profiles.setup[$profile].vehicle
  end
  if $draws == 7
    if $closed != 1 || g_slicks_profiles.setup[$profile].vehicle != $vehicle
      printf "PLAYER_HANDOFF_REOPEN_FAILED\n"
      quit 1
    end
    set $bitmap = $platform->views[0].bitmap
    dump binary memory .run/player-handoff-v1/reopened.chunky $chunky $chunky+64000
    dump binary memory .run/player-handoff-v1/reopened.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  continue
end
break *slicks_diag_player_menu_closed
commands
  silent
  set $closed = $closed + 1
  if g_slicks_player_menu || !$platform->active
    printf "PLAYER_HANDOFF_CLOSE_FAILED\n"
    quit 1
  end
  set $bitmap = $platform->views[0].bitmap
  dump binary memory .run/player-handoff-v1/title-after.chunky $chunky $chunky+64000
  dump binary memory .run/player-handoff-v1/title-after.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  dump binary memory .run/player-handoff-v1/title-after.copper $platform->views[0].copper $platform->views[0].copper+558
  printf "PLAYER_HANDOFF_CLOSED count=%d\n", $closed
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $draws != 7 || $closed != 2 || g_slicks_player_menu
    printf "PLAYER_HANDOFF_SEQUENCE_FAILED\n"
    quit 1
  end
  if slicks_race_disable_particles || g_slicks_diag_target_frame != 200
    printf "PLAYER_HANDOFF_DIAGNOSTIC_MODE_FAILED\n"
    quit 1
  end
  set $i = 0
  while $i < 4
    if $race->participation[$i] != g_slicks_setup_session.players.participation[$i]
      printf "PLAYER_HANDOFF_PARTICIPATION_FAILED\n"
      quit 1
    end
    if $race->participation[$i] && $race->cars[$i].vehicle != g_slicks_setup_session.players.vehicle[$i]
      printf "PLAYER_HANDOFF_VEHICLE_FAILED driver=%d\n", $i
      quit 1
    end
    set $i = $i + 1
  end
  continue
end
break *slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_frame != 200 || g_slicks_diag_race_error || $race->collision_error
    printf "PLAYER_HANDOFF_RACE_FAILED frame=%lu runtime_frame=%lu error=%u collision=%u target=%lu\n", g_slicks_diag_race_frame, $race->frame_count, g_slicks_diag_race_error, $race->collision_error, g_slicks_diag_target_frame
    quit 1
  end
  set $bitmap = $platform->views[1].bitmap
  if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
    quit 1
  end
  dump binary memory .run/player-handoff-v1/race200.chunky $race->chunky $race->chunky+64000
  dump binary memory .run/player-handoff-v1/race200.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  printf "PLAYER_HANDOFF_OK TWO_CLOSES_REOPEN_SETUP_PRESERVED_RACE_200\n"
  quit
end
continue
