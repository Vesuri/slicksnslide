# TRACKSV: preview Close failure, warning dismissal, retry/reopen and race.
source diag_menu_rectangles.gdb
set $expect_plain_close=1
break slicks_amiga_platform_end
commands
  silent
  if g_slicks_track_menu && (g_slicks_track_menu->message || g_slicks_track_menu->track_info)
    printf "TRACK_INFO_CLOSE_FAILURE_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_resource_archive_open
commands
  silent
  if g_slicks_track_menu && (g_slicks_track_menu->message || g_slicks_track_menu->track_info)
    printf "TRACK_INFO_CLOSE_FAILURE_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
source diag_track_info_failure.gdb
