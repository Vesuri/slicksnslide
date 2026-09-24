set $saved = 0
set $initial = -1
break slicks_diag_player_menu_ready
commands
  silent
  if $initial == -1
    if g_slicks_setup_load_report.result || g_slicks_profiles.count != 4 || g_slicks_setup_session.players.selected[2] != 3
      quit 1
    end
    set $initial = g_slicks_profiles.setup[3].vehicle
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  set $expected = $initial+1
  if (signed char)$initial > 10
    set $expected = 0
  end
  if g_slicks_setup_save_report.result || g_slicks_profiles.setup[3].vehicle != $expected
    printf "VEHICLE_SHORTCUT_SAVE_FAILED\n"
    quit 1
  end
  dump binary memory .run/vehicle-shortcut-v1/saved-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1
  set $saved = 1
  printf "VEHICLE_SHORTCUT_SAVED old=%u new=%u\n",$initial,$expected
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    printf "VEHICLE_SHORTCUT_SAVE_MISSING saved=%u restore=%u\n",$saved,g_slicks_diag_restore_status
    quit 1
  end
  printf "VEHICLE_SHORTCUT_ONLY_SAVE_RESTORE_OK\n"
  quit
end
continue
