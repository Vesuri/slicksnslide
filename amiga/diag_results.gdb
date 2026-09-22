break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_RESULTS_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame && g_slicks_diag_race_frame == 0
    printf "SLICKS_RESULTS_START TARGET=%u\n", g_slicks_diag_target_frame
    disable 1
  end
  continue
end
break slicks_diag_results_ready
commands
  silent
  set $ok = g_slicks_diag_race_complete && g_slicks_diag_results_drawn
  set $ok = $ok && g_slicks_diag_audio_ready && g_slicks_diag_music_started
  set $ok = $ok && g_slicks_diag_finished[0] && g_slicks_diag_finished[1]
  set $ok = $ok && g_slicks_diag_finished[2] && g_slicks_diag_finished[3]
  set $sum = g_slicks_diag_finish_position[0] + g_slicks_diag_finish_position[1]
  set $sum = $sum + g_slicks_diag_finish_position[2] + g_slicks_diag_finish_position[3]
  set $ok = $ok && $sum == 10
  if !$ok
    printf "SLICKS_RESULTS_FAILED FRAME=%u COMPLETE=%u DRAWN=%u FINISHED=%u,%u,%u,%u POS=%u,%u,%u,%u LAPS=%u,%u,%u,%u WAYPOINTS=%u,%u,%u,%u\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete, g_slicks_diag_results_drawn, g_slicks_diag_finished[0], g_slicks_diag_finished[1], g_slicks_diag_finished[2], g_slicks_diag_finished[3], g_slicks_diag_finish_position[0], g_slicks_diag_finish_position[1], g_slicks_diag_finish_position[2], g_slicks_diag_finish_position[3], g_slicks_diag_lap[0], g_slicks_diag_lap[1], g_slicks_diag_lap[2], g_slicks_diag_lap[3], g_slicks_diag_waypoint[0], g_slicks_diag_waypoint[1], g_slicks_diag_waypoint[2], g_slicks_diag_waypoint[3]
    quit 1
  end
  printf "SLICKS_RESULTS_OK FRAME=%u POS=%u,%u,%u,%u CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_finish_position[0], g_slicks_diag_finish_position[1], g_slicks_diag_finish_position[2], g_slicks_diag_finish_position[3], g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "SLICKS_RESULTS_TIMEOUT FRAME=%u COMPLETE=%u DRAWN=%u FINISHED=%u,%u,%u,%u POS=%u,%u,%u,%u LAPS=%u,%u,%u,%u WAYPOINTS=%u,%u,%u,%u X=%d,%d,%d,%d Y=%d,%d,%d,%d\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete, g_slicks_diag_results_drawn, g_slicks_diag_finished[0], g_slicks_diag_finished[1], g_slicks_diag_finished[2], g_slicks_diag_finished[3], g_slicks_diag_finish_position[0], g_slicks_diag_finish_position[1], g_slicks_diag_finish_position[2], g_slicks_diag_finish_position[3], g_slicks_diag_lap[0], g_slicks_diag_lap[1], g_slicks_diag_lap[2], g_slicks_diag_lap[3], g_slicks_diag_waypoint[0], g_slicks_diag_waypoint[1], g_slicks_diag_waypoint[2], g_slicks_diag_waypoint[3], g_slicks_diag_car_x[0], g_slicks_diag_car_x[1], g_slicks_diag_car_x[2], g_slicks_diag_car_x[3], g_slicks_diag_car_y[0], g_slicks_diag_car_y[1], g_slicks_diag_car_y[2], g_slicks_diag_car_y[3]
  quit 1
end
continue
