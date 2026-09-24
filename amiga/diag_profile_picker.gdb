set $menus = 0
set $pickers = 0
set $accepted = -1
set $initial = -1
break *slicks_diag_profile_picker_ready
commands
  silent
  set $pickers = $pickers + 1
  set $menu = g_slicks_player_menu
  set $platform = g_slicks_diag_profile_platform
  if !$menu || !$menu->picker || $menu->error || !$platform->active
    printf "PROFILE_PICKER_STATE_FAILED\n"
    quit 1
  end
  set $picker = &$menu->picker->renderer
  set $bitmap = $platform->views[0].bitmap
  if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
    quit 1
  end
  printf "PROFILE_PICKER_DRAW visit=%d selected=%d\n", $pickers, $picker->state.selected
  if $pickers == 1
    set $initial = $picker->state.selected
    dump binary memory .run/profile-picker-v1/open.chunky $picker->ui.pixels $picker->ui.pixels+64000
    dump binary memory .run/profile-picker-v1/open.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $pickers == 2
    set $accepted = $picker->state.selected
    if $accepted == $initial
      printf "PROFILE_PICKER_SELECTION_DID_NOT_CHANGE\n"
      quit 1
    end
  end
  if $pickers == 3 && $picker->state.selected != $accepted
    printf "PROFILE_PICKER_REOPEN_FAILED\n"
    quit 1
  end
  if $pickers == 4
    dump binary memory .run/profile-picker-v1/changed.chunky $picker->ui.pixels $picker->ui.pixels+64000
    dump binary memory .run/profile-picker-v1/changed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  set $menus = $menus + 1
  if $menus == 8
    if $pickers != 4 || g_slicks_player_menu->picker || g_slicks_player_menu->error || g_slicks_setup_session.players.selected[0] != $accepted
      printf "PROFILE_PICKER_COMMIT_CANCEL_FAILED\n"
      quit 1
    end
    set $menu = g_slicks_player_menu
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    dump binary memory .run/profile-picker-v1/closed.chunky $menu->renderer.ui.pixels $menu->renderer.ui.pixels+64000
    dump binary memory .run/profile-picker-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    printf "PROFILE_PICKER_OK ACCEPT_REOPEN_CANCEL_PRESERVED\n"
    quit
  end
  continue
end
continue
