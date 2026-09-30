set $standings = 0
set $saved = 0
set $read_errors = 0
set $save_errors = 0
set $record_inserts = 0
set $expected_save_errors = 0
source diag_records_resident.gdb
break slicks_diag_record_recovery_ready
commands
  silent
  set $record_warning_owned = 1
  if g_slicks_diag_record_results_phase == 4
    set $read_errors = $read_errors+1
    if !$_isvoid($expect_record_close) && (g_slicks_diag_plain_close_fault || g_slicks_diag_plain_close_reached!=1)
      printf "RECORD_CLOSE_FAULT_NOT_REACHED\n"
      quit 1
    end
  else
    if g_slicks_diag_record_results_phase != 5
      quit 1
    end
    set $save_errors = $save_errors+1
  end
  printf "RECORD_RECOVERY phase=%u skip=%u\n",g_slicks_diag_record_results_phase,g_slicks_diag_record_skip
  continue
end
break slicks_diag_record_results_ready
commands
  silent
  if g_slicks_diag_record_results_phase == 3
    set $record_closing = 1
  end
  if g_slicks_diag_record_results_phase == 1
    set $record_inserts = $record_inserts+1
    # A skipped first-track read leaves the one-shot save fault pending.
    # It may be reached by a qualifying second-track record, not by the skip.
    if g_slicks_diag_record_outcome.changed && (g_slicks_diag_record_faults & 2)
      set $expected_save_errors = $expected_save_errors+1
    end
    if $record_inserts == 1
      dump binary memory .run/post-race-records-v1/first.records &g_slicks_diag_record_table (char *)&g_slicks_diag_record_table+sizeof(g_slicks_diag_record_table)
    end
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $standings != 3 || g_slicks_setup_save_report.result != 0
    printf "STANDINGS_SAVE_FAILED result=%u\n",g_slicks_setup_save_report.result
    quit 1
  end
  set $saved = 1
  dump binary memory .run/post-race-records-v1/standings.profiles &g_slicks_profiles (char *)&g_slicks_profiles+sizeof(g_slicks_profiles)
  continue
end
break slicks_diag_standings_ready
commands
  silent
  if g_slicks_diag_standings_phase != $standings+1
    quit 1
  end
  set $standings = $standings+1
  printf "STANDINGS phase=%u points=%d/%d/%d/%d drivers=%u/%u/%u/%u places=%u/%u/%u/%u\n",$standings,g_slicks_diag_standings.points[0],g_slicks_diag_standings.points[1],g_slicks_diag_standings.points[2],g_slicks_diag_standings.points[3],g_slicks_diag_standings.driver[0],g_slicks_diag_standings.driver[1],g_slicks_diag_standings.driver[2],g_slicks_diag_standings.driver[3],g_slicks_diag_standings.place[0],g_slicks_diag_standings.place[1],g_slicks_diag_standings.place[2],g_slicks_diag_standings.place[3]
  if $standings == 1
    dump binary memory .run/post-race-records-v1/standings.chunky g_slicks_diag_standings_menu->renderer.ui.pixels g_slicks_diag_standings_menu->renderer.ui.pixels+64000
    dump binary memory .run/post-race-records-v1/standings.palette g_slicks_diag_standings_menu->palette g_slicks_diag_standings_menu->palette+768
    set $p0 = g_slicks_setup_session.players.selected[0]
    set $p1 = g_slicks_setup_session.players.selected[1]
    set $p2 = g_slicks_setup_session.players.selected[2]
    set $p3 = g_slicks_setup_session.players.selected[3]
    set $m0 = g_slicks_profiles.statistics[$p0][2]
    set $m1 = g_slicks_profiles.statistics[$p1][2]
    set $m2 = g_slicks_profiles.statistics[$p2][2]
    set $m3 = g_slicks_profiles.statistics[$p3][2]
    set $w0 = g_slicks_profiles.statistics[$p0][3]
    set $w1 = g_slicks_profiles.statistics[$p1][3]
    set $w2 = g_slicks_profiles.statistics[$p2][3]
    set $w3 = g_slicks_profiles.statistics[$p3][3]
  end
  if $standings == 2
    set $row = 0
    while $row < 4
      set $driver = g_slicks_diag_standings.driver[$row]
      set $profile = g_slicks_setup_session.players.selected[$driver]
      set $played = 0
      set $won = 0
      set $other = 0
      while $other < 4
        if g_slicks_setup_session.players.selected[g_slicks_diag_standings.driver[$other]] == $profile && g_slicks_diag_standings.points[$other] > 0
          set $played = $played+1
          set $won = $won+(g_slicks_diag_standings.place[$other] == 1)
        end
        set $other = $other+1
      end
      set $old_m = $driver == 0 ? $m0 : $driver == 1 ? $m1 : $driver == 2 ? $m2 : $m3
      set $old_w = $driver == 0 ? $w0 : $driver == 1 ? $w1 : $driver == 2 ? $w2 : $w3
      if g_slicks_profiles.statistics[$profile][2] != (short)($old_m+$played) || g_slicks_profiles.statistics[$profile][3] != (short)($old_w+$won)
        printf "STANDINGS_STATISTICS_FAILED driver=%u\n",$driver
        quit 1
      end
      set $row = $row+1
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $standings != 3 || !$saved || $starts != 2 || $results != 2 || $record_returns != 2 || !$title || g_slicks_diag_restore_status != 0x1f
    printf "STANDINGS_FLOW_FAILED phase=%u\n",$standings
    quit 1
  end
  if ($read_errors || $save_errors) && ($read_errors != 1 || $save_errors != $expected_save_errors || $record_inserts != (g_slicks_diag_record_skip == 2 ? 1 : 2))
    printf "RECORD_RECOVERY_FAILED reads=%u saves=%u inserts=%u\n",$read_errors,$save_errors,$record_inserts
    quit 1
  end
  if !$_isvoid($expect_record_close) && $read_errors!=1
    printf "RECORD_CLOSE_WARNING_MISSING\n"
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_STANDINGS_STATS_RESTORE_OK reads=%u saves=%u inserts=%u returns=%u\n",$read_errors,$save_errors,$record_inserts,$record_returns
  quit
end
source diag_completion_flow.gdb
