# SHADOW=1 build: report native/reference dual-execution results at the
# benchmark checkpoint. Timing is irrelevant here; warp is allowed.
set $race=(struct SlicksRaceRuntime *)0
set $probes=0
break slicks_track_actor_probe
commands
  silent
  set $probes=$probes+1
  continue
end
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  printf "SHADOW_START sites=0x%lx\n",slicks_shadow_sites
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "SHADOW_SITES=0x%lx\n",slicks_shadow_sites
  if $race
    printf "TRACK_HITS probes=%u collisions=%lu special=%d,%d,%d,%d\n",$probes,$race->track_collision_count,$race->cars[0].special_drive_state,$race->cars[1].special_drive_state,$race->cars[2].special_drive_state,$race->cars[3].special_drive_state
  end
  printf "SHADOW calls=%lu,%lu,%lu,%lu,%lu,%lu,%lu mismatches=%lu first_site=%lu block=%lu frame=%lu state=0x%x\n",slicks_shadow_calls[1],slicks_shadow_calls[2],slicks_shadow_calls[3],slicks_shadow_calls[4],slicks_shadow_calls[5],slicks_shadow_calls[6],slicks_shadow_calls[7],slicks_shadow_mismatches[0],slicks_shadow_first_site,slicks_shadow_first_block,slicks_shadow_first_frame,slicks_shadow_state
  printf "RACE_ERROR=%u frames=%lu\n",g_slicks_diag_race_error,g_slicks_diag_bench_frames
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  if slicks_shadow_mismatches[0]
    set $i=0
    while $i<1024
      if slicks_shadow_native[$i]!=slicks_shadow_reference[$i]
        printf "DIFF offset=%lu native=0x%02x reference=0x%02x\n",slicks_shadow_first_block*1024+$i,slicks_shadow_native[$i],slicks_shadow_reference[$i]
      end
      set $i=$i+1
    end
  end
  if slicks_shadow_mismatches[0] || g_slicks_diag_race_error
    quit 1
  end
  if !(slicks_shadow_sites & 126)
    printf "SHADOW_INVALID no implemented sites selected\n"
    quit 1
  end
  set $site=1
  while $site<=6
    if (slicks_shadow_sites & (1<<$site)) && !slicks_shadow_calls[$site]
      printf "SHADOW_INVALID selected site %u was not exercised\n",$site
      quit 1
    end
    set $site=$site+1
  end
  quit
end
break slicks_diag_system_restored
commands
  printf "SHADOW_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
