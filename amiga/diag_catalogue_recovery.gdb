# CATRECOV against the actual 10001-file/2-MiB low-memory fixture. No injected OOM.
set $attempts=0
set $failures=0
set $dismissals=0
break *prepare_race
commands
  silent
  set $attempts=$attempts+1
  if $attempts>2 || g_slicks_track_playlist.count!=10000
    printf "CATALOGUE_MEMORY_ATTEMPT_FAILED attempts=%u count=%u\n",$attempts,g_slicks_track_playlist.count
    quit 1
  end
  if $attempts==1
    dump binary memory .run/catalogue-recovery/session-before-1.bin &g_slicks_setup_session &g_slicks_setup_session+1
    dump binary memory .run/catalogue-recovery/config-before-1.bin title_configuration title_configuration+1
  else
    dump binary memory .run/catalogue-recovery/session-before-2.bin &g_slicks_setup_session &g_slicks_setup_session+1
    dump binary memory .run/catalogue-recovery/config-before-2.bin title_configuration title_configuration+1
  end
  continue
end
break slicks_diag_race_load_failed
commands
  silent
  set $failures=$failures+1
  if $failures!=$attempts || g_slicks_diag_race_error!=1 || g_slicks_diag_race_allocation_failures!=4 || g_slicks_diag_race_load_fault || g_slicks_diag_ingame || !g_slicks_diag_profile_platform->active
    printf "CATALOGUE_MEMORY_RECOVERY_FAILED\n"
    quit 1
  end
  if $failures==1
    dump binary memory .run/catalogue-recovery/session-after-1.bin &g_slicks_setup_session &g_slicks_setup_session+1
    dump binary memory .run/catalogue-recovery/config-after-1.bin title_configuration title_configuration+1
  else
    dump binary memory .run/catalogue-recovery/session-after-2.bin &g_slicks_setup_session &g_slicks_setup_session+1
    dump binary memory .run/catalogue-recovery/config-after-2.bin title_configuration title_configuration+1
  end
  printf "CATALOGUE_MEMORY_RECOVERED attempt=%u allocation_failures=%u\n",$attempts,g_slicks_diag_race_allocation_failures
  continue
end
break slicks_diag_race_load_dismissed
commands
  silent
  set $dismissals=$dismissals+1
  if $dismissals!=$failures || !g_slicks_diag_profile_platform->active
    printf "CATALOGUE_MEMORY_DISMISS_FAILED dismissals=%u failures=%u active=%u\n",$dismissals,$failures,g_slicks_diag_profile_platform->active
    quit 1
  end
  continue
end
break slicks_race_start
commands
  silent
  printf "CATALOGUE_MEMORY_UNEXPECTED_RACE\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  if $attempts!=2 || $failures!=2 || $dismissals!=2 || g_slicks_diag_restore_status!=31
    printf "CATALOGUE_MEMORY_EXIT_FAILED attempts=%u failures=%u dismissals=%u restore=%u\n",$attempts,$failures,$dismissals,g_slicks_diag_restore_status
    quit 1
  end
  printf "CATALOGUE_MEMORY_FAILURE_RETRY_DISMISS_EXIT_OK\n"
  quit
end
continue
