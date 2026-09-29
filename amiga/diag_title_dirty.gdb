# REGCHECKD: native arrow keys, count edits, then normal exit. Read-only audit.
break slicks_diag_system_restored
commands
  silent
  printf "TITLE_DIRTY full=%lu partial=%lu checks=%lu errors=%lu last_pixels=%lu restore=%u\n",g_slicks_title_full_publications,g_slicks_title_partial_publications,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors,g_slicks_title_last_pixels,g_slicks_diag_restore_status
  if g_slicks_title_full_publications != 1 || g_slicks_title_partial_publications != 6 || g_slicks_title_dirty_checks != 7 || g_slicks_title_dirty_errors || g_slicks_title_last_pixels != 15520 || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "TITLE_DIRTY_NATIVE_OK\n"
  quit
end
continue
quit 1
