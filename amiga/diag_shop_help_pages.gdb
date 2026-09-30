source diag_menu_rectangles.gdb
set $keys=0
set $opened=0
set $closed=0
break *slicks_amiga_help_open
commands
  silent
  if $opened
    quit 1
  end
  set $opened=1
  set $owner=*(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $cash=g_slicks_setup_session.cash[0]
  dump binary memory .run/shop-help-pages/before.chunky $owner->renderer.ui.pixels $owner->renderer.ui.pixels+64000
  continue
end
break *slicks_resource_archive_open
commands
  silent
  if $opened && !$closed
    printf "SHOP_HELP_PAGES_DISK_OPEN\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $opened && !$closed
    printf "SHOP_HELP_PAGES_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_diag_shop_help_key
commands
  silent
  set $v=g_slicks_shop_help_viewer
  printf "SHOP_HELP_PAGE_KEY key=%u ascii=%u scan=%u chapter=%lu page=%d country=%d selected=%d\n",$keys,g_slicks_shop_help_ascii,g_slicks_shop_help_scan,$v->navigation.chapter,$v->navigation.page,$v->renderer.style.country,$v->renderer.style.selected
  if title_configuration->field_05e1!=3
    quit 1
  end
  if $keys==0 || $keys==7
    if g_slicks_shop_help_scan!=59 || g_slicks_shop_help_ascii || $v->navigation.chapter!=$v->info.body || $v->navigation.page
      quit 1
    end
  end
  if $keys==2
    if g_slicks_shop_help_ascii!=13 || $v->navigation.page || $v->navigation.next_page<1
      printf "SHOP_HELP_MULTIPAGE_LINK_REQUIRED\n"
      quit 1
    end
    set $back_chapter=$v->navigation.chapter
    set $back_page=0
  end
  if $keys==3 || $keys==5
    if g_slicks_shop_help_scan!=81 || g_slicks_shop_help_ascii
      quit 1
    end
    if $keys==3
      set $forward_chapter=$v->navigation.chapter
      set $forward_page=$v->navigation.page
      if $forward_chapter!=$back_chapter || $forward_page!=$back_page+1
        printf "SHOP_HELP_PAGE_FORWARD_DID_NOT_MOVE\n"
        quit 1
      end
    else
      if $forward_chapter!=$v->navigation.chapter || $forward_page!=$v->navigation.page
        quit 1
      end
    end
  end
  if $keys==4 || $keys==6
    if g_slicks_shop_help_scan!=73 || g_slicks_shop_help_ascii
      quit 1
    end
    if $back_chapter!=$v->navigation.chapter || $back_page!=$v->navigation.page
      quit 1
    end
  end
  if $keys==7
    set $initial_selection=$v->renderer.style.selected
  end
  if $keys==8
    if g_slicks_shop_help_ascii!=9 || g_slicks_shop_help_scan || $v->renderer.style.selected==$initial_selection
      quit 1
    end
  end
  if $keys==9
    if g_slicks_shop_help_ascii!=75 || g_slicks_shop_help_scan || $v->renderer.style.selected!=$initial_selection
      quit 1
    end
  end
  if $keys==10
    if g_slicks_shop_help_ascii!=27 || g_slicks_shop_help_scan || !$v->navigation.done
      quit 1
    end
  end
  set $keys=$keys+1
  continue
end
break *slicks_amiga_help_close
commands
  silent
  set $close_return=*(unsigned long *)$sp
  tbreak *$close_return
  commands
    silent
    if $d0 || $keys!=11 || $owner->help || g_slicks_setup_session.cash[0]!=$cash
      quit 1
    end
    dump binary memory .run/shop-help-pages/after.chunky $owner->renderer.ui.pixels $owner->renderer.ui.pixels+64000
    set $closed=1
    continue
  end
  continue
end
break enter_prepared_race
commands
  silent
  if !$closed || g_slicks_shop_help_phase!=2 || g_slicks_diag_race_error
    quit 1
  end
  printf "SHOP_HELP_FI_CONFIG_PAGE_ALIASES_RESTORE_RACE_OK\n"
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "SHOP_HELP_PAGES_EARLY_EXIT\n"
  quit 1
end
continue
