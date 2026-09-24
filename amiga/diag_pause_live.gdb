set $steps = 0
set $closed = 0
set $advanced = 0
break slicks_diag_pause_live_ready
commands
  silent
  set $m = g_slicks_diag_pause_menu
  set $r = g_slicks_diag_paused_race
  set $p = g_slicks_diag_profile_platform
  if !$m || !$r || !$p->active || $steps > 12
    quit 1
  end
  if $steps == 0
    set $frame = $r->frame_count
    set $clock = $r->game_clock_ticks
    set $status = status_clock.ticks
    set $vblank = $p->vblank_count
    dump binary memory .run/pause-live-v1/before.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
    dump binary memory .run/pause-live-v1/before.chunky $m->saved $m->saved+64000
  end
  if $r->frame_count != $frame || $r->game_clock_ticks != $clock || status_clock.ticks != $status
    printf "PAUSE_ADVANCED_RACE\n"
    quit 1
  end
  if $steps == 2 && !$m->help
    quit 1
  end
  if $steps == 5 && !$m->controllers_dialog
    quit 1
  end
  if $steps == 11 && (!$m->race_menu->speed_active || $m->race_menu->speed.displayed != 106)
    quit 1
  end
  printf "LIVE_PAUSE_STEP %u\n", $steps
  set $steps = $steps+1
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  set $m = g_slicks_diag_pause_menu
  if $steps != 13 || $m->race_menu || $r->frame_count != $frame || $r->game_clock_ticks != $clock || $r->physics_tick_period != 618250 || $r->physics_tick_phase
    quit 1
  end
  if driver_device_configuration_storage.field_05de != 106 || g_slicks_diag_profile_platform->vblank_count <= $vblank
    quit 1
  end
  dump binary memory .run/pause-live-v1/after.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
  dump binary memory .run/pause-live-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  set $closed = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "LIVE_PAUSE_RACE_ERROR %u\n", g_slicks_diag_race_error
    quit 1
  end
  if $closed && g_slicks_diag_race_frame == 150
    if $r->game_clock_ticks != $clock+96 || !g_slicks_diag_engine_started || g_slicks_diag_pause_menu
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
    printf "LIVE_PAUSE_FAILED steps=%u closed=%u advanced=%u\n", $steps, $closed, $advanced
    quit 1
  end
  printf "NATIVE_LIVE_PAUSE_CHILDREN_SPEED_RESUME_OK\n"
  quit
end
continue
