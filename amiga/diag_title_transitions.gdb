# REGCHECKT uses native menu inputs, not debugger state injection.
break slicks_diag_system_restored
commands
  silent
  printf "TITLE_TRANSITIONS modes=%u roles=%u counts=%u checks=%lu errors=%lu restore=%u\n",g_slicks_title_seen_modes,g_slicks_title_seen_roles,g_slicks_title_seen_counts,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors,g_slicks_diag_restore_status
  if g_slicks_title_seen_modes != 31 || g_slicks_title_seen_roles != 7 || g_slicks_title_seen_counts != 3 || g_slicks_title_dirty_checks < 15 || g_slicks_title_dirty_errors || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "TITLE_TRANSITIONS_OK\n"
  quit
end
continue
quit 1
