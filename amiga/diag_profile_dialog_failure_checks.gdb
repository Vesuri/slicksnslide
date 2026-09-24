set $failures = 0
set $dismissals = 0
break *slicks_amiga_name_dialog_open
commands
  silent
  if g_slicks_diag_profile_dialog_fault
    eval "dump binary memory %s/before.chunky g_slicks_player_menu->renderer.ui.pixels g_slicks_player_menu->renderer.ui.pixels+64000", $failure_run
    eval "dump binary memory %s/before.profiles &g_slicks_profiles &g_slicks_profiles+1", $failure_run
  end
  continue
end
break *slicks_amiga_colour_dialog_open
commands
  silent
  if g_slicks_diag_profile_dialog_fault
    eval "dump binary memory %s/before.chunky g_slicks_player_menu->renderer.ui.pixels g_slicks_player_menu->renderer.ui.pixels+64000", $failure_run
    eval "dump binary memory %s/before.profiles &g_slicks_profiles &g_slicks_profiles+1", $failure_run
  end
  continue
end
break slicks_diag_profile_dialog_failed
commands
  silent
  set $failures = $failures+1
  if $failures != 1 || !g_slicks_player_menu->help_warning || !g_slicks_player_menu->editor_active || g_slicks_player_menu->name_dialog || g_slicks_player_menu->colour_dialog || g_slicks_player_menu->error
    printf "PROFILE_DIALOG_FAILURE_RECOVERY_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_help_warning_closed
commands
  silent
  set $dismissals = $dismissals+1
  if $failures != 1 || $dismissals != 1 || g_slicks_player_menu->help_warning || !g_slicks_player_menu->editor_active
    quit 1
  end
  eval "dump binary memory %s/after.chunky g_slicks_player_menu->renderer.ui.pixels g_slicks_player_menu->renderer.ui.pixels+64000", $failure_run
  eval "dump binary memory %s/after.profiles &g_slicks_profiles &g_slicks_profiles+1", $failure_run
  printf "PROFILE_DIALOG_FAILURE_DISMISSED_EDITOR_RETAINED\n"
  continue
end
