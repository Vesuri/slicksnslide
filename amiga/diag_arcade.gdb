set $race = 0
set $edited = 0
set $expired = 0
set $timer_drawn = 0
set $last_drawn = 0
set $lap_drawn = 0
set $finish_seen = 0
break slicks_diag_options_closed
commands
  silent
  if g_slicks_options_configuration->options[0] != 5 || g_slicks_options_configuration->options[13] != 5
    printf "ARCADE_MENU_EDIT_FAILED\n"
    quit 1
  end
  set $edited = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if !$edited || $race->race_mode != 5 || $race->arcade_seconds != 5
    printf "ARCADE_SETUP_HANDOFF_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $race
    if g_slicks_diag_race_error
      quit 1
    end
    if !$race->arcade_hud_valid
      printf "ARCADE_HUD_NOT_DRAWN\n"
      quit 1
    end
    if $race->racing && $race->game_clock_ticks < 450
      if $race->arcade_hud_text || $race->arcade_hud_count != 2 || $race->arcade_bar_top != 189
        quit 1
      end
      if !$timer_drawn
        dump binary memory .run/arcade-v1/timer.chunky $race->chunky $race->chunky+64000
        set $timer_drawn = 1
      end
    end
    if $race->game_clock_ticks >= 450 && $race->game_clock_ticks < 720 && !$last_drawn
      if $race->arcade_hud_text != 1
        quit 1
      end
      dump binary memory .run/arcade-v1/last.chunky $race->chunky $race->chunky+64000
      set $last_drawn = 1
    end
    if $race->game_clock_ticks >= 720 && $race->game_clock_ticks < 990 && !$lap_drawn
      if $race->arcade_hud_text != 2
        quit 1
      end
      dump binary memory .run/arcade-v1/lap.chunky $race->chunky $race->chunky+64000
      set $lap_drawn = 1
    end
    if $race->game_clock_ticks < 450 && $race->laps_to_run != 9999
      printf "ARCADE_PREMATURE_LIMIT_FAILED\n"
      quit 1
    end
    if $race->game_clock_ticks >= 450 && $race->laps_to_run != 9999
      set $expired = 1
    end
    if $expired && $race->finished_count && !$finish_seen
      if !$timer_drawn || !$last_drawn || !$lap_drawn || !$race->finish_deadline || $race->arcade_bar_top != 196
        printf "ARCADE_DISPLAY_PHASE_FAILED\n"
        quit 1
      end
      dump binary memory .run/arcade-v1/finish.chunky $race->chunky $race->chunky+64000
      set $finish_seen = 1
      if $race->finished_count < 4 && $race->finish_deadline != $race->game_clock_ticks+1800
        printf "ARCADE_ACTIVE_AI_GRACE_FAILED\n"
        quit 1
      end
    end
    if $race->race_complete
      if !$finish_seen || $race->game_clock_ticks <= $race->finish_deadline
        printf "ARCADE_PREMATURE_RACE_END\n"
        quit 1
      end
      printf "ARCADE_NATIVE_MENU_DISPLAY_GRACE_RACE_END_OK ticks=%u deadline=%u finished=%u\n", $race->game_clock_ticks,$race->finish_deadline,$race->finished_count
      quit
    end
    if $race->frame_count > 3000
      printf "ARCADE_NATIVE_TIMEOUT ticks=%u target=%u finished=%u\n", $race->game_clock_ticks,$race->laps_to_run,$race->finished_count
      quit 1
    end
  end
  continue
end
continue
