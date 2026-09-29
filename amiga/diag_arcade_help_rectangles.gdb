# HELPA with a saved Arcade configuration: F2/F1 on each of its two rows.
source diag_menu_rectangles.gdb
set $arcade_help_row=0
set $arcade_help_opens=0
set $arcade_help_closes=0
set $arcade_help_guard=0
break redraw_title_configuration
commands
  silent
  set $arcade_help_row=selection
  continue
end
break open_title_help
commands
  silent
  if title_configuration->options[0]!=5 || $arcade_help_row!=$arcade_help_opens
    printf "ARCADE_HELP_ROW_FAILED\n"
    quit 1
  end
  set $arcade_help_guard=1
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $arcade_help_guard
    printf "ARCADE_HELP_ARCHIVE_REOPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $arcade_help_guard
    printf "ARCADE_HELP_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  if !g_slicks_title_help || !g_slicks_title_help->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $v=g_slicks_title_help->help
  if !$v->renderer.active || $v->navigation.done || $v->navigation.chapter!=9589 || $v->navigation.page
    printf "ARCADE_HELP_TOPIC_FAILED\n"
    quit 1
  end
  set $arcade_help_pixels=$v->renderer.ui.pixels
  eval "dump binary memory .run/arcade-help/%u.before %p %p",$arcade_help_opens,$v->renderer.saved.pixels,$v->renderer.saved.pixels+64000
  set $arcade_help_opens=$arcade_help_opens+1
  continue
end
break slicks_diag_help_closed
commands
  silent
  if g_slicks_title_help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  eval "dump binary memory .run/arcade-help/%u.after %p %p",$arcade_help_closes,$arcade_help_pixels,$arcade_help_pixels+64000
  set $arcade_help_closes=$arcade_help_closes+1
  if $arcade_help_closes==2
    set $arcade_help_guard=0
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $arcade_help_opens!=2 || $arcade_help_closes!=2 || $arcade_help_guard || g_slicks_diag_restore_status!=31
    printf "ARCADE_HELP_GATE_FAILED\n"
    quit 1
  end
  printf "ARCADE_HELP_BOTH_ROWS_RESTORE_OK\n"
  quit
end
continue
quit 1
