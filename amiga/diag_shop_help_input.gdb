source diag_menu_rectangles.gdb
set $keys=0
break slicks_diag_shop_help_key
commands
  silent
  set $v=g_slicks_shop_help_viewer
  printf "SHOP_HELP_KEY %u ascii=%u scan=%u chapter=%lu history=%d target=%s\n",$keys,g_slicks_shop_help_ascii,g_slicks_shop_help_scan,$v->navigation.chapter,$v->navigation.selections[0],$v->renderer.style.target
  if $keys==0
    if g_slicks_shop_help_ascii || g_slicks_shop_help_scan!=59
      quit 1
    end
    set $body=$v->navigation.chapter
  else
    set $expected=27
    if $keys==1
      set $expected=13
    end
    if $keys==2
      set $expected=8
    end
    if $keys==3
      set $expected=32
    end
    if $keys==4
      set $expected=98
    end
    if g_slicks_shop_help_scan || g_slicks_shop_help_ascii!=$expected
      quit 1
    end
    if $keys==1 || $keys==3
      if $v->navigation.selections[0]<0 || $v->navigation.chapters[0]!=$body
        quit 1
      end
    end
    if $keys==2 || $keys==4
      if $v->navigation.chapter!=$body || $v->navigation.selections[0]!=-1
        quit 1
      end
    end
  end
  set $keys=$keys+1
  continue
end
break enter_prepared_race
commands
  silent
  if $keys!=6 || g_slicks_shop_help_phase!=2 || g_slicks_diag_race_error
    quit 1
  end
  printf "SHOP_HELP_ASCII_HISTORY_RACE_OK\n"
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "SHOP_HELP_EARLY_EXIT\n"
  quit 1
end
continue
