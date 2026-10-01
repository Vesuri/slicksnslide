# Isolated fixture: make TRACKS/66.SS read-only on the host before this run.
# Current sorted catalogue: 1WAY.SS commits first, then the 66.SS write fails.
set $ready = 0
set $cancelled = 0
set $writes = 0
break slicks_amiga_clear_track_records
commands
  silent
  if !$cancelled || $ready != 2
    quit 1
  end
  set $writes = $writes+1
  if !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
    printf "CLEAR_DISK_DISPLAY_OR_SERVICE_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_track_clear_cancelled
commands
  silent
  if $ready != 1 || $writes || g_slicks_track_clear_phase
    quit 1
  end
  set $cancelled = 1
  continue
end
break slicks_diag_track_clear_ready
commands
  silent
  set $ready = $ready+1
  if !g_slicks_options_menu->message || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $ready < 3
    if g_slicks_track_clear_phase != 1 || $writes
      quit 1
    end
  else
    if g_slicks_track_clear_phase != $ready || g_slicks_track_clear_report.result != 1 || $writes != 2 || g_slicks_track_clear_changed != 1
      printf "CLEAR_FAILURE_WRONG_RESULT phase=%u status=%u writes=%u changed=%u\n", g_slicks_track_clear_phase, g_slicks_track_clear_report.result, $writes, g_slicks_track_clear_changed
      quit 1
    end
    printf "CLEAR_FAILURE_DISPLAY phase=%u path=%s\n", g_slicks_track_clear_phase, g_slicks_track_clear_report.path
    if $ready == 3
      set $ui = &g_slicks_options_menu->renderer.ui
      dump binary memory .run/clear-records-failure-v1/failure.chunky $ui->pixels $ui->pixels+64000
      dump binary memory .run/clear-records-failure-v1/failure.palette $ui->palette $ui->palette+768
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$_isvoid($clear_io_windows)
    if $clear_io_windows!=1
      quit 1
    end
  end
  if $ready != 4 || !$cancelled || $writes != 2 || g_slicks_track_clear_phase || g_slicks_diag_restore_status != 0x1f
    printf "CLEAR_FAILURE_INCOMPLETE ready=%u writes=%u phase=%u restore=%x\n", $ready, $writes, g_slicks_track_clear_phase, g_slicks_diag_restore_status
    quit 1
  end
  printf "CLEAR_RECORDS_NATIVE_PARTIAL_FAILURE_PRESERVED_OK\n"
  quit
end
continue
