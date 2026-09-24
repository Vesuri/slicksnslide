set $edited = 0
set $prepared = 0
set $initialized = 0
set $r = 0
break slicks_diag_options_ready
commands
  silent
  if g_slicks_options_state.row == 7 && g_slicks_options_configuration->options[0] == 4 && g_slicks_options_configuration->options[7] == $expected
    set $edited = 1
  end
  continue
end
break prepare_race
commands
  silent
  if !$edited || !session || configuration->options[0] != 4 || configuration->options[7] != $expected
    quit 1
  end
  set $prepared = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $r = *(struct SlicksRaceRuntime **)($sp+4)
  if !$prepared || $r->weapons_enabled != $expected
    quit 1
  end
  set $seed = g_slicks_setup_session.random_state
  set $d = 0
  while $d < 4
    if g_slicks_setup_session.players.participation[$d] > 0
      set $seed = ($seed*0x015a4e35+1)&0xffffffff
    end
    set $d = $d+1
  end
  continue
end
break slicks_amiga_platform_set_view
commands
  silent
  if $r && view == 1 && !$initialized
    if $r->random_state != $seed || $r->weapons_enabled != $expected
      printf "WEAPONS_INITIAL_RNG_OR_GATE_FAILED\n"
      quit 1
    end
    set $d = 0
    while $d < 4
      if $r->selected_weapon[$d] != -1
        printf "WEAPONS_EMPTY_INVENTORY_SELECTED\n"
        quit 1
      end
      set $d = $d+1
    end
    if $r->weapon_capacity[5] != 100 || $r->weapon_capacity[6] != 50 || $r->weapon_capacity[7] != 50 || $r->weapon_capacity[8] != 10 || $r->weapon_capacity[9] != 80 || $r->weapon_capacity[10] != 10 || $r->weapon_capacity[11] != 100 || $r->weapon_capacity[12] != 10
      quit 1
    end
    set $initialized = 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && g_slicks_diag_race_frame == 200
    if !$initialized || !$r->racing || g_slicks_diag_race_error || $r->weapons_enabled != $expected || driver_device_configuration_storage.options[7] != $expected
      quit 1
    end
    printf "NATIVE_WEAPONS_OPTION_MENU_RACE_OK enabled=%u original_initial_selection_RNG=verified\n", $expected
    quit
  end
  continue
end
continue
