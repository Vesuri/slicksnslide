set $visits = 0
set $waits = 0
break *slicks_diag_player_menu_ready
commands
  silent
  set $visits = $visits + 1
  set $menu = g_slicks_player_menu
  set $platform = g_slicks_diag_profile_platform
  printf "PLAYER_MENU_DRAW visit=%d row=%d head=%d tail=%d\n", $visits, g_slicks_diag_player_menu_row, $platform->key_head, $platform->key_tail
  if !$menu || $menu->error || $menu->renderer.error || !$platform->active
    printf "PLAYER_MENU_STATE_FAILED\n"
    quit 1
  end
  set $bitmap = $platform->views[0].bitmap
  if $bitmap->BytesPerRow != 320 || $bitmap->Depth != 8
    printf "PLAYER_MENU_BITMAP_FAILED\n"
    quit 1
  end
  if $visits == 1
    dump binary memory .run/player-menu-v1/initial.chunky $menu->renderer.ui.pixels $menu->renderer.ui.pixels+64000
    dump binary memory .run/player-menu-v1/initial.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $visits == 6
    if g_slicks_diag_player_menu_row != 0 || g_slicks_diag_player_menu_action
      printf "PLAYER_MENU_INPUT_FAILED\n"
      quit 1
    end
    dump binary memory .run/player-menu-v1/edited.chunky $menu->renderer.ui.pixels $menu->renderer.ui.pixels+64000
    dump binary memory .run/player-menu-v1/edited.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    printf "PLAYER_MENU_OK SIX_DRAWS REAL_DOWN_C_RIGHT_LEFT_UP_INPUT\n"
    quit
  end
  continue
end
break *slicks_amiga_platform_wait_vblank
commands
  silent
  if $visits > 0
    set $waits = $waits + 1
    if $waits > 10
      printf "PLAYER_MENU_INPUT_STALLED visits=%d row=%d head=%d tail=%d action=%d\n", $visits, g_slicks_diag_player_menu_row, g_slicks_diag_profile_platform->key_head, g_slicks_diag_profile_platform->key_tail, g_slicks_diag_player_menu_action
      quit 1
    end
  end
  continue
end
continue
