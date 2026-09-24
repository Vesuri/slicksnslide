set $steps = 0
set $closed = 0
set $advanced = 0
break slicks_diag_pause_live_ready
commands
  silent
  set $m = g_slicks_diag_pause_menu
  set $r = g_slicks_diag_paused_race
  if !$m || !$r || !g_slicks_diag_profile_platform->active || $steps > 12
    quit 1
  end
  if $steps == 0
    set $clock = $r->game_clock_ticks
    set $status = status_clock.ticks
    set $font0 = $m->fonts[0][6]
    set $font1 = $m->fonts[1][6]
    dump binary memory .run/pause-nested-v1/before.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
    dump binary memory .run/pause-nested-v1/before.chunky $m->saved $m->saved+64000
  end
  if $r->frame_count != 100 || $r->game_clock_ticks != $clock || status_clock.ticks != $status || g_slicks_diag_controllers_fault || g_slicks_diag_help_fail_allocation
    quit 1
  end
  if $steps == 2 || $steps == 7 || $steps == 9
    if !$m->help_warning || $m->help || $m->controllers_dialog || $m->message
      printf "NESTED_FAILURE_WARNING_MISSING\n"
      quit 1
    end
    eval "dump binary memory .run/pause-nested-v1/warning%u.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000", $steps
  else
    if $m->help_warning
      quit 1
    end
  end
  if $steps == 4 && !$m->help
    quit 1
  end
  if $steps == 11 && !$m->controllers_dialog
    quit 1
  end
  if $steps == 1 || $steps == 3 || $steps == 5 || $steps == 6 || $steps == 8 || $steps == 10 || $steps == 12
    if $m->help || $m->controllers_dialog || $m->fonts[0][6] != $font0 || $m->fonts[1][6] != $font1
      printf "NESTED_FAILURE_PARENT_NOT_RESTORED\n"
      quit 1
    end
    eval "dump binary memory .run/pause-nested-v1/parent%u.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000", $steps
  end
  set $steps = $steps+1
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  if $steps != 13 || $m->race_menu || $m->help_warning || $m->help || $m->controllers_dialog
    quit 1
  end
  dump binary memory .run/pause-nested-v1/after.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
  dump binary memory .run/pause-nested-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  set $closed = 1
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
  if !$closed || !$advanced || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "NATIVE_PAUSE_NESTED_FAILURES_RETRY_RESUME_OK\n"
  quit
end
continue
