set $record_starts = 0
set $record_shows = 0
set $record_finishes = 0
break slicks_diag_record_results_ready
commands
  silent
  if g_slicks_diag_record_results_phase == 1
    set $record_starts = $record_starts+1
    if $record_starts > 2 || $record_starts != $record_finishes+1
      quit 1
    end
    printf "RECORD_INSERT track=%u show=%u changed=%u ranks=%d/%d/%d/%d\n",$record_starts,g_slicks_diag_record_outcome.show,g_slicks_diag_record_outcome.changed,g_slicks_diag_record_outcome.ranks[0],g_slicks_diag_record_outcome.ranks[1],g_slicks_diag_record_outcome.ranks[2],g_slicks_diag_record_outcome.ranks[3]
    if $record_starts == 1
      dump binary memory .run/post-race-records-v1/first.records &g_slicks_diag_record_table (char *)&g_slicks_diag_record_table+sizeof(g_slicks_diag_record_table)
    else
      dump binary memory .run/post-race-records-v1/second.records &g_slicks_diag_record_table (char *)&g_slicks_diag_record_table+sizeof(g_slicks_diag_record_table)
    end
  end
  if g_slicks_diag_record_results_phase == 2
    set $record_shows = $record_shows+1
    if !g_slicks_diag_record_outcome.show || $record_shows != $record_starts
      quit 1
    end
    printf "RECORD_DISPLAY track=%u\n",$record_shows
  end
  if g_slicks_diag_record_results_phase == 3
    set $record_finishes = $record_finishes+1
    if $record_finishes != $record_starts || (g_slicks_diag_record_outcome.changed && g_slicks_diag_record_save.result != 0)
      printf "RECORD_SAVE_FAILED result=%u\n",g_slicks_diag_record_save.result
      quit 1
    end
    printf "RECORD_SAVED track=%u changed=%u\n",$record_finishes,g_slicks_diag_record_outcome.changed
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $record_starts != 2 || $record_shows != 2 || $record_finishes != 2 || $starts != 2 || $results != 2 || !$title || g_slicks_diag_restore_status != 0x1f
    printf "RECORD_FLOW_FAILED starts=%u shows=%u finishes=%u\n",$record_starts,$record_shows,$record_finishes
    quit 1
  end
  printf "NATIVE_POST_RACE_RECORDS_TWO_TRACKS_OK restore=%x\n",g_slicks_diag_restore_status
  quit
end
source diag_completion_flow.gdb
