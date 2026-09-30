set $loads = 0
set $failed = 0
set $dismissed = 0
set $menus = 0
break slicks_amiga_platform_end
commands
  silent
  if !$_isvoid($race_failure_quit) && $dismissed == 1
    continue
  end
  printf "RECOVERABLE_RACE_FAILURE_RELEASED_DISPLAY\n"
  quit 1
end
break prepare_race
commands
  silent
  set $loads = $loads+1
  if !session || !new_game || $loads > 2
    quit 1
  end
  if $loads == 1
    set $configuration = configuration
    set $chunky = chunky
    dump binary memory .run/race-load-failure-v1/session-before.bin session session+1
    dump binary memory .run/race-load-failure-v1/config-before.bin configuration configuration+1
  end
  continue
end
break slicks_diag_race_load_failed
commands
  silent
  if !$_isvoid($shop_failure_expected) && g_slicks_diag_race_error != 9
    printf "EXPECTED_SHOP_PREPARATION_FAILURE\n"
    quit 1
  end
  if $loads != 1 || (g_slicks_diag_race_error != 2 && g_slicks_diag_race_error != 6 && g_slicks_diag_race_error != 9) || g_slicks_diag_race_load_fault || g_slicks_diag_ingame || !g_slicks_diag_profile_platform->active
    printf "RACE_LOAD_FAILURE_NOT_RECOVERABLE\n"
    quit 1
  end
  if g_slicks_diag_race_error==9
    if $_isvoid($shop_failure_expected) || !shop_live_failure_test || g_slicks_diag_shop_create_fault
      quit 1
    end
  end
  if shown_view != 0 || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $pixels=g_slicks_diag_profile_platform->views[0].bitmap->Planes[0]
  dump binary memory .run/menu-rectangles/1000.planar $pixels $pixels+64000
  dump binary memory .run/menu-rectangles/1000.chunky $chunky $chunky+64000
  dump binary memory .run/race-load-failure-v1/session-after.bin &g_slicks_setup_session &g_slicks_setup_session+1
  dump binary memory .run/race-load-failure-v1/config-after.bin $configuration $configuration+1
  set $failed = 1
  printf "RACE_LOAD_RECOVERED error=%u\n",g_slicks_diag_race_error
  continue
end
break slicks_diag_race_load_dismissed
commands
  silent
  set $dismissed = $dismissed+1
  if shown_view != 0 || !g_slicks_diag_profile_platform->active || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $pixels=g_slicks_diag_profile_platform->views[0].bitmap->Planes[0]
  dump binary memory .run/menu-rectangles/1001.planar $pixels $pixels+64000
  dump binary memory .run/menu-rectangles/1001.chunky $chunky $chunky+64000
  continue
end
break slicks_diag_player_menu_ready
commands
  silent
  if !$failed || !$dismissed || !g_slicks_player_menu
    quit 1
  end
  set $menus = $menus+1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $loads != 2 || !$failed || $dismissed != 1 || $menus != 1 || g_slicks_diag_race_error
      printf "RACE_LOAD_RETRY_FAILED\n"
      quit 1
    end
    printf "NATIVE_GO_LOAD_FAILURE_DISMISS_PLAYERS_RETRY_RACE_OK\n"
    quit
  end
  continue
end
continue
