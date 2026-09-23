break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_GAMEPLAY_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit
  end
  if g_slicks_diag_ingame
    if g_slicks_diag_checksum != 0xea8fd88d || g_slicks_diag_surface_checksum != 0xaa8bc219 || !g_slicks_diag_start_light_visible || g_slicks_diag_start_light_stage_mask != 1
      printf "SLICKS_GAMEPLAY_START_FAILED VISIBLE=%u LIGHTS=%u SURFACE=%08x CHECKSUM=%08x\n", g_slicks_diag_start_light_visible, g_slicks_diag_start_light_stage_mask, g_slicks_diag_surface_checksum, g_slicks_diag_checksum
      quit 1
    end
    printf "SLICKS_GAMEPLAY_START FRAME=%u SKIDS=%u TIMER=%u LIGHTS=%u X=%d Y=%d MATERIAL=%08x CHECKSUM=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_skidmarks, g_slicks_diag_timer[0], g_slicks_diag_start_light_stage_mask, g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_material_checksum, g_slicks_diag_checksum
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  set $ok = g_slicks_diag_race_frame == 200 && g_slicks_diag_timer[0] == 218 && g_slicks_diag_skidmarks > 0
  set $ok = $ok && g_slicks_diag_collisions > 0
  set $ok = $ok && g_slicks_diag_countdown_stage == 6
  set $ok = $ok && g_slicks_diag_audio_ready && g_slicks_diag_engine_started
  set $ok = $ok && g_slicks_diag_engine_sample_block == 18
  set $vx = g_slicks_diag_car_vx[0]
  set $vy = g_slicks_diag_car_vy[0]
  if $vx < 0
    set $vx = -$vx
  end
  if $vy < 0
    set $vy = -$vy
  end
  set $magnitude = ($vx + $vy) / 2
  set $ok = $ok && g_slicks_diag_engine_frequency == 2000 + 4 * $magnitude
  set $ok = $ok && g_slicks_diag_engine_period == 3546895 / g_slicks_diag_engine_frequency
  set $ok = $ok && g_slicks_diag_effect_sample_block == 6
  set $ok = $ok && g_slicks_diag_effect_priority == 14
  set $ok = $ok && g_slicks_diag_sound_event_totals[2] + g_slicks_diag_sound_event_totals[3] + g_slicks_diag_sound_event_totals[4] > 0
  set $ok = $ok && !g_slicks_diag_start_light_visible
  set $ok = $ok && g_slicks_diag_start_light_stage_mask == 15
  set $ok = $ok && g_slicks_diag_dirty_ranges > 0
  set $ok = $ok && g_slicks_diag_dirty_rows > 0 && g_slicks_diag_dirty_rows < 200
  set $ok = $ok && g_slicks_diag_dirty_c2p_rows < 40000
  set $ok = $ok && g_slicks_diag_lap[0] == 1 && g_slicks_diag_lap_timer[0] == 218
  set $ok = $ok && (g_slicks_diag_car_x[0] != 25500 || g_slicks_diag_car_y[0] != 5700)
  set $ok = $ok && (g_slicks_diag_car_x[1] != 26300 || g_slicks_diag_car_y[1] != 5700)
  set $ok = $ok && (g_slicks_diag_car_x[2] != 25500 || g_slicks_diag_car_y[2] != 4900)
  set $ok = $ok && (g_slicks_diag_car_x[3] != 26300 || g_slicks_diag_car_y[3] != 4900)
  set $ok = $ok && g_slicks_diag_acceleration[0] == 80 && g_slicks_diag_acceleration[1] == 87
  set $ok = $ok && g_slicks_diag_acceleration[2] == 100 && g_slicks_diag_acceleration[3] == 100
  set $ok = $ok && g_slicks_diag_steering[0] == 100 && g_slicks_diag_steering[1] == 106
  set $ok = $ok && g_slicks_diag_steering[2] == 100 && g_slicks_diag_steering[3] == 100
  if !$ok
    printf "SLICKS_GAMEPLAY_FAILED FRAME=%u STAGE=%u SKIDS=%u COLLISIONS=%u TRACKCOLL=%u TIMER=%u X=%d Y=%d C1X=%d C1Y=%d C2X=%d C2Y=%d C3X=%d C3Y=%d\n", g_slicks_diag_race_frame, g_slicks_diag_countdown_stage, g_slicks_diag_skidmarks, g_slicks_diag_collisions, g_slicks_diag_track_collisions, g_slicks_diag_timer[0], g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_car_x[1], g_slicks_diag_car_y[1], g_slicks_diag_car_x[2], g_slicks_diag_car_y[2], g_slicks_diag_car_x[3], g_slicks_diag_car_y[3]
    quit 1
  end
  printf "SLICKS_DIRTY RANGES=%u ROWS=%u TOTAL_CALLS=%u TOTAL_ROWS=%u\n", g_slicks_diag_dirty_ranges, g_slicks_diag_dirty_rows, g_slicks_diag_dirty_c2p_calls, g_slicks_diag_dirty_c2p_rows
  printf "SLICKS_PROFILE STEP=%u AUDIO=%u C2P=%u DIAG=%u TOTAL=%u\n", g_slicks_diag_profile_step_vblanks, g_slicks_diag_profile_audio_vblanks, g_slicks_diag_profile_c2p_vblanks, g_slicks_diag_profile_diag_vblanks, g_slicks_diag_profile_total_vblanks
  printf "SLICKS_PROFILE_LINES STEP=%u AUDIO=%u C2P=%u DIAG=%u TOTAL=%u\n", g_slicks_diag_profile_step_lines, g_slicks_diag_profile_audio_lines, g_slicks_diag_profile_c2p_lines, g_slicks_diag_profile_diag_lines, g_slicks_diag_profile_total_lines
  printf "SLICKS_GAMEPLAY_OK FRAME=%u STAGE=%u SKIDS=%u COLLISIONS=%u TRACKCOLL=%u TIMER=%u LAP=%u LTIME=%u WAYPOINTS=%u,%u,%u,%u X=%d Y=%d C1X=%d C1Y=%d C2X=%d C2Y=%d C3X=%d C3Y=%d CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_countdown_stage, g_slicks_diag_skidmarks, g_slicks_diag_collisions, g_slicks_diag_track_collisions, g_slicks_diag_timer[0], g_slicks_diag_lap[0], g_slicks_diag_lap_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_waypoint[1], g_slicks_diag_waypoint[2], g_slicks_diag_waypoint[3], g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_car_x[1], g_slicks_diag_car_y[1], g_slicks_diag_car_x[2], g_slicks_diag_car_y[2], g_slicks_diag_car_x[3], g_slicks_diag_car_y[3], g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
