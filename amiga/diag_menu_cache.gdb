# HELP fixture: first READ THIS open, close, F1 reopen, close, quit.
# Guard the entire two-open interval, not just the renderer at its checkpoint.
set $guard = 0
set $ready = 0
set $closed = 0
break open_title_help
commands
  silent
  set $guard = 1
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $guard
    printf "CACHE_DISK_OPEN_DURING_NAVIGATION\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $guard
    printf "CACHE_DISPLAY_TEARDOWN_DURING_NAVIGATION\n"
    quit 1
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  if !g_slicks_title_help || !g_slicks_title_help->help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $ready = $ready+1
  printf "CACHE_HELP_READY %u resident_bytes=%lu\n", $ready, g_slicks_menu_cache_bytes
  continue
end
break slicks_diag_help_closed
commands
  silent
  if g_slicks_title_help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  if $closed == 2
    set $guard = 0
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 2 || $closed != 2 || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "CACHE_TITLE_HELP_NO_DISK_NO_TEARDOWN_RESTORE_OK\n"
  quit
end
continue
