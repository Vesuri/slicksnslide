# Observe REGCHECK startup/exit with native Escape/key input. No personal name
# or key bytes are printed, dumped or included in any screenshot.
set $loaded = 0
set $exit_screen = 0
break slicks_diag_registration_loaded
commands
  silent
  if g_slicks_registration_status < 0
    printf "REGISTRATION_NATIVE_INVALID\n"
    quit 1
  end
  set $registered = g_slicks_registration_status
  set $loaded = 1
  printf "REGISTRATION_LOADED status=%d\n",$registered
  continue
end
break slicks_diag_registration_screen_ready
commands
  silent
  printf "REGISTRATION_SCREEN phase=%d\n",g_slicks_registration_screen
  if g_slicks_registration_screen == 1
    if $registered
      quit 1
    end
  else
    if g_slicks_registration_screen != 2 + $registered
      quit 1
    end
    set $exit_screen = 1
  end
  # Let the real delay and timeout execute: no manipulation of clocks/state.
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$loaded || !$exit_screen || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "REGISTRATION_NATIVE_EXIT_OK registered=%d restoration=%x\n",$registered,g_slicks_diag_restore_status
  quit
end
continue
quit 1
