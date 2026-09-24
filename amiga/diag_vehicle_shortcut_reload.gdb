set $checked = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $p = &g_slicks_setup_session.players
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present || g_slicks_profiles.count != 4
    quit 1
  end
  if $p->selected[0] != 1 || $p->selected[1] != 1 || $p->selected[2] != 3 || $p->selected[3] != 0 || $p->count != 3
    quit 1
  end
  set $i = 0
  while $i < 4
    set $role = 1
    if $i == 2
      set $role = -1
    end
    if $i == 3
      set $role = 0
    end
    if $race->participation[$i] != $role || $p->participation[$i] != $role
      quit 1
    end
    if $role
      set $profile = $p->selected[$i]
      if $race->cars[$i].vehicle != $p->vehicle[$i] || $p->vehicle[$i] < 0 || $p->vehicle[$i] >= 10
        quit 1
      end
      if g_slicks_profiles.setup[$profile].vehicle < 10 && $p->vehicle[$i] != g_slicks_profiles.setup[$profile].vehicle
        quit 1
      end
      set $c = 0
      while $c < 6
        if $p->colours[$i][$c] != g_slicks_profiles.setup[$profile].colours[$c]
          quit 1
        end
        set $c = $c+1
      end
    end
    set $i = $i+1
  end
  dump binary memory .run/vehicle-shortcut-v1/reloaded-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1
  set $checked = 1
  printf "VEHICLE_SHORTCUT_RELOADED vehicle=%u\n",g_slicks_profiles.setup[3].vehicle
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$checked || g_slicks_diag_race_error
      quit 1
    end
    printf "VEHICLE_SHORTCUT_FRESH_RELOAD_RACE_OK\n"
    quit
  end
  continue
end
continue
