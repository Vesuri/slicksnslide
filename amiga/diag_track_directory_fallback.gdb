# STARTGO with ROOT.SS in the data directory and no usable TRACKS entries.
if $_isvoid($expected_track_directory)
  set $expected_track_directory = 1
end
break *prepare_race
commands
  silent
  if track_files_in_current_directory!=$expected_track_directory || g_slicks_track_playlist.count!=1
    printf "TRACK_DIRECTORY_FALLBACK_FAILED directory=%u count=%u\n",track_files_in_current_directory,g_slicks_track_playlist.count
    quit 1
  end
  printf "TRACK_DIRECTORY_FALLBACK current_directory=%u count=1\n",track_files_in_current_directory
  continue
end
source diag_title_start.gdb
