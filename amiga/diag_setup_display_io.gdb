# Source before a setup save retry/cancel fixture. Compare all before/after
# captures; expected windows are failed save, obstruction removal, saved retry.
set $setup_io_windows=0
set $setup_io_open=0
break slicks_amiga_platform_begin_io
commands
  silent
  if $setup_io_open || !platform->active
    quit 1
  end
  set $setup_io_open=1
  set $setup_io_view=shown_view
  set $setup_io_pixels=platform->views[shown_view].bitmap->Planes[0]
  eval "dump binary memory .run/setup-io/%u-before.planar %p %p",$setup_io_windows,$setup_io_pixels,$setup_io_pixels+64000
  eval "dump binary memory .run/setup-io/%u-before.palette %p %p",$setup_io_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if !$setup_io_open || !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$setup_io_view
    quit 1
  end
  eval "dump binary memory .run/setup-io/%u-after.planar %p %p",$setup_io_windows,$setup_io_pixels,$setup_io_pixels+64000
  eval "dump binary memory .run/setup-io/%u-after.palette %p %p",$setup_io_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  set $setup_io_open=0
  set $setup_io_windows=$setup_io_windows+1
  continue
end
