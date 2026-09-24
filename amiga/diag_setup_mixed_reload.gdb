set $checked = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $p = &g_slicks_setup_session.players
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present
    quit 1
  end
  if $p->selected[0] != 3 || $p->selected[1] != 1 || $p->selected[2] != 0 || $p->selected[3] != 1 || $p->count != 3
    quit 1
  end
  if $race->participation[0] != -1 || $race->participation[1] != 1 || $race->participation[2] != 0 || $race->participation[3] != 1
    quit 1
  end
  if $race->cars[0].vehicle != g_slicks_profiles.setup[3].vehicle || $p->order[0] != 0
    quit 1
  end
  set $i = 0
  while $i < 4
    if $race->participation[$i] != $p->participation[$i] || ($p->participation[$i] && $race->cars[$i].vehicle != $p->vehicle[$i])
      quit 1
    end
    set $i = $i+1
  end
  set $i = 0
  while $i < 6
    if $p->colours[0][$i] != g_slicks_profiles.setup[3].colours[$i] || $p->colours[1][$i] != g_slicks_profiles.setup[1].colours[$i] || $p->colours[3][$i] != g_slicks_profiles.setup[1].colours[$i]
      quit 1
    end
    set $i = $i+1
  end
  dump binary memory .run/setup-mixed-v1/reloaded-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1
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
    printf "MIXED_SETUP_RELOAD_RACE_OK HUMAN_SHARED_AI_INACTIVE_VEHICLES_COLOURS\n"
    quit
  end
  continue
end
continue
