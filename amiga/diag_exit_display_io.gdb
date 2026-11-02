# REGCHECK: the exit screen keeps the display and borrows OS file service.
set $exit_owner=0
set $exit_closed=0
set $exit_io=0
set $exit_windows=0
break exit_screen
commands
  silent
  if $exit_owner
    quit 1
  end
  set $exit_owner=1
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $exit_owner
    printf "EXIT_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if $exit_owner
    if $exit_io || !platform->active
      quit 1
    end
    set $exit_io=1
    set $exit_view=shown_view
    set $exit_pixels=platform->views[shown_view].bitmap->Planes[0]
    eval "dump binary memory .run/exit-io/%u-before.planar %p %p",$exit_windows,$exit_pixels,$exit_pixels+64000
    eval "dump binary memory .run/exit-io/%u-before.palette %p %p",$exit_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  end
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if $exit_owner
    if !$exit_io || !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$exit_view
      quit 1
    end
    eval "dump binary memory .run/exit-io/%u-after.planar %p %p",$exit_windows,$exit_pixels,$exit_pixels+64000
    eval "dump binary memory .run/exit-io/%u-after.palette %p %p",$exit_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
    set $exit_io=0
    set $exit_windows=$exit_windows+1
  end
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if $exit_owner && g_slicks_diag_profile_platform->active && !g_slicks_diag_profile_platform->io_active
    printf "EXIT_FILE_WITHOUT_OS_SERVICE\n"
    quit 1
  end
  continue
end
break slicks_diag_exit_screen_closed
commands
  silent
  if !$exit_owner || $exit_io || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $exit_owner=0
  set $exit_closed=$exit_closed+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $exit_owner || $exit_io || $exit_windows!=1 || $exit_closed!=1 || g_slicks_diag_restore_status!=31
    printf "EXIT_RETAINED_IO_FAILED windows=%u closed=%u\n",$exit_windows,$exit_closed
    quit 1
  end
  printf "EXIT_RETAINED_IO_OK windows=%u closed=%u\n",$exit_windows,$exit_closed
  quit
end
continue
