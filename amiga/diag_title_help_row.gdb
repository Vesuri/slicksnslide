# HELPQ[ROW], saved normal or Arcade mode; no debugger input/state writes.
source diag_menu_rectangles.gdb
set $opened=0
set $closed=0
break *slicks_resource_archive_open_impl
commands
  silent
  if g_slicks_diag_ready && !$closed
    printf "TITLE_HELP_ROW_DISK_OPEN\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_diag_ready && !$closed
    printf "TITLE_HELP_ROW_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  set $opened=$opened+1
  set $viewer=g_slicks_title_help->help
  if $opened!=1 || !$viewer || !$viewer->renderer.active || $viewer->navigation.chapter!=9589 || $viewer->navigation.page || g_slicks_diag_title_help_seen!=g_slicks_diag_title_help_row || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $pixels=$viewer->renderer.ui.pixels
  dump binary memory .run/title-help-row/before.chunky $viewer->renderer.saved.pixels $viewer->renderer.saved.pixels+64000
  printf "TITLE_HELP_ROW_OPEN mode=%d row=%u chapter=9589\n",title_configuration->options[0],g_slicks_diag_title_help_seen
  continue
end
break slicks_diag_help_closed
commands
  silent
  set $closed=$closed+1
  if $opened!=1 || $closed!=1 || g_slicks_title_help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  dump binary memory .run/title-help-row/after.chunky $pixels $pixels+64000
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $opened!=1 || $closed!=1 || g_slicks_diag_restore_status!=31
    quit 1
  end
  printf "TITLE_HELP_ROW_OK row=%u publications=%u restore=31\n",g_slicks_diag_title_help_seen,$menu_publications
  quit
end
continue
quit 1
