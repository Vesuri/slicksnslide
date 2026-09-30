set $attempts = 0
set $failed = 0
set $saved = 0
break slicks_amiga_store_setup
commands
  silent
  set $attempts = $attempts+1
  if !buffer || buffer_size<5771 || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
    quit 1
  end
  if $attempts > 2
    printf "SETUP_RETRY_LOOP_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $attempts == 1
    if g_slicks_setup_save_report.result != 1 || g_slicks_setup_save_report.io_error != 212
      printf "SETUP_RETRY_ERROR_CLASS_FAILED\n"
      quit 1
    end
  else
    if !$failed || g_slicks_setup_save_report.result || g_slicks_profiles.count != 4 || g_slicks_setup_session.players.selected[0] != 3
      printf "SETUP_RETRY_SAVE_FAILED result=%d io=%ld\n",g_slicks_setup_save_report.result,g_slicks_setup_save_report.io_error
      quit 1
    end
    set $saved = 1
  end
  continue
end
break slicks_diag_setup_save_failed
commands
  silent
  if $attempts != 1 || !g_slicks_diag_profile_platform->active || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65
    printf "SETUP_RETRY_LIVE_STATE_FAILED\n"
    quit 1
  end
  set $failed = 1
  printf "SETUP_RETRY_FAILURE_SCREEN_OK\n"
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || $attempts != 2 || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "SETUP_RETRY_OK FAILURE_RETAINED_STATE_RETRY_SAVED_RESTORED\n"
  quit
end
continue
