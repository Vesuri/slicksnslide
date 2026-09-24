set $loads = 0
set $starts = 0
set $awards = 0
set $failed = 0
break prepare_race
commands
  silent
  set $loads = $loads+1
  if !session || $loads > 3 || new_game != ($loads == 1)
    quit 1
  end
  if $loads == 2
    if $awards != 1 || $starts != 1
      quit 1
    end
    dump binary memory .run/next-track-failure-v1/before.bin session session+1
    set $track = track_path
  end
  if $loads == 3
    if $awards != 1 || $starts != 1 || !$failed || track_path != $track
      printf "NEXT_TRACK_RETRY_REENTERED_GAME_TRANSITION\n"
      quit 1
    end
    dump binary memory .run/next-track-failure-v1/retry.bin session session+1
  end
  continue
end
break slicks_diag_race_load_failed
commands
  silent
  if $loads != 2 || $failed || g_slicks_diag_race_error != 6 || g_slicks_diag_ingame || !g_slicks_diag_profile_platform->active
    quit 1
  end
  dump binary memory .run/next-track-failure-v1/failed.bin &g_slicks_setup_session &g_slicks_setup_session+1
  set $failed = 1
  printf "NEXT_TRACK_FAILURE_RECOVERED\n"
  continue
end
break *slicks_race_start
commands
  silent
  set $starts = $starts+1
  if $starts > 2 || ($starts == 2 && $loads != 3)
    quit 1
  end
  if $starts == 2
    dump binary memory .run/next-track-failure-v1/started.bin &g_slicks_setup_session &g_slicks_setup_session+1
  end
  continue
end
break slicks_diag_track_rewarded
commands
  silent
  set $awards = $awards+1
  if $awards != $starts || $awards > 2
    printf "NEXT_TRACK_RETRY_DUPLICATE_REWARD\n"
    quit 1
  end
  continue
end
break redraw_title_configuration
commands
  silent
  if $awards
    if $awards != 2 || $starts != 2 || $loads != 3 || !$failed || g_slicks_diag_race_error
      quit 1
    end
    printf "NATIVE_NEXT_TRACK_FAILURE_RETRY_TWO_RACES_REWARDS_ONCE_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "NEXT_TRACK_RECOVERY_EARLY_EXIT\n"
  quit 1
end
continue
