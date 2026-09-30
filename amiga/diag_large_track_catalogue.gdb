# STARTGO against 300 distinct valid track filenames.
if $_isvoid($expected_catalogue_tracks)
  set $expected_catalogue_tracks = 300
end
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
  if g_slicks_track_playlist.count!=$expected_catalogue_tracks || g_slicks_track_playlist.capacity<$expected_catalogue_tracks
    printf "LARGE_CATALOGUE_TRUNCATED count=%u capacity=%u\n",g_slicks_track_playlist.count,g_slicks_track_playlist.capacity
    quit 1
  end
  dump binary memory .run/large-catalogue/selection.bin g_slicks_track_playlist.tracks g_slicks_track_playlist.tracks+$expected_catalogue_tracks
  if !$catalogue_slot
    quit 1
  end
  set $catalogue_names = *(unsigned long *)$catalogue_slot
  dump binary memory .run/large-catalogue/names.bin $catalogue_names $catalogue_names+12*$expected_catalogue_tracks
  printf "LARGE_CATALOGUE_SELECTION count=%u\n",g_slicks_track_playlist.count
  set $prepare_return = *(unsigned long *)$sp
  tbreak *$prepare_return
  commands
    silent
    printf "LARGE_CATALOGUE_PREPARE_RETURN result=%d error=%u allocation_failures=%u stage=%u\n",$d0,g_slicks_diag_race_error,g_slicks_diag_race_allocation_failures,g_slicks_diag_race_stage
    if !$_isvoid($capture_race_memory)
      printf "CATALOGUE_SCRATCH_MEMORY before_total=%lu before_largest=%lu after_total=%lu after_largest=%lu\n",g_slicks_diag_race_memory[0],g_slicks_diag_race_memory[1],g_slicks_diag_race_memory[2],g_slicks_diag_race_memory[3]
      if !g_slicks_diag_race_memory[0] || !g_slicks_diag_race_memory[2]
        quit 1
      end
    end
    if $d0 || g_slicks_diag_race_error || g_slicks_diag_race_allocation_failures
      quit 1
    end
    continue
  end
  continue
end
source diag_title_start.gdb
