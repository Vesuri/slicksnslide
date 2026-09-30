# STARTGO with eight prefix/punctuation/mixed-case filenames.
# Capture the public C argument at the exact entry, not optimized locals.
set $catalogue_slot = 0
break *discover_tracks
commands
  silent
  set $catalogue_slot = *(unsigned long *)($sp+4)
  continue
end
break *prepare_race
commands
  silent
  if !$catalogue_slot || g_slicks_track_playlist.count!=8
    printf "TRACK_STEM_CATALOGUE_COUNT_FAILED\n"
    quit 1
  end
  set $catalogue_names = *(unsigned long *)$catalogue_slot
  dump binary memory .run/track-stem-order/names.bin $catalogue_names $catalogue_names+96
  printf "TRACK_STEM_CATALOGUE_CAPTURED count=8\n"
  continue
end
source diag_title_start.gdb
