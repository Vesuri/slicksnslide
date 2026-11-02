# Observe REGCHECK startup/exit with native Escape/key input. A key file only
# supplies the title owner name: an invalid one is ignored, never fatal. No
# personal name or key bytes are printed, dumped or included in a screenshot.
set $loaded = 0
set $exit_screen = 0
break slicks_diag_registration_loaded
commands
  silent
  if (g_slicks_registration_status == 1) != (registration.name[0] != 0)
    quit 1
  end
  set $owner = g_slicks_registration_status
  set $loaded = 1
  printf "OWNER_NAME_LOADED status=%d\n",$owner
  continue
end
break slicks_diag_exit_screen_ready
commands
  silent
  printf "EXIT_SCREEN phase=%d\n",g_slicks_exit_screen
  if g_slicks_exit_screen != 1 || $exit_screen
    quit 1
  end
  set $exit_screen = 1
  # Let the real delay and timeout execute: no manipulation of clocks/state.
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$loaded || !$exit_screen || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "EXIT_SCREEN_NATIVE_OK owner=%d restoration=%x\n",$owner,g_slicks_diag_restore_status
  quit
end
continue
quit 1
