break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_LAP_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame
    if g_slicks_diag_race_frame == 0
      printf "SLICKS_LAP_START TARGET=%u\n", g_slicks_diag_target_frame
      disable 1
    end
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  set $ok = g_slicks_diag_race_frame == 700
  set $ok = $ok && g_slicks_diag_lap[0] == 2
  set $ok = $ok && g_slicks_diag_lap_timer[0] == 126
  set $ok = $ok && g_slicks_diag_timer[0] == 1218
  set $ok = $ok && g_slicks_diag_waypoint[0] == 1
  set $ok = $ok && g_slicks_diag_car_x[0] == 25334
  set $ok = $ok && g_slicks_diag_car_y[0] == 12048
  if !$ok
    printf "SLICKS_LAP_FAILED FRAME=%u LAP=%u LTIME=%u TIMER=%u WAYPOINT=%u TRACKCOLL=%u X=%d Y=%d\n", g_slicks_diag_race_frame, g_slicks_diag_lap[0], g_slicks_diag_lap_timer[0], g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_track_collisions, g_slicks_diag_car_x[0], g_slicks_diag_car_y[0]
    quit 1
  end
  printf "SLICKS_LAP_OK FRAME=%u LAP=%u LTIME=%u TIMER=%u WAYPOINT=%u TRACKCOLL=%u X=%d Y=%d CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_lap[0], g_slicks_diag_lap_timer[0], g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_track_collisions, g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
