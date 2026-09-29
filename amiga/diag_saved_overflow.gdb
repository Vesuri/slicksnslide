# CHAMPSAVF with 41 synthetic S*.SSS files in the isolated DH1 directory.
# No file is opened as a save: the resident catalogue must reject overflow.
source diag_menu_rectangles.gdb
source diag_saved_resident.gdb
init-if-undefined $expected_catalogue_error = -2
set $overflow_notices=0
set $overflow_returns=0
set $overflow_races=0
break slicks_race_start
commands
  silent
  set $overflow_races=$overflow_races+1
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if g_slicks_diag_saved_phase != 3 || saved_files_cache.count != $expected_catalogue_error || $saved_enumerations != 1 || !g_slicks_diag_profile_platform->active
    printf "SAVED_CATALOGUE_WARNING_FAILED phase=%u count=%d expected=%d enumerations=%u active=%u\n",g_slicks_diag_saved_phase,saved_files_cache.count,$expected_catalogue_error,$saved_enumerations,g_slicks_diag_profile_platform->active
    quit 1
  end
  set $overflow_notices=$overflow_notices+1
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active || saved_files_cache.count != $expected_catalogue_error
    quit 1
  end
  set $overflow_returns=$overflow_returns+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $overflow_notices != 1 || $overflow_returns != 1 || $overflow_races != 1 || $saved_enumerations != 1 || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 31
    printf "SAVED_OVERFLOW_RETURN_FAILED notices=%u returns=%u races=%u enumerations=%u count=%d\n",$overflow_notices,$overflow_returns,$overflow_races,$saved_enumerations,saved_files_cache.count
    quit 1
  end
  printf "NATIVE_SAVED_CATALOGUE_ERROR_WARNING_RETURN_OK error=%d\n",$expected_catalogue_error
  quit
end
continue
