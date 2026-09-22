break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_TRACK_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  set $ok = g_slicks_diag_race_frame == 200
  set $ok = $ok && g_slicks_diag_track_zones == 25
  set $ok = $ok && g_slicks_diag_timer[0] == 218
  set $ok = $ok && g_slicks_diag_skidmarks > 0
  set $ok = $ok && g_slicks_diag_track_collisions == 0
  set $ok = $ok && g_slicks_diag_waypoint[0] > 0
  set $ok = $ok && g_slicks_diag_waypoint[1] > 0
  set $ok = $ok && g_slicks_diag_waypoint[2] > 0
  set $ok = $ok && g_slicks_diag_waypoint[3] > 0
  if !$ok
    printf "SLICKS_TRACK_FAILED FRAME=%u ZONES=%u SKIDS=%u TRACKCOLL=%u TIMER=%u WAYPOINT=%u SPEED=%d MATERIAL=%u X=%d Y=%d\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones, g_slicks_diag_skidmarks, g_slicks_diag_track_collisions, g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_speed[0], g_slicks_diag_material[0], g_slicks_diag_car_x[0], g_slicks_diag_car_y[0]
    quit 1
  end
  printf "SLICKS_TRACK_OK FRAME=%u ZONES=%u SKIDS=%u TRACKCOLL=%u TIMER=%u WAYPOINTS=%u,%u,%u,%u X=%d Y=%d CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones, g_slicks_diag_skidmarks, g_slicks_diag_track_collisions, g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_waypoint[1], g_slicks_diag_waypoint[2], g_slicks_diag_waypoint[3], g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
