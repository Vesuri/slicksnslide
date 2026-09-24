set $loaded = 0
set $started = 0
break prepare_race
commands
  silent
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present || !session || configuration->field_05de != 106
    printf "PAUSE_EDIT_FRESH_LOAD_FAILED\n"
    quit 1
  end
  set $c = configuration
  set $r = race
  dump binary memory .run/pause-save-v1/reloaded.config $c (char *)$c+sizeof(*$c)
  dump binary memory .run/pause-save-v1/reloaded.profiles &g_slicks_profiles &g_slicks_profiles+1
  set $loaded = 1
  continue
end
break *slicks_race_start
commands
  silent
  if !$loaded
    quit 1
  end
  set $started = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $started && g_slicks_diag_race_frame >= 100
    if g_slicks_diag_race_error || $r->physics_tick_period != 618250 || driver_device_configuration_storage.field_05de != 106 || driver_device_configuration_storage.player_input[0] != $c->player_input[0]
      printf "PAUSE_EDIT_RELOAD_CONSUMER_FAILED\n"
      quit 1
    end
    printf "NATIVE_PAUSE_EDIT_FRESH_RELOAD_RACE_OK speed=106 device=%u\n", $c->player_input[0]
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "PAUSE_EDIT_RELOAD_EARLY_EXIT\n"
  quit 1
end
continue
