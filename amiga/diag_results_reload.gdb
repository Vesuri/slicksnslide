# Fresh SETUPR process in the NATURALD/NATURALF sandbox; no target writes.
break *slicks_race_start
commands
  silent
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.profiles_present || g_slicks_profiles.count != 7
    quit 1
  end
  set $i = 0
  while $i < 4
    if g_slicks_setup_session.players.selected[$i] != $i+3 || g_slicks_profiles.names[$i+3][4] != 49+$i
      quit 1
    end
    set $vehicle = 0
    if $i == 0
      set $vehicle = 5
    end
    if $i == 1
      set $vehicle = 2
    end
    if g_slicks_setup_session.players.vehicle[$i] != $vehicle || g_slicks_profiles.statistics[$i+3][0] != 1
      quit 1
    end
    set $i = $i+1
  end
  dump binary memory .run/native-results/reloaded.stats &g_slicks_profiles.statistics[3] &g_slicks_profiles.statistics[7]
  printf "NATIVE_RESULTS_RELOAD_CAPTURE_OK\n"
  quit
end
continue
