# SETUPH: dismiss a recoverable load failure, then exit through normal input.
set $race_failure_quit=1
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status != 31 || $loads != 1 || !$failed || $dismissed != 1 || $menus
    printf "RACE_FAILURE_EXIT_RESTORATION_FAILED\n"
    quit 1
  end
  printf "RACE_FAILURE_EXIT_RESTORED\n"
  quit
end
source diag_race_load_failure.gdb
