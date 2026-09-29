# CHAMPSAVE queues ordinary raw-key input in native code. Read-only checks.
source diag_saved_resident.gdb
set $picker = 0
set $name = 0
set $saved = 0
set $starts = 0
break *slicks_race_start
commands
  silent
  set $starts = $starts+1
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  printf "SAVE_INTERMISSION count=%u points=%d cash=%d\n",g_slicks_track_playlist.count,g_slicks_setup_session.points[0],g_slicks_setup_session.cash[0]
  if g_slicks_track_playlist.count != 3 || $starts != 1
    quit 1
  end
  continue
end
break slicks_diag_saved_ready
commands
  silent
  printf "SAVED_DIALOG phase=%u picker=%u\n",g_slicks_diag_saved_phase,$picker
  if g_slicks_diag_saved_phase == 1
    set $picker = $picker+1
  end
  if g_slicks_diag_saved_phase == 2
    if $name
      quit 1
    end
    set $name = 1
  end
  if g_slicks_diag_saved_phase == 3
    if !$name || $picker != 2
      quit 1
    end
    dump binary memory .run/championship-v1/points.before &g_slicks_setup_session.points (char *)&g_slicks_setup_session.points+8
    dump binary memory .run/championship-v1/cash.before &g_slicks_setup_session.cash (char *)&g_slicks_setup_session.cash+8
    dump binary memory .run/championship-v1/inventory.before &g_slicks_setup_session.inventory (char *)&g_slicks_setup_session.inventory+104
    dump binary memory .run/championship-v1/vehicles.before &g_slicks_setup_session.players.vehicle (char *)&g_slicks_setup_session.players.vehicle+4
    set $saved = 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f || $starts != 1
    printf "CHAMPIONSHIP_SAVE_FAILED picker=%u restore=%u starts=%u\n",$picker,g_slicks_diag_restore_status,$starts
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK\n"
  quit
end
continue
