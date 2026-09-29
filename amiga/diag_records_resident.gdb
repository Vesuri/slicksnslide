set $record_owner = 0
set $record_warning_owned = 0
set $record_closing = 0
set $record_returns = 0
break run_record_results
commands
  silent
  set $record_owner = 1
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $record_owner
    printf "RECORD_RESULT_ARCHIVE_READ\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $record_warning_owned || $record_closing
    printf "RECORD_RAM_CLOSE_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_emergency_warning_close
commands
  silent
  if $record_warning_owned
    if !g_slicks_diag_profile_platform->active
      quit 1
    end
    set $record_warning_owned = 0
  end
  continue
end
break slicks_amiga_store_track_records
commands
  silent
  if g_slicks_diag_profile_platform->active
    printf "RECORD_WRITE_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_diag_record_results_closed
commands
  silent
  if !$record_owner || $record_warning_owned || !g_slicks_diag_profile_platform->active
    printf "RECORD_RETURN_FAILED\n"
    quit 1
  end
  set $record_owner = 0
  set $record_closing = 0
  set $record_returns = $record_returns+1
  continue
end
