# CHAMPSAVX/Y: startup Lock failure / injected partial next-entry failure.
source diag_menu_rectangles.gdb
set $scans=0
set $warnings=0
set $returns=0
set $starts=0
break slicks_amiga_saved_files
commands
  silent
  set $scans=$scans+1
  set $process=(struct Process *)SysBase->ThisTask
  set $window=$process->pr_WindowPtr
  set $partial=g_slicks_diag_saved_next_failure
  if $scans!=1 || (!g_slicks_diag_saved_lock_failure && !$partial)
    quit 1
  end
  continue
end
break *slicks_race_start
commands
  silent
  set $starts=$starts+1
  continue
end
break championship_notice
commands
  silent
  printf "SCAN_FAILURE_NOTICE %s\n",text
  if text[0]!=67 || text[7]!=82 || text[12]!=83 || saved_files_cache.count!=-1 || !g_slicks_diag_saved_lock_error || g_slicks_diag_saved_lock_failure || $process->pr_WindowPtr!=$window
    quit 1
  end
  set $warnings=$warnings+1
  if $partial && (g_slicks_diag_saved_next_failure || g_slicks_diag_saved_partial_count!=1)
    quit 1
  end
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if g_slicks_diag_saved_phase!=3 || !g_slicks_diag_profile_platform->active || $warnings!=1
    quit 1
  end
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $returns=$returns+1
  continue
end
break *slicks_amiga_store_saved_game
commands
  silent
  printf "SCAN_FAILURE_UNEXPECTED_STORE\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  if $scans!=1 || $warnings!=1 || $returns!=1 || $starts!=1 || g_slicks_diag_force_exit || g_slicks_diag_restore_status!=31
    quit 1
  end
  printf "SAVED_CATALOGUE_SCAN_FAILURE_RETURN_EXIT_OK io_error=%ld partial=%u\n",g_slicks_diag_saved_lock_error,g_slicks_diag_saved_partial_count
  quit
end
continue
