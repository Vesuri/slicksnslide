set $race = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break draw_results
commands
  silent
  if !$race || !$race->finish_ranks_ready || !$race->race_complete || $race->game_clock_ticks <= $race->finish_deadline || g_slicks_diag_race_error
    printf "NATIVE_FINISH_RANK_RACE_FAILED\n"
    quit 1
  end
  set $i = 0
  set $count = 0
  while $i < 4
    if $race->participation[$i]
      if $race->cars[$i].finished != ($race->finish_ranks[$i] >= 0)
        quit 1
      end
      if $race->cars[$i].finished
        if $race->cars[$i].finish_position != $race->finish_ranks[$i]
          quit 1
        end
        set $count = $count+1
      end
    end
    set $i = $i+1
  end
  if $count != $race->finished_count || !$count
    quit 1
  end
  printf "NATIVE_ORIGINAL_FINISH_RANKS_RACE_END_OK ranks=%d,%d,%d,%d finished=%u ticks=%u\n",$race->finish_ranks[0],$race->finish_ranks[1],$race->finish_ranks[2],$race->finish_ranks[3],$count,$race->game_clock_ticks
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "FINISH_RANK_EARLY_EXIT\n"
  quit 1
end
continue
