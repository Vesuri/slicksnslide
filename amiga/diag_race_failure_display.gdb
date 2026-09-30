# SETUPF/SETUPG: disk failure followed by successful preparation. Neither
# window may change the displayed loading image or return to Workbench.
set $load_io_count=0
set $load_io_open=0
break slicks_amiga_platform_begin_io
commands
  silent
  if $load_io_open || !platform->active
    quit 1
  end
  set $load_io_open=1
  set $load_io_view=shown_view
  set $load_io_pixels=platform->views[shown_view].bitmap->Planes[0]
  eval "dump binary memory .run/load-io/%u-before.planar %p %p",$load_io_count,$load_io_pixels,$load_io_pixels+64000
  eval "dump binary memory .run/load-io/%u-before.palette %p %p",$load_io_count,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if !$load_io_open || !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$load_io_view
    quit 1
  end
  eval "dump binary memory .run/load-io/%u-after.planar %p %p",$load_io_count,$load_io_pixels,$load_io_pixels+64000
  eval "dump binary memory .run/load-io/%u-after.palette %p %p",$load_io_count,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  set $load_io_open=0
  set $load_io_count=$load_io_count+1
  continue
end
source diag_race_load_failure.gdb
