# TRACKSV: preview Close failure, warning dismissal, retry/reopen and race.
source diag_menu_rectangles.gdb
set $expect_plain_close=1
break slicks_amiga_platform_end
commands
  silent
  if g_slicks_track_menu
    printf "TRACK_INFO_CLOSE_FAILURE_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open_impl
commands
  silent
  if g_slicks_track_menu && (g_slicks_track_menu->message || g_slicks_track_menu->track_info)
    printf "TRACK_INFO_CLOSE_FAILURE_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if g_slicks_track_menu
    set $info_view=shown_view
    set $info_pixels=platform->views[shown_view].bitmap->Planes[0]
    eval "dump binary memory .run/track-info-io/%u-before.planar %p %p",$info_windows,$info_pixels,$info_pixels+64000
    eval "dump binary memory .run/track-info-io/%u-before.palette %p %p",$info_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  end
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if g_slicks_track_menu
    if !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$info_view
      quit 1
    end
    eval "dump binary memory .run/track-info-io/%u-after.planar %p %p",$info_windows,$info_pixels,$info_pixels+64000
    eval "dump binary memory .run/track-info-io/%u-after.palette %p %p",$info_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
    set $info_windows=$info_windows+1
  end
  continue
end
set $info_windows=0
source diag_track_info_failure.gdb
