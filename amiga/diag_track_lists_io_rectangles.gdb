# TRACKSR with a directory named SLICKS.TRK in a private installation.
# Real AmigaDOS Open failure; no debugger fault/state injection.
source diag_menu_rectangles.gdb
set $track_io_loads=0
break slicks_amiga_load_track_lists
commands
  silent
  set $track_io_loads=$track_io_loads+1
  if $track_io_loads!=1
    printf "TRACK_IO_RELOADED_ON_NAVIGATION\n"
    quit 1
  end
  continue
end
break slicks_amiga_platform_end
commands
  silent
  if g_slicks_track_menu
    printf "TRACK_IO_WARNING_NAVIGATION_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open
commands
  silent
  if g_slicks_track_menu
    printf "TRACK_IO_WARNING_NAVIGATION_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
if $_isvoid($expected_load)
  set $expected_load=1
end
source diag_track_lists_failure.gdb
