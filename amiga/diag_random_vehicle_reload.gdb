set $prepared = 0
set $checked = 0
set $seed = 0x1234
set $startup_calls = 0
set $weapon_seed_checked = 0
define choose_expected
  set $seed = ($seed*0x015a4e35+1)&0xffffffff
  set $total = 0
  set $i = 0
  while $i < 10
    set $total = $total+slicks_original_vehicle_weights[$i]
    set $i = $i+1
  end
  set $draw = ($total*(($seed>>16)&0x7fff))/32768
  set $sum = 0
  set $expected = 0
  while $expected < 10
    set $sum = $sum+slicks_original_vehicle_weights[$expected]
    if $draw < $sum
      loop_break
    end
    set $expected = $expected+1
  end
end
break request_setup_vehicle
commands
  silent
  if !$prepared
    choose_expected
    if driver != $startup_calls || vehicle != $expected || g_slicks_setup_session.random_state != $seed
      printf "RANDOM_VEHICLE_STARTUP_FAILED\n"
      quit 1
    end
    set $startup_calls = $startup_calls+1
  end
  continue
end
break prepare_race
commands
  silent
  if $startup_calls != 3 || !session || !new_game || g_slicks_setup_load_report.result || !g_slicks_setup_load_report.profiles_present || !g_slicks_setup_load_report.configuration_present
    quit 1
  end
  set $p = &session->players
  if $p->selected[0] != 1 || $p->selected[1] != 1 || $p->selected[2] != 3 || $p->selected[3] != 0 || g_slicks_profiles.setup[3].vehicle != 10 || g_slicks_profiles.setup[1].vehicle != 11
    quit 1
  end
  set $once = $p->vehicle[2]
  set $seed = session->random_state
  set $prepared = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $p = &g_slicks_setup_session.players
  if !$prepared || $p->vehicle[2] != $once || $once < 0 || $once >= 10
    quit 1
  end
  set $d = 0
  while $d < 3
    set $expected = $once
    if $d < 2
      choose_expected
    end
    set $role = 1
    if $d == 2
      set $role = -1
    end
    if $p->vehicle[$d] != $expected || $race->cars[$d].vehicle != $expected || $race->participation[$d] != $role
      printf "RANDOM_VEHICLE_RACE_FAILED driver=%u expected=%u actual=%u\n",$d,$expected,$p->vehicle[$d]
      quit 1
    end
    set $d = $d+1
  end
  if g_slicks_setup_session.random_state != $seed || $race->participation[3] || g_slicks_profiles.setup[3].vehicle != 10
    printf "RANDOM_VEHICLE_STREAM_OR_PROFILE_CHANGED\n"
    quit 1
  end
  set $checked = 1
  continue
end
break slicks_amiga_platform_set_view
commands
  silent
  if $checked && view == 1 && !$weapon_seed_checked
    set $weapon_seed = $seed
    set $d = 0
    while $d < 4
      if $race->participation[$d] > 0
        set $weapon_seed = ($weapon_seed*0x015a4e35+1)&0xffffffff
      end
      if $race->selected_weapon[$d] != -1
        quit 1
      end
      set $d = $d+1
    end
    if $race->random_state != $weapon_seed || g_slicks_setup_session.random_state != $seed
      printf "RANDOM_VEHICLE_WEAPON_STARTUP_STREAM_FAILED\n"
      quit 1
    end
    set $weapon_seed_checked = 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$checked || !$weapon_seed_checked || g_slicks_diag_race_error
      quit 1
    end
    printf "RANDOM_ONCE_RETAINED_RANDOM_EACH_REROLLED_NATIVE_RELOAD_RACE_OK\n"
    quit
  end
  continue
end
continue
