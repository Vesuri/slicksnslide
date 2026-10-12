# Informational audit of startup; no target-state writes.
# SLICKS_AUTO_RACE=1 SLICKS_DEBUG_WARP=1 amiga/debug.sh '' diag_audit_startup.gdb
set $race = (struct SlicksRaceRuntime *)0
set $cues = 0
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
break slicks_diag_frame_ready
commands
  silent
  if $race && $race->racing
    printf "STARTUP_AUDIT drive_enabled frame=%lu clock_ticks=%lu car0_elapsed=%lu cues=%u\n", $race->frame_count, $race->game_clock_ticks, $race->cars[0].elapsed_time_units, $cues
    quit
  end
  continue
end
continue
printf "STARTUP_AUDIT_UNEXPECTED_STOP\n"
quit 1
