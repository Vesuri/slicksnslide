# Capture the shown view's bitmap and palette at the loading I/O window and
# at racing update $capture_frame of the first race. Evidence of bitmap
# content only (not scanout). Render with tools/render_bitmap_capture.py.
set $capture_frame=40
set $loaded=0
break slicks_amiga_platform_begin_io
commands
  silent
  if !$loaded
    set $v='amiga_platform.cpp'::shown_view
    set $bm=g_slicks_diag_profile_platform->views[$v].bitmap
    printf "CAPTURE load view=%u bpr=%u p0=%lu p1=%lu p7=%lu\n",$v,$bm->BytesPerRow,$bm->Planes[0],$bm->Planes[1],$bm->Planes[7]
    eval "dump binary memory ../tmp/capture-load.planes %lu %lu",$bm->Planes[0],$bm->Planes[7]+200*$bm->BytesPerRow
    eval "dump binary memory ../tmp/capture-load.palette %lu %lu",&'amiga_platform.cpp'::view_palettes[$v],&'amiga_platform.cpp'::view_palettes[$v]+768
    set $loaded=1
  end
  continue
end
set $updates=0
break update_race_engines
commands
  silent
  set $updates=$updates+1
  if $updates==$capture_frame
    set $v='amiga_platform.cpp'::shown_view
    set $bm=g_slicks_diag_profile_platform->views[$v].bitmap
    printf "CAPTURE race view=%u update=%u bpr=%u p0=%lu p1=%lu p7=%lu\n",$v,$updates,$bm->BytesPerRow,$bm->Planes[0],$bm->Planes[1],$bm->Planes[7]
    eval "dump binary memory ../tmp/capture-race.planes %lu %lu",$bm->Planes[0],$bm->Planes[7]+200*$bm->BytesPerRow
    eval "dump binary memory ../tmp/capture-race.palette %lu %lu",&'amiga_platform.cpp'::view_palettes[$v],&'amiga_platform.cpp'::view_palettes[$v]+1
    printf "CAPTURE_OK\n"
    quit
  end
  continue
end
continue
