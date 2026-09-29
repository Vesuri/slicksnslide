set $demo_starts=0
break show_race_load_error
commands
  silent
  printf "DEMO_LIFECYCLE_LOAD_FAILED error=%u\n",g_slicks_diag_race_error
  quit 1
end
break *slicks_race_start
commands
  silent
  set $demo_race=*(struct SlicksRaceRuntime **)($sp+4)
  if $demo_race->demo_flag!=-1 || $demo_race->race_mode!=5 || $demo_race->track_reward
    printf "DEMO_LIFECYCLE_INVALID race setup\n"
    quit 1
  end
  set $demo_starts=$demo_starts+1
  continue
end
break run_record_results
commands
  silent
  printf "DEMO_LIFECYCLE_INVALID normal results reached\n"
  quit 1
end
break slicks_amiga_store_setup
commands
  silent
  printf "DEMO_LIFECYCLE_INVALID setup save reached\n"
  quit 1
end
break slicks_diag_demo_test_done
commands
  silent
  if g_slicks_demo_test_error || $demo_starts!=2 || g_slicks_demo_test_views!=4
    printf "DEMO_LIFECYCLE_FAILED error=%u starts=%u views=%u\n",g_slicks_demo_test_error,$demo_starts,g_slicks_demo_test_views
    quit 1
  end
  printf "DEMO_LIFECYCLE_OK starts=%u views=%u configuration_and_playlist_restored=1\n",$demo_starts,g_slicks_demo_test_views
  continue
end
break slicks_diag_system_restored
commands
  silent
  if g_slicks_demo_test_error || $demo_starts!=2 || g_slicks_demo_test_views!=4
    printf "DEMO_LIFECYCLE_EARLY_EXIT\n"
    quit 1
  end
  printf "DEMO_LIFECYCLE_EXIT_OK\n"
  quit
end
continue
quit 1
