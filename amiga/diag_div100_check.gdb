# DIV100CHECK=1: compare every production helper call with ordinary C /100.
break slicks_diag_gameplay_ready
commands
  silent
  printf "DIV100_CHECK calls=%lu mismatches=%lu first_input=%ld actual=%ld expected=%ld\n",slicks_div100_checks,slicks_div100_mismatches,slicks_div100_first_input,slicks_div100_first_actual,slicks_div100_first_expected
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  if slicks_div100_checks<1000 || slicks_div100_mismatches || g_slicks_diag_race_error
    printf "DIV100_CHECK_FAILED\n"
    quit 1
  end
  quit
end
break slicks_diag_system_restored
commands
  printf "DIV100_CHECK_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
