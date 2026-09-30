# REGCHECK: keyless data; missing optional order form exercises cleanup too.
set $registration_owner=0
set $registration_closed=0
set $registration_io=0
set $registration_windows=0
break registration_screen
commands
  silent
  if $registration_owner
    quit 1
  end
  set $registration_owner=1
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $registration_owner
    printf "REGISTRATION_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if $registration_owner
    if $registration_io || !platform->active
      quit 1
    end
    set $registration_io=1
    set $registration_view=shown_view
    set $registration_pixels=platform->views[shown_view].bitmap->Planes[0]
    eval "dump binary memory .run/registration-io/%u-before.planar %p %p",$registration_windows,$registration_pixels,$registration_pixels+64000
    eval "dump binary memory .run/registration-io/%u-before.palette %p %p",$registration_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  end
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if $registration_owner
    if !$registration_io || !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$registration_view
      quit 1
    end
    eval "dump binary memory .run/registration-io/%u-after.planar %p %p",$registration_windows,$registration_pixels,$registration_pixels+64000
    eval "dump binary memory .run/registration-io/%u-after.palette %p %p",$registration_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
    set $registration_io=0
    set $registration_windows=$registration_windows+1
  end
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $registration_owner && g_slicks_diag_profile_platform->active && !g_slicks_diag_profile_platform->io_active
    printf "REGISTRATION_FILE_WITHOUT_OS_SERVICE\n"
    quit 1
  end
  continue
end
break slicks_diag_registration_screen_closed
commands
  silent
  if !$registration_owner || $registration_io || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $registration_owner=0
  set $registration_closed=$registration_closed+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $registration_owner || $registration_io || $registration_windows!=2 || $registration_closed<2 || g_slicks_diag_restore_status!=31
    printf "REGISTRATION_RETAINED_IO_FAILED windows=%u closed=%u\n",$registration_windows,$registration_closed
    quit 1
  end
  printf "REGISTRATION_RETAINED_IO_OK windows=%u closed=%u\n",$registration_windows,$registration_closed
  quit
end
continue
