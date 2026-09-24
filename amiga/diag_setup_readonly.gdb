set $failed = 0
set $cancelled = 0
break slicks_amiga_store_setup
commands
  silent
  set $process = (struct Process *)SysBase->ThisTask
  set $window = $process->pr_WindowPtr
  continue
end
break slicks_diag_setup_save_failed
commands
  silent
  if g_slicks_setup_save_report.result != 1 || !g_slicks_setup_save_report.io_error || !g_slicks_diag_profile_platform->active || g_slicks_profiles.count != 4 || $process->pr_WindowPtr != $window
    printf "SETUP_READ_ONLY_FAILURE_SCREEN_FAILED\n"
    quit 1
  end
  set $failed = 1
  printf "SETUP_READ_ONLY_WARNING io_error=%ld\n", g_slicks_setup_save_report.io_error
  continue
end
break slicks_diag_setup_save_cancelled
commands
  silent
  if !$failed || !g_slicks_diag_profile_platform->active || g_slicks_player_menu
    quit 1
  end
  set $cancelled = 1
  continue
end
break slicks_diag_player_menu_ready
commands
  silent
  if $cancelled
    if !g_slicks_player_menu || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3] || g_slicks_setup_session.players.selected[0] != 3
      printf "SETUP_READ_ONLY_LOST_EDITS\n"
      quit 1
    end
    printf "SETUP_NATIVE_READ_ONLY_FAILURE_CANCEL_REOPEN_EDITS_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "SETUP_READ_ONLY_EARLY_EXIT\n"
  quit 1
end
continue
