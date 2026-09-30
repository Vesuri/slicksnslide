set $retries = 0
set $expected_retries = 1
source diag_intermission_recovery_resident.gdb
break slicks_diag_intermission_retry
commands
  silent
  set $retries = $retries+1
  set $warning_owned = 1
  if !$_isvoid($expect_plain_close) && (g_slicks_diag_plain_close_fault || g_slicks_diag_plain_close_reached!=1)
    printf "INTERMISSION_CLOSE_FAULT_NOT_REACHED\n"
    quit 1
  end
  if $retries != 1 || $awards != 1 || $loads != 1 || $starts != 1 || g_slicks_diag_intermission_menu || g_slicks_diag_intermission_fault
    quit 1
  end
  dump binary memory .run/pause-transitions-v1/retry.before &g_slicks_setup_session (char *)&g_slicks_setup_session+sizeof(g_slicks_setup_session)
  printf "INTERMISSION_FAILED_OPEN_RETRY_NOTICE\n"
  continue
end
source diag_pause_transitions.gdb
