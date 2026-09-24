# Separate process after CHAMPSAVE, same isolated DH1. No state injection.
set $loads = 0
set $started = 0
set $advanced = 0
set $picker = 0
break slicks_diag_saved_ready
commands
  silent
  if g_slicks_diag_saved_phase != 1
    printf "CHAMPIONSHIP_LOAD_WARNING phase=%u\n",g_slicks_diag_saved_phase
    quit 1
  end
  set $picker = $picker+1
  continue
end
break prepare_race
commands
  silent
  set $loads = $loads+1
  if $loads != 1 || new_game || !session || !session->saved_position_scale_valid
    quit 1
  end
  printf "RESUME_PREPARE path=%s cash=%d points=%d new_game=%u\n",track_path,session->cash[0],session->points[0],new_game
  dump binary memory .run/championship-v1/points.after &session->points (char *)&session->points+8
  dump binary memory .run/championship-v1/cash.after &session->cash (char *)&session->cash+8
  dump binary memory .run/championship-v1/inventory.after &session->inventory (char *)&session->inventory+104
  dump binary memory .run/championship-v1/vehicles.after &session->players.vehicle (char *)&session->players.vehicle+4
  continue
end
break *slicks_race_start
commands
  silent
  set $r = *(struct SlicksRaceRuntime **)($sp+4)
  set $i = 0
  while $i < 4
    if $r->cars[$i].vehicle != g_slicks_setup_session.players.vehicle[$i] || $r->cars[$i].position_scale != g_slicks_setup_session.saved_position_scale[$i]
      quit 1
    end
    set $j = 0
    while $j < 13
      if $r->weapon_inventory[$i][$j] != g_slicks_setup_session.inventory[$i][$j]
        quit 1
      end
      set $j = $j+1
    end
    set $i = $i+1
  end
  set $started = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if g_slicks_diag_race_error || g_slicks_track_playlist.count != 3 || !$started
      quit 1
    end
    if g_slicks_diag_race_frame >= 40
      set $advanced = 1
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $loads != 1 || $picker != 1 || !$advanced || g_slicks_diag_restore_status != 0x1f
    printf "CHAMPIONSHIP_RESUME_FAILED loads=%u picker=%u advanced=%u restore=%u\n",$loads,$picker,$advanced,g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_FRESH_PROCESS_RESUME_RACE_OK\n"
  quit
end
continue
