# Natural BASIC race with fuel option 10. No car/fuel/AI state injection.
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_FUEL_LOAD_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
break slicks_diag_results_ready
commands 2 3
  silent
  printf "SLICKS_FUEL FRAME=%u COMPLETE=%u\n", g_slicks_diag_race_frame, g_slicks_diag_race_complete
  set $car = 0
  set $stops = 0
  set $positions = 0
  set $finished = 0
  while $car < 4
    printf "CAR=%u FUEL=%d CAPACITY=%d SERVICE=%d REQUESTS=%u REFUEL_FRAMES=%u DEPARTURES=%u X=%d Y=%d LAP=%u\n", $car, g_slicks_diag_fuel[$car], g_slicks_diag_fuel_capacity[$car], g_slicks_diag_service[$car], g_slicks_diag_service_requests[$car], g_slicks_diag_refuel_frames[$car], g_slicks_diag_service_departures[$car], g_slicks_diag_car_x[$car], g_slicks_diag_car_y[$car], g_slicks_diag_lap[$car]
    printf "SURFACE=%u LAYER=%u SPEED=%d TARGET=%d,%d PIT_FRAMES=%u\n", g_slicks_diag_fuel_surface[$car], g_slicks_diag_fuel_layer[$car], g_slicks_diag_fuel_speed[$car], g_slicks_diag_fuel_target_x[$car], g_slicks_diag_fuel_target_y[$car], g_slicks_diag_pit_frames[$car]
    if g_slicks_diag_service_requests[$car] && g_slicks_diag_refuel_frames[$car] && g_slicks_diag_service_departures[$car]
      set $stops = $stops + 1
    end
    if g_slicks_diag_finished[$car] && g_slicks_diag_lap[$car] == 5
      set $finished = $finished + 1
    end
    set $position = g_slicks_diag_finish_position[$car]
    if $position >= 1 && $position <= 4
      set $positions = $positions | (1 << $position)
    end
    set $car = $car + 1
  end
  printf "STATUS_CHECKS=%u STATUS_PIXEL_FAILURES=%u\n", g_slicks_diag_status_checks, g_slicks_diag_status_failures
  if $stops != 4 || $finished != 4 || $positions != 30 || !g_slicks_diag_race_complete || !g_slicks_diag_results_drawn || g_slicks_diag_status_checks < 3 || g_slicks_diag_status_failures
    printf "SLICKS_FUEL_RACE_FAILED STOPS=%u FINISHED=%u POSITIONS=%u RESULTS=%u\n", $stops, $finished, $positions, g_slicks_diag_results_drawn
    quit 1
  end
  printf "SLICKS_FUEL_RACE_OK CARS=%u\n", $stops
  quit
end
continue
