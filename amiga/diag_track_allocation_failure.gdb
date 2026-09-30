# CATFAIL1/2/3 with 300 track files: initial names, growing names, selection.
set $discovery_returned = 0
break *discover_tracks
commands
  silent
  set $discovery_return = *(unsigned long *)$sp
  tbreak *$discovery_return
  commands
    silent
    set $discovery_returned = 1
    if track_files_in_current_directory || ($d0 & 65535)!=(g_slicks_diag_track_alloc_fail==3?300:0)
      printf "CATALOGUE_ALLOCATION_PARTIAL_OR_FALLBACK\n"
      quit 1
    end
    continue
  end
  continue
end
break *slicks_amiga_platform_begin
commands
  silent
  printf "CATALOGUE_ALLOCATION_TOOK_OVER_DISPLAY\n"
  quit 1
end
break *prepare_race
commands
  silent
  printf "CATALOGUE_ALLOCATION_REACHED_RACE\n"
  quit 1
end
break slicks_diag_frame_ready
commands
  silent
  printf "CATALOGUE_ALLOCATION_REACHED_TITLE\n"
  quit 1
end
break slicks_diag_track_storage_released
commands
  silent
  if !$discovery_returned || g_slicks_diag_track_alloc_fail<1 || g_slicks_diag_track_alloc_fail>3 || g_slicks_diag_track_alloc_attempts!=g_slicks_diag_track_alloc_fail || g_slicks_diag_track_alloc_live || g_slicks_track_playlist.tracks!=initial_track_selection || g_slicks_track_playlist.capacity!=256 || !(g_slicks_diag_restore_status & 1)
    printf "CATALOGUE_ALLOCATION_CLEANUP_FAILED fail=%u attempts=%u live=%u\n",g_slicks_diag_track_alloc_fail,g_slicks_diag_track_alloc_attempts,g_slicks_diag_track_alloc_live
    quit 1
  end
  printf "CATALOGUE_ALLOCATION_FAILURE_CLEANUP_OK case=%u\n",g_slicks_diag_track_alloc_fail
  quit
end
continue
