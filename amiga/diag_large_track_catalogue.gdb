# STARTGO against 300 distinct valid track filenames.
break *prepare_race
commands
  silent
  if g_slicks_track_playlist.count!=300 || g_slicks_track_playlist.capacity<300
    printf "LARGE_CATALOGUE_TRUNCATED count=%u capacity=%u\n",g_slicks_track_playlist.count,g_slicks_track_playlist.capacity
    quit 1
  end
  dump binary memory .run/large-catalogue/selection.bin g_slicks_track_playlist.tracks g_slicks_track_playlist.tracks+300
  printf "LARGE_CATALOGUE_SELECTION count=300\n"
  continue
end
source diag_title_start.gdb
