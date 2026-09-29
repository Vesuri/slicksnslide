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
  if (demo_lifecycle_test==2 || demo_lifecycle_test==3) && g_slicks_demo_idle_entries!=2
    printf "DEMO_IDLE_FAILED entries=%u\n",g_slicks_demo_idle_entries
    quit 1
  end
  printf "DEMO_IDLE_ENTRIES %u\n",g_slicks_demo_idle_entries
  printf "DEMO_IDLE_WAIT_AFTER_INPUT_FRAMES %lu\n",g_slicks_demo_idle_wait_frames
  if demo_lifecycle_test==3 && g_slicks_demo_menu_waits!=4
    printf "DEMO_MENU_WAIT_FAILED waits=%u\n",g_slicks_demo_menu_waits
    quit 1
  end
  printf "DEMO_MENU_WAITS %u\n",g_slicks_demo_menu_waits
  if demo_lifecycle_test==4 && g_slicks_loading_io_checks!=2
    printf "LOADING_IO_FAILED checks=%u\n",g_slicks_loading_io_checks
    quit 1
  end
  printf "LOADING_IO checks=%u bytes=%lu hash=%lu\n",g_slicks_loading_io_checks,g_slicks_loading_io_bytes,g_slicks_loading_io_hash
  if demo_lifecycle_test==5 && g_slicks_demo_natural_returns!=2
    printf "DEMO_NATURAL_FAILED returns=%u\n",g_slicks_demo_natural_returns
    quit 1
  end
  printf "DEMO_NATURAL returns=%u frames=%lu,%lu clocks=%lu,%lu deadlines=%lu,%lu\n",g_slicks_demo_natural_returns,g_slicks_demo_return_frames[0],g_slicks_demo_return_frames[1],g_slicks_demo_return_clocks[0],g_slicks_demo_return_clocks[1],g_slicks_demo_return_deadlines[0],g_slicks_demo_return_deadlines[1]
  continue
end
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31
    printf "DEMO_SYSTEM_RESTORE_FAILED status=%u\n",g_slicks_diag_restore_status
    quit 1
  end
  if g_slicks_demo_test_error || $demo_starts!=2 || g_slicks_demo_test_views!=4
    printf "DEMO_LIFECYCLE_EARLY_EXIT\n"
    quit 1
  end
  printf "DEMO_LIFECYCLE_EXIT_OK\n"
  quit
end
continue
quit 1
