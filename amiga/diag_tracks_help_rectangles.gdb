# OPTIONSO: ordinary Tracks -> F1 -> link/history -> close/reopen -> exit.
source diag_menu_rectangles.gdb
set $track_help_ready=0
set $track_help_closed=0
set $track_help_guard=0
break open_help
commands
  silent
  if !g_slicks_track_menu || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $track_help_guard=1
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $track_help_guard
    printf "TRACK_HELP_ARCHIVE_REOPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $track_help_guard
    printf "TRACK_HELP_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  set $m=g_slicks_track_menu
  if !$m || !$m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $v=$m->help
  if !$v->renderer.active || $v->navigation.done
    quit 1
  end
  set $track_help_ready=$track_help_ready+1
  printf "TRACK_HELP_READY %u chapter=%lu page=%d links=%d\n",$track_help_ready,$v->navigation.chapter,$v->navigation.page,$v->renderer.style.total_links
  if $track_help_ready==1 || $track_help_ready==5
    set $track_help_initial=$v->navigation.chapter
    eval "dump binary memory .run/track-help/%u.before %p %p",$track_help_closed,$v->renderer.saved.pixels,$v->renderer.saved.pixels+64000
  end
  if $track_help_ready==3 && $v->navigation.selections[0]<0
    quit 1
  end
  if $track_help_ready==4 && $v->navigation.chapter!=$track_help_initial
    quit 1
  end
  continue
end
break slicks_diag_help_closed
commands
  silent
  set $m=g_slicks_track_menu
  if !$m || $m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  eval "dump binary memory .run/track-help/%u.after %p %p",$track_help_closed,$m->renderer.ui.pixels,$m->renderer.ui.pixels+64000
  set $track_help_closed=$track_help_closed+1
  if $track_help_closed==2
    set $track_help_guard=0
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $track_help_ready!=5 || $track_help_closed!=2 || $track_help_guard || g_slicks_diag_restore_status!=31
    printf "TRACK_HELP_GATE_FAILED ready=%u closed=%u restore=%u\n",$track_help_ready,$track_help_closed,g_slicks_diag_restore_status
    quit 1
  end
  printf "TRACK_HELP_NAVIGATION_CACHE_RESTORE_OK\n"
  quit
end
continue
quit 1
