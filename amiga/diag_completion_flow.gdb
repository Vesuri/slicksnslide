# CONFIGDR, FUELR or OPTIONSB: natural results -> native Return/Next ->
# title Escape -> hardware restoration. All game input comes from the
# diagnostic's ordinary key queue; debugger expressions are read-only.
set $race = 0
set $starts = 0
set $results = 0
set $title = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $starts = $starts+1
  if $starts != $results+1 || $starts > 2
    quit 1
  end
  if (service_menu_test && ($race->fuel_option != 10 || $race->damage_scale != 300)) || (fuel_race_test && ($race->fuel_option != 10 || $race->damage_scale != 0)) || (!service_menu_test && !fuel_race_test && $race->race_mode != 5)
    printf "COMPLETION_SETUP_FAILED\n"
    quit 1
  end
  printf "COMPLETION_START track=%u mode=%d fuel=%d damage=%d\n",$starts,$race->race_mode,$race->fuel_option,$race->damage_scale
  continue
end
break slicks_diag_frame_ready if g_slicks_diag_race_error || g_slicks_diag_race_frame >= 14400
commands
  silent
  printf "COMPLETION_FLOW_FAILED frame=%u error=%u\n",g_slicks_diag_race_frame,g_slicks_diag_race_error
  quit 1
end
break slicks_diag_results_ready
commands
  silent
  if !$race || !$race->race_complete || !$race->results_drawn || !$race->finish_deadline || $race->game_clock_ticks <= $race->finish_deadline || $results+1 != $starts || g_slicks_diag_race_error || g_slicks_diag_status_failures
    printf "COMPLETION_RESULTS_FAILED\n"
    quit 1
  end
  set $i = 0
  set $finished = 0
  set $repairs = 0
  while $i < 4
    set $c = &$race->cars[$i]
    set $active = !$race->participation_ready || $race->participation[$i] != 0
    if $active
      if $c->finished != ($race->finish_ranks[$i] >= 0)
        quit 1
      end
      set $finished = $finished + $c->finished
      if $race->fuel_option && (!$c->finished || !g_slicks_diag_refuel_frames[$i])
        printf "COMPLETION_SERVICE_FAILED driver=%u\n",$i
        quit 1
      end
    end
    set $repairs = $repairs+g_slicks_diag_repair_frames[$i]
    printf "COMPLETION_CAR %u lap=%u rank=%d repaired=%u refuelled=%u\n",$i,$c->lap,$race->finish_ranks[$i],g_slicks_diag_repair_frames[$i],g_slicks_diag_refuel_frames[$i]
    set $i = $i+1
  end
  if !$finished || $finished != $race->finished_count || ($race->damage_scale == 300 && !$repairs)
    quit 1
  end
  set $results = $results+1
  printf "COMPLETION_RESULTS track=%u frame=%u ticks=%u deadline=%u finished=%u\n",$results,$race->frame_count,$race->game_clock_ticks,$race->finish_deadline,$finished
  continue
end
break redraw_title_configuration
commands
  silent
  if $results
    set $title = $title+1
    printf "COMPLETION_TITLE results=%u\n",$results
  end
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "COMPLETION_COLLISION_FAILED\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  if !$results || $results != $starts || !$title || g_slicks_diag_restore_status != 0x1f || ($race->race_mode == 5 && $starts != 2)
    printf "COMPLETION_RESTORE_FAILED starts=%u results=%u title=%u restore=%x\n",$starts,$results,$title,g_slicks_diag_restore_status
    quit 1
  end
  printf "NATURAL_COMPLETION_MENU_RESTORE_OK tracks=%u restore=%x\n",$starts,g_slicks_diag_restore_status
  quit
end
continue
