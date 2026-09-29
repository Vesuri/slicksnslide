set $entered = 0
set $recovered = 0
set $closed = 0
set $steps = 0
set $advanced = 0
set $warnings = 0
set $pause_owned = 0
break *slicks_amiga_platform_end
commands
  silent
  if $pause_owned
    printf "PAUSE_FAILURE_UNEXPECTED_DISPLAY_RELEASE\n"
    quit 1
  end
  continue
end
break slicks_diag_pause_warning_ready
commands
  silent
  set $warnings = $warnings+1
  if $warnings != $entered || !$p->active || $r->frame_count != $frame || $r->game_clock_ticks != $clock || status_clock.ticks != $status
    quit 1
  end
  eval "dump binary memory .run/pause-failure-v1/warning%u.chunky $pixels $pixels+64000", $warnings
  set $planes = $p->views[0].bitmap->Planes[0]
  eval "dump binary memory .run/menu-rectangles/%u.chunky $pixels $pixels+64000", $warnings
  eval "dump binary memory .run/menu-rectangles/%u.planar $planes $planes+64000", $warnings
  set $palette = help_warning.renderer.painter.ui.palette
  dump binary memory .run/pause-failure-v1/warning.palette $palette $palette+768
  continue
end
break slicks_diag_pause_live_enter
commands
  silent
  set $entered = $entered+1
  set $pause_owned = 1
  set $r = g_slicks_diag_paused_race
  set $p = g_slicks_diag_profile_platform
  if !$r || !$p->active || $entered > 6
    quit 1
  end
  set $frame = $r->frame_count
  set $clock = $r->game_clock_ticks
  set $status = status_clock.ticks
  up
  set $pixels = chunky
  set $configuration = configuration
  eval "dump binary memory .run/pause-failure-v1/before%u.race $r (char *)$r+sizeof(*$r)", $entered
  eval "dump binary memory .run/pause-failure-v1/before%u.chunky $pixels $pixels+64000", $entered
  eval "dump binary memory .run/pause-failure-v1/before%u.config $configuration (char *)$configuration+sizeof(*$configuration)", $entered
  down
  continue
end
break slicks_diag_pause_live_recovered
commands
  silent
  if $entered != $recovered+1 || $entered > 5 || !$p->active || g_slicks_diag_pause_menu || g_slicks_diag_paused_race || !g_slicks_diag_engine_started || g_slicks_diag_pause_fault
    printf "PAUSE_RECOVERY_OWNERSHIP_FAILED\n"
    quit 1
  end
  if ($entered <= 3 && g_slicks_diag_pause_unavailable != 3) || ($entered == 4 && g_slicks_diag_pause_unavailable != 1) || ($entered == 5 && g_slicks_diag_pause_unavailable != 2)
    quit 1
  end
  if status_clock.ticks != $status || status_clock_vblank != $p->vblank_count
    quit 1
  end
  eval "dump binary memory .run/pause-failure-v1/after%u.race $r (char *)$r+sizeof(*$r)", $entered
  eval "dump binary memory .run/pause-failure-v1/after%u.chunky $pixels $pixels+64000", $entered
  eval "dump binary memory .run/pause-failure-v1/after%u.config $configuration (char *)$configuration+sizeof(*$configuration)", $entered
  set $recovered = $recovered+1
  set $pause_owned = 0
  printf "PAUSE_FAILURE_RECOVERED %u\n", $recovered
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  if $recovered != 5 || $entered != 6 || !g_slicks_diag_pause_menu || !$p->active || $r->frame_count != $frame || $r->game_clock_ticks != $clock || status_clock.ticks != $status
    quit 1
  end
  set $steps = $steps+1
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  if $steps != 13 || g_slicks_diag_pause_menu->race_menu || $r->physics_tick_period != 618250
    quit 1
  end
  set $closed = 1
  set $pause_owned = 0
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    quit 1
  end
  if $closed && g_slicks_diag_race_frame == 150
    if !g_slicks_diag_engine_started || g_slicks_diag_pause_menu || $r->game_clock_ticks <= $clock
      quit 1
    end
    set $advanced = 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $recovered != 5 || $warnings != 5 || !$closed || !$advanced || g_slicks_diag_restore_status != 0x1f
    printf "PAUSE_FAILURE_GATE_FAILED\n"
    quit 1
  end
  printf "NATIVE_PAUSE_FIVE_FAILURES_RETRY_RESUME_OK\n"
  quit
end
continue
