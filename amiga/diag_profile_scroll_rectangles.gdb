source diag_menu_rectangles.gdb
set $pickers = 0
set $menus = 0
set $accepted = -1
break *slicks_diag_profile_picker_ready
commands
  silent
  set $pickers = $pickers+1
  set $m = g_slicks_player_menu
  set $s = &$m->picker->renderer.state
  if $m->error || !g_slicks_diag_profile_platform->active || $s->count != 100 || $s->visible >= 100
    printf "PROFILE_SCROLL_STATE_FAILED\n"
    quit 1
  end
  if $pickers == 1
    set $initial = $s->selected
    set $step = $s->visible-1
    set $expected = $initial
  end
  if $pickers == 2 || $pickers == 3 || $pickers == 4
    set $expected = $expected+$step
  end
  if $pickers == 5 || $pickers == 6 || $pickers == 12
    set $expected = $expected-$step
  end
  if $pickers == 7 || $pickers == 10
    set $expected = 99
  end
  if $pickers == 8 || $pickers == 9
    set $expected = 0
  end
  if $s->selected != $expected || ($pickers == 4 && $s->top == 0)
    printf "PROFILE_SCROLL_SELECTION_FAILED visit=%u got=%d expected=%d top=%d\n", $pickers, $s->selected, $expected, $s->top
    quit 1
  end
  if $pickers == 10
    set $accepted = $s->selected
  end
  printf "PROFILE_SCROLL visit=%u selected=%d top=%d visible=%d\n", $pickers, $s->selected, $s->top, $s->visible
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  set $menus = $menus+1
  if $menus == 3
    if $pickers != 12 || g_slicks_player_menu->picker || g_slicks_setup_session.players.selected[0] != $accepted
      printf "PROFILE_SCROLL_ACCEPT_CANCEL_FAILED\n"
      quit 1
    end
    printf "PROFILE_SCROLL_ACCEPT_REOPEN_CANCEL_OK publications=%u\n", $menu_publications
    quit
  end
  continue
end
continue
