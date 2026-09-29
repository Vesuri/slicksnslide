set $ready = 0
set $closed = 0
break slicks_diag_help_ready
commands
  silent
  set $v = g_slicks_player_menu->help
  if !$v || !$v->renderer.active || $v->navigation.done
    quit 1
  end
  set $ready = $ready+1
  printf "HELP_PAGE_READY %u chapter=%lu page=%d\n", $ready, $v->navigation.chapter, $v->navigation.page
  if $ready == 1
    set $initial_chapter = $v->navigation.chapter
    set $initial_page = $v->navigation.page
    if $initial_page < 1
      quit 1
    end
    dump binary memory .run/help-pages-v1/before.chunky $v->renderer.saved.pixels $v->renderer.saved.pixels+64000
  end
  if $ready == 2 && ($v->navigation.chapter != $initial_chapter || $v->navigation.page != $initial_page-1)
    quit 1
  end
  if $ready == 3 && ($v->navigation.chapter != $initial_chapter || $v->navigation.page != $initial_page)
    quit 1
  end
  if $ready == 4 && ($v->navigation.chapter != $v->info.body || $v->navigation.page != 0)
    quit 1
  end
  continue
end
break slicks_diag_help_closed
commands
  silent
  if g_slicks_player_menu->help
    quit 1
  end
  set $closed = $closed+1
  set $pixels = g_slicks_player_menu->renderer.ui.pixels
  dump binary memory .run/help-pages-v1/after.chunky $pixels $pixels+64000
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 4 || $closed != 1 || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "HELP_NATIVE_PAGE_KEYS_CONTENTS_RESTORE_OK\n"
  quit
end
continue
