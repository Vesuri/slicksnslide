set $record_owner = 0
set $record_warning_owned = 0
set $record_closing = 0
set $record_returns = 0
set $record_io_count = 0
set $record_io_open = 0
init-if-undefined $capture_record_io = 0
# Keep the full standings fixture within FS-UAE's breakpoint capacity.
# diag_records_io.gdb separately captures every retained display across I/O.
if $capture_record_io
break slicks_amiga_platform_begin_io
commands
  silent
  if $record_owner
    if $record_io_open || !g_slicks_diag_profile_platform->active
      printf "RECORD_IO_NESTING_FAILED\n"
      quit 1
    end
    set $record_io_open = 1
    set $record_io_view = shown_view
    set $record_io_pixels = g_slicks_diag_profile_platform->views[shown_view].bitmap->Planes[0]
    eval "dump binary memory .run/post-race-records-v1/io-%u-before.planar %p %p", $record_io_count, $record_io_pixels, $record_io_pixels+64000
    eval "dump binary memory .run/post-race-records-v1/io-%u-before.palette %p %p", $record_io_count, &view_palettes[shown_view][0], &view_palettes[shown_view][0]+768
  end
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if $record_owner
    if !$record_io_open || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView || shown_view!=$record_io_view
      printf "RECORD_IO_DISPLAY_OWNERSHIP_FAILED\n"
      quit 1
    end
    eval "dump binary memory .run/post-race-records-v1/io-%u-after.planar %p %p", $record_io_count, $record_io_pixels, $record_io_pixels+64000
    eval "dump binary memory .run/post-race-records-v1/io-%u-after.palette %p %p", $record_io_count, &view_palettes[shown_view][0], &view_palettes[shown_view][0]+768
    set $record_io_count = $record_io_count+1
    set $record_io_open = 0
  end
  continue
end
end
break *run_record_results
commands
  silent
  set $record_owner = 1
  set $record_pixels = *(unsigned char **)($sp+12)
  set $record_bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  eval "dump binary memory .run/post-race-records-v1/record-%u-entry.chunky %p %p", $record_returns, $record_pixels, $record_pixels+64000
  eval "dump binary memory .run/post-race-records-v1/record-%u-entry.planar %p %p", $record_returns, $record_bitmap->Planes[0], $record_bitmap->Planes[0]+64000
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $record_owner
    printf "RECORD_RESULT_ARCHIVE_READ\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $record_owner || $record_warning_owned || $record_closing
    printf "RECORD_RAM_CLOSE_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_emergency_warning_close
commands
  silent
  if $record_warning_owned
    if !g_slicks_diag_profile_platform->active
      quit 1
    end
    set $record_warning_owned = 0
  end
  continue
end
break slicks_amiga_store_track_records
commands
  silent
  if !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
    printf "RECORD_WRITE_WITHOUT_DISPLAY_PRESERVING_IO\n"
    quit 1
  end
  continue
end
break slicks_diag_record_results_closed
commands
  silent
  if !$record_owner || $record_warning_owned || !g_slicks_diag_profile_platform->active || g_slicks_diag_profile_platform->io_active || $record_io_open || ($capture_record_io && !$record_io_count)
    printf "RECORD_RETURN_FAILED\n"
    quit 1
  end
  set $record_owner = 0
  set $record_closing = 0
  eval "dump binary memory .run/post-race-records-v1/record-%u-close.chunky %p %p", $record_returns, $record_pixels, $record_pixels+64000
  eval "dump binary memory .run/post-race-records-v1/record-%u-close.planar %p %p", $record_returns, $record_bitmap->Planes[0], $record_bitmap->Planes[0]+64000
  set $record_returns = $record_returns+1
  continue
end
