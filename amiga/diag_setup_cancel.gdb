set $attempts = 0
set $failed = 0
set $cancelled = 0
set $reopened = 0
set $saved = 0
set $ends = 0
break *slicks_amiga_platform_end
commands
  silent
  set $ends = $ends+1
  printf "SETUP_PLATFORM_END %u\n",$ends
  continue
end
break slicks_amiga_store_setup
commands
  silent
  set $attempts = $attempts+1
  if !buffer || buffer_size<5771 || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
    quit 1
  end
  if $attempts > 2
    quit 1
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $attempts == 1
    if g_slicks_setup_save_report.result != 1 || g_slicks_setup_save_report.io_error != 212
      quit 1
    end
  else
    if !$cancelled || !$reopened || g_slicks_setup_save_report.result || g_slicks_profiles.count != 4 || g_slicks_setup_session.players.selected[0] != 3
      quit 1
    end
    set $saved = 1
  end
  continue
end
break slicks_diag_setup_save_failed
commands
  silent
  if $attempts != 1 || !g_slicks_diag_profile_platform->active || g_slicks_profiles.count != 4
    quit 1
  end
  dump binary memory .run/setup-cancel-v1/failed-profiles.bin &g_slicks_profiles &g_slicks_profiles+1
  dump binary memory .run/setup-cancel-v1/failed-session.bin &g_slicks_setup_session &g_slicks_setup_session+1
  set $failed = 1
  set $failure_ends = $ends
  continue
end
break slicks_diag_setup_save_cancelled
commands
  silent
  # The injected write fault leaves no file to clean up.
  if $ends != $failure_ends || $ends
    printf "SAVE_CANCEL_UNEXPECTED_TEARDOWN ends=%u at_failure=%u\n",$ends,$failure_ends
    quit 1
  end
  if !$failed || $attempts != 1 || !g_slicks_diag_profile_platform->active || g_slicks_player_menu || g_slicks_options_menu || g_slicks_title_help || g_slicks_options_renderer.surface || g_slicks_options_configuration
    quit 1
  end
  dump binary memory .run/setup-cancel-v1/cancelled-profiles.bin &g_slicks_profiles &g_slicks_profiles+1
  dump binary memory .run/setup-cancel-v1/cancelled-session.bin &g_slicks_setup_session &g_slicks_setup_session+1
  set $cancelled = 1
  printf "SAVE_CANCEL_RETURNED_TO_TITLE\n"
  continue
end
break slicks_diag_player_menu_ready
commands
  silent
  if $cancelled
    if !g_slicks_player_menu || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3] || g_slicks_setup_session.players.selected[0] != 3
      quit 1
    end
    set $reopened = 1
    printf "SAVE_CANCEL_PLAYERS_REOPENED_WITH_EDITS\n"
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || $attempts != 2 || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "SETUP_CANCEL_OK RETURN_REOPEN_SAVE_RESTORED\n"
  quit
end
continue
