set $closed = 0
set $captures = 0
break slicks_amiga_player_menu_destroy
commands
  silent
  printf "CONTROLLERS_UNEXPECTED_MENU_DESTRUCTION\n"
  backtrace 5
  quit 1
end
break slicks_diag_controllers_ready
commands
  silent
  set $m = g_slicks_options_menu
  if !$m || !$m->controllers_dialog || $m->error || $m->renderer.error
    printf "CONTROLLERS_DRAW_FAILED\n"
    quit 1
  end
  if $m->controllers_dialog->state.capturing
    set $captures = $captures+1
  end
  continue
end
break slicks_diag_controllers_closed
commands
  silent
  set $closed = $closed+1
  set $c = g_slicks_options_configuration
  if g_slicks_options_menu->controllers_dialog || !$c || !g_slicks_options_state.dirty || $c->player_input[0] != 1
    printf "CONTROLLERS_CLOSE_FAILED\n"
    quit 1
  end
  if $closed == 1 && $c->keys[0] != 0x11
    printf "CONTROLLERS_CAPTURE_W_OR_REJECT_F3_FAILED\n"
    quit 1
  end
  if $closed == 2
    set $i = 0
    while $i < 20
      if $c->keys[$i] != slicks_original_configuration.keys[$i]
        printf "CONTROLLERS_DEFAULTS_FAILED\n"
        quit 1
      end
      set $i = $i+1
    end
  end
  if $closed == 3
    if $c->keys[0] != 0x15 || $captures != 3 || !g_slicks_diag_profile_platform->active
      printf "CONTROLLERS_REEDIT_FAILED\n"
      quit 1
    end
    printf "CONTROLLERS_ENTRY_CAPTURE_DEFAULTS_REOPEN_REEDIT_OK\n"
    quit
  end
  continue
end
continue
