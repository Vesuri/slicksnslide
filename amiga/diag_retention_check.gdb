# RETCHECK=1 build: chunky surfaces of retention and non-retention updates.
break slicks_diag_gameplay_ready
commands
  silent
  printf "RETENTION_CHECK updates=%lu mismatches=%lu particles=%lu first=%lu\n",g_slicks_retention_checks,g_slicks_retention_mismatches,g_slicks_retention_particle_mismatches,g_slicks_retention_first_mismatch
  printf "GEOMETRY_CACHE checks=%lu mismatches=%lu\n",slicks_geometry_cache_checks,slicks_geometry_cache_mismatches
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  if !g_slicks_retention_checks || g_slicks_retention_mismatches || g_slicks_retention_particle_mismatches || slicks_geometry_cache_mismatches || g_slicks_diag_race_error
    printf "RETENTION_CHECK_FAILED\n"
    quit 1
  end
  quit
end
break slicks_diag_system_restored
commands
  printf "RETENTION_CHECK_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
