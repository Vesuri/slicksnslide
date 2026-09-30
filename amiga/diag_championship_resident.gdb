init-if-undefined $expect_cup = 1
init-if-undefined $capture_cup_pixels = 0
set $cup_owned = 0
set $cup_io = 0
set $cup_loads = 0
set $cup_phases = 0
set $cup_closed = 0
set $title_pending = 0
set $title_seen = 0
break run_championship_results
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $cup_owned = 1
  continue
end
break slicks_diag_standings_io
commands
  silent
  if !$cup_owned || !$expect_cup
    quit 1
  end
  set $cup_io = 1
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $cup_owned || $title_pending
    printf "CUP_RAM_TRANSITION_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open
commands
  silent
  if $cup_owned
    if !$cup_io || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
      quit 1
    end
    set $cup_loads = $cup_loads+1
  end
  continue
end
break slicks_resource_archive_close
commands
  silent
  if $cup_owned && archive->file && (!g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active)
    printf "CUP_FILE_CLOSE_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_diag_standings_ready
commands
  silent
  if g_slicks_diag_standings_phase != $cup_phases+1 || g_slicks_diag_profile_platform->io_active
    quit 1
  end
  set $cup_phases = $cup_phases+1
  set $cup_io = 0
  if $capture_cup_pixels && g_slicks_diag_standings_phase == 2
    set $cup_pixels = g_slicks_diag_standings_menu->renderer.ui.pixels
    set $cup_view = 0
    while $cup_view < 2
      set $cup_planes = g_slicks_diag_profile_platform->views[$cup_view].bitmap->Planes[0]
      eval "dump binary memory .run/menu-rectangles/%u.chunky $cup_pixels $cup_pixels+64000", 10000+$cup_view
      eval "dump binary memory .run/menu-rectangles/%u.planar $cup_planes $cup_planes+64000", 10000+$cup_view
      set $cup_view = $cup_view+1
    end
  end
  continue
end
break slicks_diag_standings_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active || g_slicks_diag_standings_menu || $cup_phases != 3*$expect_cup || $cup_loads != $expect_cup
    printf "CUP_CLOSE_FAILED phases=%u loads=%u\n",$cup_phases,$cup_loads
    quit 1
  end
  set $v = 0
  while $v < 2
    set $bank = 0
    while $bank < 2
      set $colour = 0
      while $colour < 256
        if (g_slicks_diag_profile_platform->views[$v].copper[palette_words[$v][$bank][$colour]] & 65535) != 0
          printf "CUP_RETURN_PALETTE_NOT_BLACK\n"
          quit 1
        end
        set $colour = $colour+1
      end
      set $bank = $bank+1
    end
    set $v = $v+1
  end
  set $cup_owned = 0
  set $cup_closed = $cup_closed+1
  set $title_pending = 1
  continue
end
break redraw_title_configuration
commands
  silent
  if $title_pending
    if !g_slicks_diag_profile_platform->active
      quit 1
    end
    set $title_pending = 0
    set $title_seen = $title_seen+1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $cup_closed != 1 || $title_seen != 1 || $title_pending || $cup_owned || g_slicks_diag_restore_status != 31
    printf "CUP_RESIDENT_FLOW_FAILED\n"
    quit 1
  end
  printf "NATIVE_CUP_RESIDENT_RETURN_OK cup=%u disk_loads=%u\n",$expect_cup,$cup_loads
  quit
end
continue
