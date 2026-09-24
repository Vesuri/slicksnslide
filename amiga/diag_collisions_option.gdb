set $edited = 0
set $prepared = 0
set $calls = 0
break slicks_diag_options_ready
commands
  silent
  if g_slicks_options_state.row == 11 && g_slicks_options_configuration->options[0] == 4 && g_slicks_options_configuration->options[11] == 0
    set $edited = 1
  end
  continue
end
break prepare_race
commands
  silent
  if !$edited || !session || configuration->options[0] != 4 || configuration->options[11]
    printf "COLLISIONS_OPTION_MENU_HANDOFF_FAILED\n"
    quit 1
  end
  set $r = race
  set $prepared = 1
  continue
end
break slicks_race_resolve_car_collisions
commands
  silent
  if !$prepared || !$r->car_collisions_disabled || $r->collision_count
    printf "COLLISIONS_OPTION_RUNTIME_FAILED\n"
    quit 1
  end
  set $calls = $calls+1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && g_slicks_diag_race_frame == 200
    if !$prepared || $calls < 100 || !$r->racing || g_slicks_diag_race_error || $r->collision_count || !$r->car_collisions_disabled
      quit 1
    end
    printf "NATIVE_COLLISIONS_OFF_MENU_RACE_OK calls=%u terrain_contacts=%u\n", $calls, $r->track_collision_count
    quit
  end
  continue
end
continue
