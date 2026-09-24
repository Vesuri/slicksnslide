set $warnings = 0
set $closed = 0
set $opened = 0
set $helpclosed = 0
break slicks_diag_help_test_ready
commands
  silent
  set $m = g_slicks_options_menu
  set $kind = 0
  if !$m
    set $m = g_slicks_player_menu
    set $kind = 1
  end
  if !$m
    set $m = g_slicks_track_menu
    set $kind = 2
  end
  if !$m
    quit 1
  end
  set $s0 = g_slicks_setup_session.players.selected[0]
  set $s1 = g_slicks_setup_session.players.selected[1]
  set $s2 = g_slicks_setup_session.players.selected[2]
  set $s3 = g_slicks_setup_session.players.selected[3]
  set $profiles = g_slicks_profiles.count
  dump binary memory .run/help-failure-v1/before.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  continue
end
break slicks_diag_help_failed
commands
  silent
  if !$m || !$m->help_warning || $m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $warnings = $warnings+1
  continue
end
break slicks_diag_help_warning_closed
commands
  silent
  if $m->help_warning || $m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if g_slicks_profiles.count != $profiles || g_slicks_setup_session.players.selected[0] != $s0 || g_slicks_setup_session.players.selected[1] != $s1 || g_slicks_setup_session.players.selected[2] != $s2 || g_slicks_setup_session.players.selected[3] != $s3
    printf "HELP_FAILURE_ALTERED_SELECTIONS\n"
    quit 1
  end
  set $closed = $closed+1
  if $closed == 1
    dump binary memory .run/help-failure-v1/after-missing.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  else
    dump binary memory .run/help-failure-v1/after-allocation.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  if $closed != 2 || !$m->help || !$m->help->renderer.active || $m->help_warning
    quit 1
  end
  set $opened = $opened+1
  continue
end
break slicks_diag_help_closed
commands
  silent
  if $m->help
    quit 1
  end
  set $helpclosed = $helpclosed+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $warnings != 2 || $closed != 2 || $opened != 1 || $helpclosed != 1 || g_slicks_diag_restore_status != 0x1f
    printf "HELP_FAILURE_GATE_FAILED warnings=%d closed=%d opened=%d helpclosed=%d\n", $warnings,$closed,$opened,$helpclosed
    quit 1
  end
  printf "HELP_MISSING_ALLOCATION_DISMISS_REOPEN_RESTORE_OK context=%d\n",$kind
  quit
end
continue
