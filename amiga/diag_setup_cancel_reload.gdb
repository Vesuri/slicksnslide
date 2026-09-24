set $checked = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present
    quit 1
  end
  if g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3] || g_slicks_setup_session.players.selected[0] != 3
    quit 1
  end
  set $i = 0
  while $i < 6
    if g_slicks_setup_session.players.colours[0][$i] != g_slicks_profiles.setup[3].colours[$i]
      quit 1
    end
    set $i = $i+1
  end
  set $i = 0
  while $i < 4
    if $race->participation[$i] != g_slicks_setup_session.players.participation[$i] || ($race->participation[$i] && $race->cars[$i].vehicle != g_slicks_setup_session.players.vehicle[$i])
      quit 1
    end
    set $i = $i+1
  end
  if $race->cars[0].vehicle != g_slicks_profiles.setup[3].vehicle
    quit 1
  end
  set $checked = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$checked || g_slicks_diag_race_error
      quit 1
    end
    printf "SAVE_CANCEL_RELOAD_RACE_OK PROFILE_VEHICLE_COLOURS\n"
    quit
  end
  continue
end
continue
