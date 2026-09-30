set $loads = 0
set $starts = 0
set $inputs = 0
set $opened = 0
set $awards = 0
set $resident = 0
set $closed = 0
break slicks_amiga_platform_end
commands
  silent
  if $resident
    printf "INTERMISSION_RAM_RETURN_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $resident
    printf "INTERMISSION_RAM_NAVIGATION_ARCHIVE_READ\n"
    quit 1
  end
  continue
end
break slicks_diag_intermission_closed
commands
  silent
  if !$resident || !g_slicks_diag_profile_platform->active || g_slicks_diag_intermission_menu
    printf "INTERMISSION_RETURN_OWNERSHIP_FAILED\n"
    quit 1
  end
  set $resident = 0
  set $closed = $closed+1
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  set $opened = $opened+1
  set $resident = 1
  set $m = g_slicks_diag_intermission_menu
  set $p = &g_slicks_setup_session.players
  if $opened != 1 || !$m || !$m->intermission || $starts != 1 || $awards != 1 || $p->count < 2
    quit 1
  end
  set $first = 0
  while !$p->participation[$first]
    set $first = $first+1
  end
  set $second = $first+1
  while !$p->participation[$second]
    set $second = $second+1
  end
  set $vfirst = $p->vehicle[$first]
  set $vsecond = $p->vehicle[$second]
  set $seed = g_slicks_setup_session.random_state
  dump binary memory .run/intermission-live-v1/profiles.before &g_slicks_profiles (char *)&g_slicks_profiles+sizeof(g_slicks_profiles)
  continue
end
break slicks_diag_intermission_input
commands
  silent
  set $inputs = $inputs+1
  if g_slicks_diag_intermission_phase != $inputs || g_slicks_setup_session.random_state != $seed
    quit 1
  end
  set $f = $vfirst
  set $s = $vsecond
  if $inputs >= 2
    set $f = ($f+1)%10
  end
  if $inputs >= 4
    set $s = ($s+1)%10
  end
  if $inputs >= 7
    set $f = ($f+1)%10
  end
  if $p->vehicle[$first] != $f || $p->vehicle[$second] != $s
    printf "INTERMISSION_EDIT_FAILED phase=%u vehicles=%u/%u expected=%u/%u\n", $inputs, $p->vehicle[$first], $p->vehicle[$second], $f, $s
    quit 1
  end
  if $inputs == 5 || $inputs >= 8
    if $m->change_cars || $m->intermission->content.vehicles[$first] != $f || $m->intermission->content.vehicles[$second] != $s
      quit 1
    end
  else
    if !$m->change_cars
      quit 1
    end
  end
  printf "LIVE_CHANGE_CARS_INPUT %u vehicles=%u/%u\n", $inputs, $f, $s
  continue
end
break prepare_race
commands
  silent
  set $loads = $loads+1
  if !session || $loads > 2 || new_game != ($loads == 1)
    quit 1
  end
  if $loads == 2
    if $inputs != 9 || g_slicks_diag_intermission_menu || g_slicks_setup_session.random_state != $seed || session->players.vehicle[$first] != $f || session->players.vehicle[$second] != $s
      quit 1
    end
    dump binary memory .run/intermission-live-v1/profiles.after &g_slicks_profiles (char *)&g_slicks_profiles+sizeof(g_slicks_profiles)
  end
  continue
end
break *slicks_race_start
commands
  silent
  set $starts = $starts+1
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $starts != $loads || $starts > 2
    quit 1
  end
  if $starts == 2
    if $race->cars[$first].vehicle != $f || $race->cars[$second].vehicle != $s || g_slicks_setup_session.random_state != $seed
      printf "INTERMISSION_EDITED_VEHICLE_HANDOFF_FAILED\n"
      quit 1
    end
    printf "INTERMISSION_EDITED_VEHICLES_REACH_SECOND_RACE %u/%u\n", $f, $s
  end
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  set $awards = $awards+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $loads != 2 || $starts != 2 || $awards != 2 || $inputs != 9 || $opened != 1 || $closed != 1 || g_slicks_diag_restore_status != 0x1f
    printf "INTERMISSION_LIVE_FAILED loads=%u starts=%u awards=%u inputs=%u\n", $loads, $starts, $awards, $inputs
    quit 1
  end
  printf "NATIVE_INTERMISSION_REPEATED_EDITS_SECOND_RACE_OK\n"
  quit
end
continue
