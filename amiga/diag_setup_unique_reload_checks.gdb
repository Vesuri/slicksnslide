set $checked = 0
set $palette_checked = 0
break prepare_race
commands
  silent
  eval "dump binary memory %s/reloaded-configuration.bin configuration (char *)configuration+sizeof(*configuration)", $unique_run
  continue
end
break *slicks_race_set_status_palette
commands
  silent
  set $palette = *(unsigned char **)($sp+8)
  set $p = &g_slicks_setup_session.players
  set $i = 0
  while $i < 3
    set $shade = 0
    while $shade < 5
      set $c = 0
      while $c < 3
        set $first = (signed char)$p->colours[$i][$c]
        set $last = (signed char)$p->colours[$i][$c+3]
        set $expected = (unsigned char)(($first*(4-$shade))/4+($last*$shade)/4)
        if $palette[(1+$i*5+$shade)*3+$c] != $expected
          printf "UNIQUE_PROFILE_PALETTE_FAILED slot=%u shade=%u channel=%u\n",$i,$shade,$c
          quit 1
        end
        set $c = $c+1
      end
      set $shade = $shade+1
    end
    set $i = $i+1
  end
  set $palette_checked = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $p = &g_slicks_setup_session.players
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present
    quit 1
  end
  if g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3] || g_slicks_profiles.setup[3].flags & 2
    quit 1
  end
  if $p->selected[0] != 1 || $p->selected[1] != 1 || $p->selected[2] != 3 || $p->selected[3] != 0 || $p->count != 3 || $p->order[0] != 2
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
    if $race->participation[$i] != $role || $p->participation[$i] != $role || ($role && $race->cars[$i].vehicle != $p->vehicle[$i])
      quit 1
    end
    set $i = $i+1
  end
  if $race->cars[2].vehicle != g_slicks_profiles.setup[3].vehicle
    quit 1
  end
  set $i = 0
  while $i < 6
    if $p->colours[2][$i] != g_slicks_profiles.setup[3].colours[$i] || $p->colours[0][$i] != g_slicks_profiles.setup[1].colours[$i] || $p->colours[1][$i] != g_slicks_profiles.setup[1].colours[$i]
      quit 1
    end
    set $i = $i+1
  end
  eval "dump binary memory %s/reloaded-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1", $unique_run
  set $checked = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$checked || !$palette_checked || g_slicks_diag_race_error
      quit 1
    end
    printf "UNIQUE_PROFILE_FRESH_RELOAD_RACE_DRIVERS_VEHICLE_COLOURS_OK\n"
    quit
  end
  continue
end
continue
