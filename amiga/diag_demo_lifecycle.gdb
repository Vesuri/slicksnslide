set $demo_starts=0
set $demo_load_failures=0
set $demo_saves=0
set $demo_save_failures=0
set $demo_done=0
break show_race_load_error
commands
  silent
  if (demo_lifecycle_test!=6 && demo_lifecycle_test!=7 && demo_lifecycle_test!=9 && demo_lifecycle_test!=12) || g_slicks_diag_race_error!=(demo_lifecycle_test==12?8:demo_lifecycle_test==9?1:demo_lifecycle_test==6?2:6) || $demo_load_failures || g_slicks_diag_race_load_fault
    printf "DEMO_LIFECYCLE_LOAD_FAILED error=%u\n",g_slicks_diag_race_error
    quit 1
  end
  set $demo_load_failures=$demo_load_failures+1
  continue
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
break *slicks_amiga_store_setup
commands
  silent
  if (demo_lifecycle_test!=8 && demo_lifecycle_test!=10) || title_demo.active || !g_slicks_demo_expected_configuration
    printf "DEMO_LIFECYCLE_INVALID setup save reached\n"
    quit 1
  end
  set $actual_config=*(unsigned char **)($sp+4)
  set $expected_config=(unsigned char *)g_slicks_demo_expected_configuration
  set $config_byte=0
  while $config_byte<sizeof(struct SlicksConfiguration)
    if $actual_config[$config_byte]!=$expected_config[$config_byte]
      printf "DEMO_SAVE_TEMPORARY_CONFIG byte=%u\n",$config_byte
      quit 1
    end
    set $config_byte=$config_byte+1
  end
  set $demo_saves=$demo_saves+1
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if demo_lifecycle_test==8 || demo_lifecycle_test==10
    if ($demo_saves==1 && g_slicks_setup_save_report.result==0) || ($demo_saves==2 && g_slicks_setup_save_report.result!=0) || $demo_saves>2
      printf "DEMO_SAVE_RESULT_FAILED calls=%u result=%u\n",$demo_saves,g_slicks_setup_save_report.result
      quit 1
    end
  end
  continue
end
break slicks_diag_setup_save_failed
commands
  silent
  set $demo_save_failures=$demo_save_failures+1
  continue
end
break slicks_diag_demo_test_done
commands
  silent
  set $demo_done=$demo_done+1
  if display_allocation_test && g_slicks_display_allocation_checks!=10
    printf "DISPLAY_ALLOCATION_CLEANUP_FAILED result=%d\n",g_slicks_display_allocation_checks
    quit 1
  end
  printf "DISPLAY_ALLOCATION_CHECKS %d\n",g_slicks_display_allocation_checks
  set $failure_test=(demo_lifecycle_test==6 || demo_lifecycle_test==7 || demo_lifecycle_test==9 || demo_lifecycle_test==12)
  set $expected_starts=2-$failure_test-(demo_lifecycle_test==10 || demo_lifecycle_test==11)+(demo_lifecycle_test==12)
  set $expected_views=2*($expected_starts-(demo_lifecycle_test==12))
  if g_slicks_demo_test_error || $demo_starts!=$expected_starts || g_slicks_demo_test_views!=$expected_views || $demo_load_failures!=$failure_test
    printf "DEMO_LIFECYCLE_FAILED error=%u starts=%u views=%u\n",g_slicks_demo_test_error,$demo_starts,g_slicks_demo_test_views
    quit 1
  end
  printf "DEMO_LIFECYCLE_OK starts=%u views=%u configuration_and_playlist_restored=1\n",$demo_starts,g_slicks_demo_test_views
  printf "DEMO_LOAD_FAILURES %u\n",$demo_load_failures
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
  if (demo_lifecycle_test==8 || demo_lifecycle_test==10) && ($demo_saves!=2 || $demo_save_failures!=1 || !g_slicks_demo_saved_roundtrip)
    printf "DEMO_SAVE_LIFECYCLE_FAILED saves=%u failures=%u roundtrip=%u\n",$demo_saves,$demo_save_failures,g_slicks_demo_saved_roundtrip
    quit 1
  end
  printf "DEMO_SAVE saves=%u failures=%u roundtrip=%u\n",$demo_saves,$demo_save_failures,g_slicks_demo_saved_roundtrip
  continue
end
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || $demo_done!=1
    printf "DEMO_SYSTEM_RESTORE_FAILED status=%u\n",g_slicks_diag_restore_status
    quit 1
  end
  set $failure_test=(demo_lifecycle_test==6 || demo_lifecycle_test==7 || demo_lifecycle_test==9 || demo_lifecycle_test==12)
  set $expected_starts=2-$failure_test-(demo_lifecycle_test==10 || demo_lifecycle_test==11)+(demo_lifecycle_test==12)
  set $expected_views=2*($expected_starts-(demo_lifecycle_test==12))
  if g_slicks_demo_test_error || $demo_starts!=$expected_starts || g_slicks_demo_test_views!=$expected_views || $demo_load_failures!=$failure_test
    printf "DEMO_LIFECYCLE_EARLY_EXIT\n"
    quit 1
  end
  printf "DEMO_LIFECYCLE_EXIT_OK\n"
  quit
end
continue
quit 1
