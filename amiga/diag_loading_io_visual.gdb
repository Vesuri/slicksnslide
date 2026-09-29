# Manual visible-scanout checkpoint after 50 OS-serviced refreshes.
break *slicks_amiga_platform_end_io
set $io_visual_break=$bpnum
commands
  silent
  set $io_platform=*(struct SlicksAmigaPlatform **)($sp+4)
  if !$io_platform->io_active || !$io_platform->active || $io_platform->gfx_base->ActiView
    printf "LOADING_IO_VISUAL_INVALID_STATE\n"
    quit 1
  end
  printf "LOADING_IO_VISUAL_READY bytes=%lu hash=%lu\n",g_slicks_loading_io_bytes,g_slicks_loading_io_hash
  disable $io_visual_break
  shell sleep 30
  continue
end
source diag_demo_lifecycle.gdb
