# CATPROBE with the 10001-file fixture. Bounded observation, not a release gate.
set $enumerated = 0
break slicks_diag_catalogue_progress
commands
  silent
  printf "CATALOGUE_PHASE phase=%u progress=%u\n",g_slicks_diag_catalogue_phase,g_slicks_diag_catalogue_progress
  shell date -u '+CATALOGUE_HOST_TIME %s'
  if g_slicks_diag_catalogue_phase==2
    if g_slicks_diag_catalogue_progress!=10000
      quit 1
    end
    set $enumerated = 1
  end
  if g_slicks_diag_catalogue_phase==3 && g_slicks_diag_catalogue_progress>=1024
    if !$enumerated
      quit 1
    end
    printf "CATALOGUE_ENUMERATION_AND_SORT_PROGRESS_OBSERVED\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "CATALOGUE_PHASE_EARLY_EXIT\n"
  quit 1
end
continue
