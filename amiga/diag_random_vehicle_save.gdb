set $saved = 0
break slicks_diag_setup_saved
commands
  silent
  if g_slicks_setup_save_report.result || g_slicks_profiles.count != 4 || g_slicks_profiles.setup[3].vehicle != 10 || g_slicks_profiles.setup[1].vehicle != 11
    quit 1
  end
  if g_slicks_setup_session.players.selected[2] != 3
    quit 1
  end
  set $saved = 1
  printf "RANDOM_ONCE_PROFILE_NATIVE_SAVE_OK\n"
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  quit
end
continue
