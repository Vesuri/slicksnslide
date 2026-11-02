# An invalid SLICKS.REK only loses the title owner name; startup continues
# to the ordinary title and exit screen (REGCHECK).
break slicks_diag_registration_loaded
commands
  silent
  if g_slicks_registration_status != -1 || registration.name[0]
    quit 1
  end
  printf "REGISTRATION_INVALID_IGNORED\n"
  continue
end
break slicks_diag_exit_screen_ready
commands
  silent
  printf "REGISTRATION_INVALID_EXIT_SCREEN_OK\n"
  quit
end
continue
quit 1
