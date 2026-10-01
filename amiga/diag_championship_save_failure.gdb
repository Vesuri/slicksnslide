# Fresh empty isolated directory, SLICKS_CHAMPIONSHIP=save-fail and
# SLICKS_DEBUG_READ_ONLY=1. All input uses native raw-key queues.
source diag_saved_resident.gdb
set $pickers = 0
set $names = 0
set $warnings = 0
set $returns = 0
set $starts = 0
break slicks_race_start
commands
  silent
  set $starts = $starts+1
  continue
end
break championship_notice
commands
  silent
  printf "SAVE_FAILURE_NOTICE %s\n",text
  if text[0] != 83 || text[1] != 65 || text[2] != 86 || text[3] != 69 || text[5] != 70
    printf "UNEXPECTED_SAVE_FAILURE_NOTICE\n"
    quit 1
  end
  set $warnings = $warnings+1
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if saved_files_cache.count || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if g_slicks_diag_saved_phase == 1
    set $pickers = $pickers+1
    if $saved_enumerations != 1+($pickers==3)
      quit 1
    end
  end
  if g_slicks_diag_saved_phase == 2
    set $names = $names+1
  end
  if g_slicks_diag_saved_phase == 3
    if $warnings != 1 || $saved_enumerations != 2 || $saved_process->pr_WindowPtr != $saved_window
      quit 1
    end
  end
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $returns = $returns+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $pickers != 3 || $names != 1 || $warnings != 1 || $returns != 2 || $starts != 1 || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 31 || g_slicks_menu_workspace_conflicts
    printf "CHAMPIONSHIP_SAVE_FAILURE_GATE_FAILED pickers=%u names=%u warnings=%u returns=%u starts=%u\n",$pickers,$names,$warnings,$returns,$starts
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_SAVE_FAILURE_WARNING_REOPEN_CANCEL_EXIT_OK\n"
  quit
end
continue
