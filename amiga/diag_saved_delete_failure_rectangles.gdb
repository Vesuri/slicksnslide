source diag_menu_rectangles.gdb
source diag_saved_resident.gdb
set $pickers = 0
set $warnings = 0
set $returns = 0
break championship_notice
commands
  silent
  set $warnings = $warnings+1
  printf "DELETE_FAILURE_NOTICE %s\n", text
  if $warnings == 2 && (text[0] != 68 || text[7] != 70)
    quit 1
  end
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if saved_files_cache.count != 1 || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if g_slicks_diag_saved_phase == 2
    printf "DELETE_FAILURE_UNEXPECTED_NAME_DIALOG\n"
    quit 1
  end
  if g_slicks_diag_saved_phase == 1
    set $pickers = $pickers+1
    if $saved_enumerations != $pickers
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
  if $pickers != 2 || $warnings != 2 || $returns != 1 || $saved_deletes != 1 || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 31
    printf "DELETE_FAILURE_GATE_FAILED pickers=%u warnings=%u returns=%u deletes=%u\n", $pickers, $warnings, $returns, $saved_deletes
    quit 1
  end
  printf "NATIVE_DELETE_BLOCKED_WARNING_REOPEN_CANCEL_OK\n"
  quit
end
continue
