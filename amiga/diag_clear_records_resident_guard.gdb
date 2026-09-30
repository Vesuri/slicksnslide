# Include before either Clear Records workflow. Confirmation may service disk
# I/O but never restore Workbench; other message transitions remain RAM-only.
set $clear_io_windows=0
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_options_menu
    printf "CLEAR_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_begin_io
commands
  silent
  if g_slicks_options_menu
    if g_slicks_track_clear_phase!=1 || $ready!=2 || $clear_io_windows
      printf "CLEAR_RAM_MENU_TEARDOWN phase=%u ready=%u\n",g_slicks_track_clear_phase,$ready
      quit 1
    end
    set $clear_io_windows=$clear_io_windows+1
  end
  continue
end
source diag_menu_rectangles.gdb
