set $loads = 0
set $returns = 0
set $race = 0
set $return_frame = 0
break prepare_race
commands
  silent
  set $loads = $loads+1
  if $loads > 2 || !session || new_game != ($loads == 1) || configuration->options[0] != 5 || configuration->options[13] != 5 || configuration->options[14] != 2
    printf "SEQUENCE_PREPARE_FAILED load=%u\n", $loads
    quit 1
  end
  if g_slicks_track_playlist.count != 2 || g_slicks_track_playlist.tracks[0] == g_slicks_track_playlist.tracks[1]
    printf "SEQUENCE_PLAYLIST_FAILED\n"
    quit 1
  end
  if track_path[7] != 'B' || track_path[8] != 'A' || track_path[9] != 'S' || track_path[10] != 'I' || track_path[11] != 'C' || ($loads == 1 && track_path[12] != '.') || ($loads == 2 && track_path[12] != 'T')
    printf "SEQUENCE_TRACK_ORDER_FAILED\n"
    quit 1
  end
  printf "SEQUENCE_LOAD %u new_game=%u path=%s\n", $loads,new_game,track_path
  if $loads == 2
    set $d = 0
    while $d < 4
      set $profile = session->players.selected[$d]
      if $profile > 0 && g_slicks_profiles.setup[$profile].vehicle >= 11
        set $seed = ($seed*0x015a4e35+1)&0xffffffff
      end
      set $d = $d+1
    end
    if session->random_state != $seed
      quit 1
    end
    dump binary memory .run/track-sequence-v1/session-before.bin session session+1
  end
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $loads == 2
    if g_slicks_setup_session.random_state != $seed
      quit 1
    end
    dump binary memory .run/track-sequence-v1/session-after.bin &g_slicks_setup_session &g_slicks_setup_session+1
  end
  continue
end
break slicks_diag_frame_ready if g_slicks_diag_race_complete || g_slicks_diag_race_frame > 6000
commands
  silent
  if g_slicks_diag_ingame && $race
    if g_slicks_diag_race_error || $race->frame_count > 6000
      printf "SEQUENCE_RACE_FAILED load=%u frame=%u error=%u ticks=%u deadline=%u finished=%u lap=%u\n",$loads,$race->frame_count,g_slicks_diag_race_error,$race->game_clock_ticks,$race->finish_deadline,$race->finished_count,$race->cars[0].lap
      quit 1
    end
    if $race->race_complete && $returns < $loads
      set $seed = $race->random_state
      set $returns = $returns+1
      set $return_frame = $race->frame_count
      printf "SEQUENCE_RACE_COMPLETE load=%u frame=%u\n",$loads,$return_frame
    end
    if $returns == $loads && $race->frame_count > $return_frame+3
      printf "SEQUENCE_RETURN_NOT_HANDLED complete=%u\n",$race->race_complete
      quit 1
    end
  end
  continue
end
break redraw_title_configuration
commands
  silent
  if $returns == 2
    if $loads != 2 || g_slicks_track_playlist.count != 2
      quit 1
    end
    printf "NATIVE_TWO_TRACK_MENU_RACES_RETURN_OK\n"
    quit
  end
  continue
end
continue
