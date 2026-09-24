set $ready = 0
set $cancelled = 0
set $writes = 0
break slicks_amiga_clear_track_records
commands
  silent
  if !$cancelled || $ready != 2
    printf "CLEAR_WITHOUT_CONFIRMATION\n"
    quit 1
  end
  set $writes = $writes+1
  continue
end
break slicks_diag_track_clear_cancelled
commands
  silent
  if $ready != 1 || $writes || g_slicks_track_clear_phase || g_slicks_options_menu->message
    quit 1
  end
  set $cancelled = 1
  printf "CLEAR_CANCEL_NO_WRITES_OK\n"
  continue
end
break slicks_diag_track_clear_ready
commands
  silent
  set $ready = $ready+1
  if !g_slicks_options_menu->message || !g_slicks_diag_profile_platform->active
    quit 1
  end
  printf "CLEAR_READY %u phase=%u changed=%u writes=%u\n", $ready, g_slicks_track_clear_phase, g_slicks_track_clear_changed, $writes
  if $ready < 3
    if g_slicks_track_clear_phase != 1 || $writes
      quit 1
    end
    if $ready == 1
      set $ui = &g_slicks_options_menu->renderer.ui
      dump binary memory .run/clear-records-v1/question.chunky $ui->pixels $ui->pixels+64000
      dump binary memory .run/clear-records-v1/question.palette $ui->palette $ui->palette+768
    end
  else
    if g_slicks_track_clear_phase != 2 || g_slicks_track_clear_report.result || $writes != g_slicks_diag_track_files
      quit 1
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 3 || !$cancelled || !$writes || g_slicks_diag_restore_status != 0x1f
    printf "CLEAR_INCOMPLETE ready=%u cancelled=%u writes=%u phase=%u restore=%x\n", $ready, $cancelled, $writes, g_slicks_track_clear_phase, g_slicks_diag_restore_status
    quit 1
  end
  printf "CLEAR_RECORDS_NATIVE_CANCEL_CONFIRM_RESTORE_OK\n"
  quit
end
continue
