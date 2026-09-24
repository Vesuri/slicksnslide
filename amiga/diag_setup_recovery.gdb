break slicks_diag_setup_load_failed
commands
  silent
  if g_slicks_setup_load_report.result != 4 || !g_slicks_setup_load_report.path || g_slicks_setup_load_report.configuration_present || g_slicks_setup_load_report.profiles_present || g_slicks_diag_profile_platform->active || g_slicks_player_menu || g_slicks_track_menu || g_slicks_options_menu || g_slicks_diag_ingame
    printf "SETUP_RECOVERY_STARTUP_FAILED\n"
    quit 1
  end
  printf "SETUP_NATIVE_RECOVERY_REFUSED_BEFORE_MENUS path=%s\n", g_slicks_setup_load_report.path
  quit
end
break slicks_diag_frame_ready
commands
  silent
  printf "SETUP_RECOVERY_ENTERED_DISPLAY\n"
  quit 1
end
continue
