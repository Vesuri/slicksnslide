set $loaded = 0
set $race = 0
break prepare_race
commands
  silent
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present
    printf "ARCADE_RELOAD_FILES_FAILED\n"
    quit 1
  end
  if configuration->options[0] != 5 || configuration->options[13] != 5 || configuration->options[14] != 2
    printf "ARCADE_RELOAD_OPTIONS_FAILED\n"
    quit 1
  end
  dump binary memory .run/arcade-persistence-v1/reloaded-options.bin &configuration->options[0] &configuration->options[15]
  set $loaded = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if !$loaded || $race->race_mode != 5 || $race->arcade_seconds != 5
    printf "ARCADE_RELOAD_HANDOFF_FAILED\n"
    quit 1
  end
  set $i = 0
  while $i < 4
    if $race->participation[$i] != g_slicks_setup_session.players.participation[$i] || ($race->participation[$i] && $race->cars[$i].vehicle != g_slicks_setup_session.players.vehicle[$i])
      printf "ARCADE_RELOAD_DRIVER_FAILED driver=%d\n", $i
      quit 1
    end
    set $i = $i+1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $race
    if g_slicks_diag_race_error || $race->laps_to_run != 9999 || !$race->arcade_hud_valid || $race->arcade_hud_text || $race->arcade_hud_count != 2
      printf "ARCADE_RELOAD_RACE_FAILED\n"
      quit 1
    end
    printf "ARCADE_FRESH_RELOAD_RACE_OK mode=5 seconds=5 tracks=2 target=9999\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "ARCADE_RELOAD_EARLY_EXIT\n"
  quit 1
end
continue
