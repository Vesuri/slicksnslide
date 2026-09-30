set $ready = 0
set $closed = 0
set $help_page_guard = 0
break open_help
commands
  silent
  if !g_slicks_player_menu || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $help_page_guard = 1
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $help_page_guard
    printf "HELP_PAGE_UNEXPECTED_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $help_page_guard
    printf "HELP_PAGE_UNEXPECTED_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  set $v = g_slicks_player_menu->help
  if !$v || !$v->renderer.active || $v->navigation.done || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $ready = $ready+1
  # Keep the optional arithmetic inside a separate command block: this GDB
  # rejects the void operand even in the RHS of a false logical conjunction.
  if !$_isvoid($expected_help_language)
    if menu_language_name[4] != 48+$expected_help_language
      printf "HELP_PAGE_WRONG_LANGUAGE\n"
      quit 1
    end
  end
  printf "HELP_PAGE_LANGUAGE %s\n",menu_language_name
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
  if g_slicks_player_menu->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  set $help_page_guard = 0
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
