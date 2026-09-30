# NATURALW includes two shop capture keys. Run only on disposable game data.
set $capture_windows=0
set $capture_io=0
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_shop_menu
    printf "SHOP_CAPTURE_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if g_slicks_shop_menu
    if $capture_io
      quit 1
    end
    set $capture_io=1
    set $capture_view=shown_view
    set $capture_pixels=platform->views[shown_view].bitmap->Planes[0]
    eval "dump binary memory .run/shop-capture-io/%u-before.planar %p %p",$capture_windows,$capture_pixels,$capture_pixels+64000
    eval "dump binary memory .run/shop-capture-io/%u-before.palette %p %p",$capture_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
  end
  continue
end
break slicks_amiga_store_capture
commands
  silent
  if !$capture_io || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView || !buffer || capacity<65078
    printf "SHOP_CAPTURE_STORAGE_OR_SERVICE_FAILED\n"
    quit 1
  end
  eval "dump binary memory .run/shop-capture-io/%u.chunky %p %p",$capture_windows,pixels,pixels+64000
  eval "dump binary memory .run/shop-capture-io/%u.palette %p %p",$capture_windows,palette,palette+768
  continue
end
break slicks_amiga_platform_end_io
commands
  silent
  if g_slicks_shop_menu
    if !$capture_io || !platform->active || !platform->io_active || platform->gfx_base->ActiView || shown_view!=$capture_view
      quit 1
    end
    eval "dump binary memory .run/shop-capture-io/%u-after.planar %p %p",$capture_windows,$capture_pixels,$capture_pixels+64000
    eval "dump binary memory .run/shop-capture-io/%u-after.palette %p %p",$capture_windows,&view_palettes[shown_view][0],&view_palettes[shown_view][0]+768
    set $capture_io=0
    set $capture_windows=$capture_windows+1
  end
  continue
end
source diag_shop.gdb
