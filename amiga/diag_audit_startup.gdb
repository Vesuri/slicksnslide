# Informational audit of startup; no target-state writes.
# SLICKS_AUTO_RACE=1 SLICKS_DEBUG_WARP=1 amiga/debug.sh '' diag_audit_startup.gdb
set $race = (struct SlicksRaceRuntime *)0
set $cues = 0
set $polled = 0
break *start_race_engines
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+8)
  continue
end
break *'play_effect_period.part.0'
commands
  silent
  if $race && *(unsigned long *)($sp+8) == 1
    set $cues = $cues + 1
    printf "STARTUP_AUDIT cue=%u frame=%lu light_stage=%u light_mask=%u racing=%u\n", $cues, $race->frame_count, $race->countdown_stage, $race->start_light_stage_mask, $race->racing
  end
  continue
end
break *poll_driver_devices
commands
  silent
  if $race && !$polled
    if $race->frame_count != 0 || $race->game_clock_ticks != 1 || $race->boundary_timer != 119 || $cues != 4
      printf "STARTUP_FIRST_POLL_FAILED\n"
      quit 1
    end
    set $polled = 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $race && $race->racing && $race->frame_count >= 1
    if ($race->poll_driver_devices && !$polled) || $race->frame_count != 1 || $race->game_clock_ticks != 1 || $race->cars[0].elapsed_time_units != 2
      printf "STARTUP_FIRST_UPDATE_FAILED polls=%u frame=%lu clock=%lu elapsed=%lu\n",$polled,$race->frame_count,$race->game_clock_ticks,$race->cars[0].elapsed_time_units
      quit 1
    end
    printf "STARTUP_AUDIT drive_enabled frame=%lu clock_ticks=%lu car0_elapsed=%lu cues=%u\n", $race->frame_count, $race->game_clock_ticks, $race->cars[0].elapsed_time_units, $cues
    quit
  end
  continue
end
continue
printf "STARTUP_AUDIT_UNEXPECTED_STOP\n"
quit 1
