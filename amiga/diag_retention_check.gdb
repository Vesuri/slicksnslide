# RETCHECK=1 build: chunky surfaces of retention and non-retention updates.
break slicks_diag_gameplay_ready
commands
  silent
  printf "RETENTION_CHECK updates=%lu mismatches=%lu first=%lu\n",g_slicks_retention_checks,g_slicks_retention_mismatches,g_slicks_retention_first_mismatch
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  quit
end
break slicks_diag_system_restored
commands
  printf "RETENTION_CHECK_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
