set $saved = 0
break slicks_diag_setup_saved
commands
  silent
  set $p = &g_slicks_setup_session.players
  if g_slicks_setup_save_report.result || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 67 || g_slicks_profiles.names[3][3]
    quit 1
  end
  if $p->selected[0] != 3 || $p->selected[1] != 1 || $p->selected[2] != 0 || $p->selected[3] != 1 || $p->count != 3
    quit 1
  end
  if $p->participation[0] != -1 || $p->participation[1] != 1 || $p->participation[2] != 0 || $p->participation[3] != 1 || !(g_slicks_profiles.setup[1].flags & 2)
    quit 1
  end
  dump binary memory .run/setup-mixed-v1/saved-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1
  set $saved = 1
  printf "MIXED_SETUP_SAVED HUMAN_SHARED_AI_INACTIVE\n"
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "MIXED_SETUP_SAVE_RESTORED\n"
  quit
end
continue
