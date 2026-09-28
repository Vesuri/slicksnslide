break slicks_diag_registration_loaded
commands
  silent
  if g_slicks_registration_status != -1
    quit 1
  end
  printf "REGISTRATION_INVALID_REJECTED_BEFORE_DISPLAY\n"
  quit
end
continue
quit 1
