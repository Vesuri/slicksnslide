source diag_intermission_recovery_resident.gdb
set $loads = 0
set $awards = 0
set $warnings = 0
break prepare_race
commands
  silent
  set $loads = $loads+1
  if $loads != 1 || !new_game
    quit 1
  end
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  set $awards = $awards+1
  continue
end
break slicks_diag_intermission_retry
commands
  silent
  if $loads != 1 || $awards != 1 || g_slicks_diag_intermission_menu || g_slicks_diag_intermission_fault
    quit 1
  end
  set $warnings = $warnings+1
  set $warning_owned = 1
  capture_intermission_warning
  if !$_isvoid($expect_plain_close) && (g_slicks_diag_plain_close_fault || g_slicks_diag_plain_close_reached!=1)
    printf "INTERMISSION_CLOSE_FAULT_NOT_REACHED\n"
    quit 1
  end
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  printf "SKIP_UNEXPECTEDLY_OPENED_INTERMISSION\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  if $loads != 1 || $awards != 1 || $warnings != 1 || $recovery_returns != 1 || $warning_owned || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 31
    printf "INTERMISSION_SKIP_FAILED loads=%u awards=%u warnings=%u\n",$loads,$awards,$warnings
    quit 1
  end
  printf "NATIVE_INTERMISSION_FAILED_OPEN_END_MATCH_EXIT_OK\n"
  quit
end
continue
