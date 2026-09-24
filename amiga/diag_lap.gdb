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
  set $ok = $ok && g_slicks_diag_lap_timer[0] == 178
  set $ok = $ok && g_slicks_diag_timer[0] == 1218
  set $ok = $ok && g_slicks_diag_waypoint[0] == 1
  set $ok = $ok && g_slicks_diag_car_x[0] == 25404
  set $ok = $ok && g_slicks_diag_car_y[0] == 13936
  # State-5 actors leave permanent marks after retirement (DOS 3000:3e36).
  set $ok = $ok && g_slicks_diag_checksum == 0x3d08d41f
  set $ok = $ok && g_slicks_diag_display_checksum == 0x30ad3861
  set $ok = $ok && g_slicks_diag_profile_total_lines <= 312
  if !$ok
    printf "SLICKS_LAP_FAILED FRAME=%u LAP=%u LTIME=%u TIMER=%u WAYPOINT=%u TRACKCOLL=%u X=%d Y=%d\n", g_slicks_diag_race_frame, g_slicks_diag_lap[0], g_slicks_diag_lap_timer[0], g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_track_collisions, g_slicks_diag_car_x[0], g_slicks_diag_car_y[0]
    quit 1
  end
  printf "SLICKS_LAP_OK FRAME=%u LAP=%u LTIME=%u TIMER=%u WAYPOINT=%u TRACKCOLL=%u X=%d Y=%d CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_lap[0], g_slicks_diag_lap_timer[0], g_slicks_diag_timer[0], g_slicks_diag_waypoint[0], g_slicks_diag_track_collisions, g_slicks_diag_car_x[0], g_slicks_diag_car_y[0], g_slicks_diag_checksum, g_slicks_diag_display_checksum
  printf "SLICKS_LAP_PROFILE PARTICLES=%u STEP=%u AUDIO=%u C2P=%u DIAG=%u TOTAL=%u\n", g_slicks_diag_particles, g_slicks_diag_profile_step_lines, g_slicks_diag_profile_audio_lines, g_slicks_diag_profile_c2p_lines, g_slicks_diag_profile_diag_lines, g_slicks_diag_profile_total_lines
  printf "SLICKS_LAP_PHASES RESTORE=%u ADVANCE=%u UPDATE=%u HUD=%u DRAW=%u\n", g_slicks_diag_profile_restore_lines, g_slicks_diag_profile_advance_lines, g_slicks_diag_profile_update_lines, g_slicks_diag_profile_hud_lines, g_slicks_diag_profile_draw_lines
  printf "SLICKS_LAP_DIRTY RANGES=%u ROWS=%u PIXELS=%u TOTAL_CALLS=%u TOTAL_ROWS=%u\n", g_slicks_diag_dirty_ranges, g_slicks_diag_dirty_rows, g_slicks_diag_dirty_pixels, g_slicks_diag_dirty_c2p_calls, g_slicks_diag_dirty_c2p_rows
  quit
end
continue
