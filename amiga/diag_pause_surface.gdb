set $points = 0
set $failures = 0
set $closed = 0
set $children = 0
break slicks_diag_pause_child
commands
  silent
  set $m = g_slicks_diag_pause_menu
  set $phase = g_slicks_diag_pause_phase
  set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
  if !$m || !$m->race_menu || !g_slicks_diag_profile_platform->active || $phase != 40+($children % 8)
    quit 1
  end
  if $phase == 40 && !$m->help
    quit 1
  end
  if $phase == 42 && (!$m->controllers_dialog || $m->controllers_dialog->renderer.x != 45 || $m->controllers_dialog->renderer.y != 65)
    quit 1
  end
  if $phase == 45 && !$m->race_menu->speed_active
    quit 1
  end
  if $phase == 41 || $phase == 44 || $phase == 47
    if $m->help || $m->controllers_dialog || $m->race_menu->speed_active
      quit 1
    end
  end
  if $phase == 40
    dump binary memory .run/pause-surface-v1/help.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    dump binary memory .run/pause-surface-v1/help.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $phase == 42
    dump binary memory .run/pause-surface-v1/controllers.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    dump binary memory .run/pause-surface-v1/controllers.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $phase == 46
    dump binary memory .run/pause-surface-v1/speed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    dump binary memory .run/pause-surface-v1/speed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  set $children = $children+1
  printf "PAUSE_CHILD_PHASE %u\n", $phase
  continue
end
break slicks_diag_pause_checkpoint
commands
  silent
  set $m = g_slicks_diag_pause_menu
  set $phase = g_slicks_diag_pause_phase
  if !$m
    quit 1
  end
  if $phase < 4
    if $m->race_menu
      quit 1
    end
    set $failures = $failures+1
  else
    set $platform = g_slicks_diag_profile_platform
    set $bitmap = $platform->views[0].bitmap
    if !$platform->active || $bitmap->BytesPerRow != 320 || $bitmap->Depth != 8
      quit 1
    end
    if $phase == 17 || $phase == 27
      if $m->race_menu
        quit 1
      end
      set $closed = $closed+1
      dump binary memory .run/pause-surface-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      dump binary memory .run/pause-surface-v1/after.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    else
      if !$m->race_menu || $m->race_menu->state.redraw || $m->race_menu->state.result
        quit 1
      end
      set $expected = $phase-10
      if $phase >= 20
        set $expected = 5-($phase-20)
      end
      if $expected < 0
        set $expected = 0
      end
      if $expected > 5
        set $expected = 5
      end
      if $m->race_menu->state.row != $expected
        quit 1
      end
      if $phase == 10
        dump binary memory .run/pause-surface-v1/before.chunky $m->saved $m->saved+64000
        dump binary memory .run/pause-surface-v1/menu.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
        dump binary memory .run/pause-surface-v1/menu.palette $m->renderer.ui.palette $m->renderer.ui.palette+768
        dump binary memory .run/pause-surface-v1/menu.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
        printf "PAUSE_LABELS %s / %s\n", $m->race_menu->labels[0], $m->race_menu->labels[3]
      end
    end
  end
  set $points = $points+1
  printf "PAUSE_SURFACE_PHASE %u\n", $phase
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $points != 19 || $children != 16 || $failures != 3 || $closed != 2 || g_slicks_diag_pause_menu || g_slicks_diag_restore_status != 0x1f
    printf "PAUSE_SURFACE_FAILED points=%u failures=%u closed=%u restore=%x\n", $points, $failures, $closed, g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_PAUSE_SURFACE_REOPEN_RESTORE_FAILURES_OK\n"
  quit
end
continue
