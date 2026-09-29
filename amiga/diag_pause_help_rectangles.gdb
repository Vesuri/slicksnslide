# LIVEMENUH: real race pause, nested Help navigation, reopen and resume.
source diag_menu_rectangles.gdb
set $steps=0
set $closed=0
set $resumed=0
break slicks_resource_archive_open
commands
  silent
  if g_slicks_diag_pause_menu
    printf "PAUSE_HELP_ARCHIVE_REOPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if g_slicks_diag_pause_menu
    printf "PAUSE_HELP_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  set $m=g_slicks_diag_pause_menu
  set $r=g_slicks_diag_paused_race
  if !$m || !$r || !g_slicks_diag_profile_platform->active || $steps>9
    quit 1
  end
  if !$steps
    set $frame=$r->frame_count
    set $clock=$r->game_clock_ticks
    set $status=status_clock.ticks
    dump binary memory .run/pause-help/before.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
    dump binary memory .run/pause-help/before.race $m->saved $m->saved+64000
  end
  if $r->frame_count!=$frame || $r->game_clock_ticks!=$clock || status_clock.ticks!=$status
    printf "PAUSE_HELP_ADVANCED_RACE\n"
    quit 1
  end
  if ($steps>=2 && $steps<=6) || $steps==8
    set $v=$m->help
    if !$v || !$v->renderer.active || $v->navigation.done
      quit 1
    end
    if $steps==2 || $steps==8
      if $v->navigation.chapter!=9589 || $v->navigation.page
        quit 1
      end
      eval "dump binary memory .run/pause-help/%u.before %p %p",$steps,$v->renderer.saved.pixels,$v->renderer.saved.pixels+64000
    end
    if ($steps==3 || $steps==6) && ($v->navigation.chapter!=$v->info.body || $v->navigation.page)
      quit 1
    end
    if $steps==5 && ($v->navigation.selections[0]<0 || $v->navigation.chapters[0]!=$v->info.body)
      quit 1
    end
    printf "PAUSE_HELP_STEP %u chapter=%lu page=%d\n",$steps,$v->navigation.chapter,$v->navigation.page
  else
    if $m->help || $m->help_warning
      quit 1
    end
    if $steps==7 || $steps==9
      eval "dump binary memory .run/pause-help/%u.after %p %p",$steps,$m->renderer.ui.pixels,$m->renderer.ui.pixels+64000
    end
  end
  set $steps=$steps+1
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  if $steps!=10 || $m->race_menu || $r->frame_count!=$frame || $r->game_clock_ticks!=$clock
    quit 1
  end
  dump binary memory .run/pause-help/after.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)
  dump binary memory .run/pause-help/after.race $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  set $closed=1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $closed && g_slicks_diag_race_frame==150
    if g_slicks_diag_race_error || g_slicks_diag_pause_menu || !g_slicks_diag_engine_started || $r->game_clock_ticks<=$clock
      quit 1
    end
    set $resumed=1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$closed || !$resumed || g_slicks_diag_restore_status!=31
    quit 1
  end
  printf "PAUSE_HELP_CONTENTS_HISTORY_REOPEN_RESUME_OK\n"
  quit
end
continue
quit 1
