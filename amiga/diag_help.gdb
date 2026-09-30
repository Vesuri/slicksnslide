set $ready = 0
set $closed = 0
set $help_owner_guard = 0
set $help_owner_opened = 0
break open_help
commands
  silent
  set $help_owner = g_slicks_options_menu
  if !$help_owner
    set $help_owner = g_slicks_player_menu
  end
  if !$help_owner
    set $help_owner = g_slicks_track_menu
  end
  if !$help_owner || $help_owner_guard || $help_owner->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $help_owner_guard = 1
  set $help_owner_opened = $help_owner_opened+1
  set $help_font0 = $help_owner->renderer.fonts[0]
  set $help_font1 = $help_owner->renderer.fonts[1]
  set $help_font2 = $help_owner->renderer.fonts[2]
  set $help_dirty_callback = $help_owner->renderer.ui.dirty
  set $help_dirty_context = $help_owner->renderer.ui.dirty_context
  set $help_font_bytes = (unsigned char *)$help_owner->fonts
  eval "dump binary memory .run/help-owner/%u.font-before %p %p", $closed, $help_font_bytes, $help_font_bytes+sizeof($help_owner->fonts)
  eval "dump binary memory .run/help-owner/%u.before %p %p", $closed, $help_owner->renderer.ui.pixels, $help_owner->renderer.ui.pixels+64000
  printf "HELP_OWNER_OPEN language=%s options=%u players=%u tracks=%u\n",menu_language_name, $help_owner==g_slicks_options_menu, $help_owner==g_slicks_player_menu, $help_owner==g_slicks_track_menu
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $help_owner_guard
    printf "HELP_OWNER_ARCHIVE_REOPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $help_owner_guard
    printf "HELP_OWNER_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  set $m = g_slicks_options_menu
  if !$m
    set $m = g_slicks_player_menu
  end
  if !$m
    set $m = g_slicks_track_menu
  end
  if !$m || $m != $help_owner || !$help_owner_guard
    quit 1
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
      if g_slicks_player_menu
        dump binary memory .run/help-players-v1/help.chunky $v->renderer.ui.pixels $v->renderer.ui.pixels+64000
        dump binary memory .run/help-players-v1/before.chunky $v->renderer.saved.pixels $v->renderer.saved.pixels+64000
        dump binary memory .run/help-players-v1/help.palette $v->renderer.ui.palette $v->renderer.ui.palette+768
      end
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
  if !$m
    set $m = g_slicks_track_menu
  end
  if !$m || $m->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $m != $help_owner || !$help_owner_guard || $m->renderer.fonts[0] != $help_font0 || $m->renderer.fonts[1] != $help_font1 || $m->renderer.fonts[2] != $help_font2 || $m->renderer.ui.dirty != $help_dirty_callback || $m->renderer.ui.dirty_context != $help_dirty_context
    printf "HELP_OWNER_ALIAS_RESTORE_FAILED\n"
    quit 1
  end
  eval "dump binary memory .run/help-owner/%u.font-after %p %p", $closed, $help_font_bytes, $help_font_bytes+sizeof($m->fonts)
  eval "dump binary memory .run/help-owner/%u.after %p %p", $closed, $m->renderer.ui.pixels, $m->renderer.ui.pixels+64000
  set $help_owner_guard = 0
  set $closed = $closed+1
  if g_slicks_options_menu
    dump binary memory .run/help-menu-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
  else
    if g_slicks_player_menu
      dump binary memory .run/help-players-v1/after.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 5 || $closed != 2 || $help_owner_opened != 2 || $help_owner_guard || g_slicks_diag_restore_status != 0x1f
    printf "HELP_GATE_FAILED ready=%u closed=%u\n", $ready, $closed
    quit 1
  end
  printf "HELP_NATIVE_MENU_NAVIGATION_REOPEN_RESTORE_OK\n"
  quit
end
continue
