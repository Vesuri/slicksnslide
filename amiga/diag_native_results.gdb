set $records = 0
set $native_results = 1
# One-lap persistence fixture. CONFIGDR/FUELR separately require actual
# refuelling/repair activity in their longer service regressions.
set $standings = 0
set $saved = 0
break slicks_diag_record_results_ready
commands
  silent
  if g_slicks_diag_record_results_phase != $records+1
    quit 1
  end
  set $records = $records+1
  continue
end
break slicks_diag_standings_ready
commands
  silent
  if g_slicks_diag_standings_phase != $standings+1
    quit 1
  end
  set $standings = $standings+1
  if $standings == 2
    set $i = 0
    set $wins = 0
    while $i < 4
      set $p = g_slicks_setup_session.players.selected[$i]
      if $p != $i+3 || g_slicks_profiles.statistics[$p][0] != 1 || g_slicks_profiles.statistics[$p][2] != (g_slicks_setup_session.points[$i]>0)
        printf "NATIVE_RESULTS_STATISTICS_FAILED driver=%u\n",$i
        quit 1
      end
      set $wins = $wins+g_slicks_profiles.statistics[$p][1]
      set $i = $i+1
    end
    if $wins != 1
      quit 1
    end
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $records != 3 || $standings != 3 || g_slicks_setup_save_report.result
    printf "NATIVE_RESULTS_SAVE_FAILED\n"
    quit 1
  end
  dump binary memory .run/native-results/saved.stats &g_slicks_profiles.statistics[3] &g_slicks_profiles.statistics[7]
  set $saved = 1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || $records != 3 || $standings != 3 || $starts != 1 || $results != 1 || !$title || g_slicks_diag_restore_status != 0x1f
    printf "NATIVE_RESULTS_FAILED records=%u standings=%u saved=%u restore=%x\n",$records,$standings,$saved,g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_FUEL_DAMAGE_RECORDS_STANDINGS_STATS_SAVE_RESTORE_OK\n"
  quit
end
source diag_completion_flow.gdb
