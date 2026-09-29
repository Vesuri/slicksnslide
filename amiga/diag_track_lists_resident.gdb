set $resident_guard = 0
set $commits = 0
break slicks_amiga_load_track_lists
commands
  silent
  if $resident_guard || g_slicks_diag_profile_platform->active
    printf "TRACK_LIST_LOAD_OUTSIDE_DISK_BOUNDARY\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $resident_guard
    printf "TRACK_LIST_RAM_NAVIGATION_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
break track_lists_commit
commands
  silent
  if !$resident_guard || !g_slicks_diag_profile_platform->active
    printf "TRACK_LIST_COMMIT_BOUNDARY_FAILED\n"
    quit 1
  end
  set $resident_guard = 0
  set $commits = $commits+1
  continue
end
break slicks_amiga_store_track_lists
commands
  silent
  if g_slicks_diag_profile_platform->active
    printf "TRACK_LIST_SAVE_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if $resident_guard
    printf "TRACK_LIST_RAM_NAVIGATION_TEARDOWN\n"
    quit 1
  end
  continue
end
