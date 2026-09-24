set logging file .run/display-events/events.log
set logging overwrite on
set logging on
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "DISPLAY_TRACE_ERROR=%u\n", g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame && (g_slicks_diag_race_frame % 50 == 0)
    printf "DISPLAY_TRACE FRAME=%u PARTICLES=%u TRACK_COLLISIONS=%u\n", g_slicks_diag_race_frame, g_slicks_diag_particles, g_slicks_diag_track_collisions
    set $car = 0
    while $car < 4
      printf " CAR=%u X=%d Y=%d MATERIAL=%u WAYPOINT=%u\n", $car, g_slicks_diag_car_x[$car], g_slicks_diag_car_y[$car], g_slicks_diag_material[$car], g_slicks_diag_waypoint[$car]
      set $car = $car + 1
    end
    print g_slicks_diag_sound_event_totals
  end
  if g_slicks_diag_race_frame >= 700
    printf "DISPLAY_TRACE_COMPLETE: continuing without trace breakpoints\n"
    disable 1
    set logging off
  end
  continue
end
continue
