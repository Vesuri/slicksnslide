# SETUP uses the production lifecycle with an explicit diagnostic seed 1234.
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $driver = 0
  while $driver < 4
    set $expected = 0
    if $driver == 2
      set $expected = 5
    end
    if $race->cars[$driver].vehicle != g_slicks_setup_session.players.vehicle[$driver] || $race->cars[$driver].vehicle != $expected
      printf "SETUP_SESSION_VEHICLE_FAILED DRIVER=%u\n",$driver
      quit 1
    end
    set $driver = $driver + 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SETUP_SESSION_LOAD_FAILED ERROR=%u\n",g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame
    if $race->random_state != 0xa7020e1f || g_slicks_setup_session.random_state != $race->random_state || g_slicks_setup_session.players.count != 4
      printf "SETUP_SESSION_STATE_FAILED RNG=%08x\n",$race->random_state
      quit 1
    end
    set $driver = 0
    while $driver < 4
      set $slot = 0
      while $slot < 13
        if $race->weapon_inventory[$driver][$slot] != g_slicks_setup_session.inventory[$driver][$slot] || $race->cars[$driver].drive_setup[$slot] != g_slicks_setup_session.inventory[$driver][$slot]
          printf "SETUP_SESSION_INVENTORY_FAILED DRIVER=%u SLOT=%u\n",$driver,$slot
          quit 1
        end
        set $slot = $slot + 1
      end
      if $race->cars[$driver].fuel_upgrade != g_slicks_setup_session.inventory[$driver][2]
        quit 1
      end
      if $race->cars[$driver].steering_property != $race->cars[$driver].drive_coefficients[6] || $race->cars[$driver].steering_property != 104
        printf "SETUP_SESSION_STEERING_FAILED DRIVER=%u\n",$driver
        quit 1
      end
      set $driver = $driver + 1
    end
    printf "SETUP_SESSION_INVENTORY_OK ALL_52_SLOTS\n"
    printf "SETUP_SESSION_STEERING_OK ALL_FOUR_DRIVERS\n"
    printf "SETUP_SESSION_OK VEHICLES=0,0,5,0 RNG=%08x COUNT=%u\n",$race->random_state,g_slicks_setup_session.players.count
    quit
  end
  continue
end
continue
