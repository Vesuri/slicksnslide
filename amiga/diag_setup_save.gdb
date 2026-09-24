break slicks_diag_setup_saved
commands
  silent
  if g_slicks_setup_save_report.result || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3]
    printf "SETUP_SAVE_FAILED result=%d count=%d\n",g_slicks_setup_save_report.result,g_slicks_profiles.count
    quit 1
  end
  if g_slicks_setup_session.players.selected[0] != 3
    printf "SETUP_SAVE_SELECTION_FAILED\n"
    quit 1
  end
  printf "SETUP_SAVE_OK ABC_SELECTED FIRST_COLOUR_EDITED\n"
  continue
end
break slicks_diag_system_restored
commands
  silent
  if g_slicks_setup_save_report.result || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "SETUP_SAVE_RESTORED\n"
  quit
end
continue
