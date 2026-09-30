# CHAMPEDIT, Classic CFG, one existing E2E.SSS. Behaviour is checked separately
# by diag_championship_edit; this gate captures every dialog disk boundary.
set $io_open = 0
set $windows = 0
set $returns = 0
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_diag_saved_menu
    printf "SAVED_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if g_slicks_diag_saved_menu
    if $io_open || !g_slicks_diag_profile_platform->active
      quit 1
    end
    set $io_open = 1
    set $view = shown_view
    set $pixels = g_slicks_diag_profile_platform->views[$view].bitmap->Planes[0]
    eval "dump binary memory .run/saved-io/%u-before.planar %p %p",$windows,$pixels,$pixels+64000
    eval "dump binary memory .run/saved-io/%u-before.palette %p %p",$windows,&view_palettes[$view][0],&view_palettes[$view][0]+768
  end
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if g_slicks_diag_saved_menu
    if !$io_open || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView || shown_view!=$view
      printf "SAVED_IO_DISPLAY_LOST\n"
      quit 1
    end
    eval "dump binary memory .run/saved-io/%u-after.planar %p %p",$windows,$pixels,$pixels+64000
    eval "dump binary memory .run/saved-io/%u-after.palette %p %p",$windows,&view_palettes[$view][0],&view_palettes[$view][0]+768
    set $windows = $windows+1
    set $io_open = 0
  end
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if $io_open || !g_slicks_diag_profile_platform->active || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $returns = $returns+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$returns || $windows<6 || $io_open || g_slicks_diag_restore_status!=31
    printf "SAVED_DISPLAY_IO_FAILED windows=%u returns=%u\n",$windows,$returns
    quit 1
  end
  printf "SAVED_DISPLAY_IO_OK windows=%u returns=%u\n",$windows,$returns
  quit
end
continue
