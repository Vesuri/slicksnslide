set $ready = 0
set $closed = 0
break slicks_diag_help_ready
commands
  silent
  set $m = g_slicks_options_menu
  if !$m
    set $m = g_slicks_player_menu
  end
  set $v = $m->help
  if !$v || !$v->renderer.active || $v->navigation.done || !g_slicks_diag_profile_platform->active
    printf "HELP_OPEN_FAILED\n"
    quit 1
  end
  set $ready = $ready+1
  printf "HELP_READY %u chapter=%lu page=%d selection=%d links=%d target=%s\n", $ready, $v->navigation.chapter, $v->navigation.page, $v->renderer.style.selected, $v->renderer.style.total_links, $v->renderer.style.target
  if $ready == 1
    set $initial = $v->navigation.chapter
    if g_slicks_options_menu
      dump binary memory .run/help-menu-v1/help.chunky $v->renderer.ui.pixels $v->renderer.ui.pixels+64000
      dump binary memory .run/help-menu-v1/before.chunky $v->renderer.saved.pixels $v->renderer.saved.pixels+64000
    else
      dump binary memory .run/help-players-v1/help.chunky $v->renderer.ui.pixels $v->renderer.ui.pixels+64000
      dump binary memory .run/help-players-v1/before.chunky $v->renderer.saved.pixels $v->renderer.saved.pixels+64000
      dump binary memory .run/help-players-v1/help.palette $v->renderer.ui.palette $v->renderer.ui.palette+768
    end
  end
  if $ready == 3 && $v->navigation.selections[0] < 0
    printf "HELP_HISTORY_PUSH_FAILED\n"
    quit 1
  end
  if $ready == 4 && $v->navigation.chapter != $initial
    printf "HELP_HISTORY_RETURN_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_help_closed
commands
  silent
  set $m = g_slicks_options_menu
  if !$m
    set $m = g_slicks_player_menu
  end
  if !$m || $m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  if g_slicks_options_menu
    dump binary memory .run/help-menu-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  else
    dump binary memory .run/help-players-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 5 || $closed != 2 || g_slicks_diag_restore_status != 0x1f
    printf "HELP_GATE_FAILED ready=%u closed=%u\n", $ready, $closed
    quit 1
  end
  printf "HELP_NATIVE_MENU_NAVIGATION_REOPEN_RESTORE_OK\n"
  quit
end
continue
