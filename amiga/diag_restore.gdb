break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status != 0x1f
    printf "SLICKS_RESTORE_FAILED STATUS=%02x\n", g_slicks_diag_restore_status
    quit 1
  end
  printf "SLICKS_RESTORE_OK STATUS=%02x\n", g_slicks_diag_restore_status
  quit
end
continue
