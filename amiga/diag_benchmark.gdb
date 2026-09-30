# SLICKS_BENCHMARK=1: no debugger stops during measured updates.
set $race=(struct SlicksRaceRuntime *)0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "INNER_PROFILE_ENABLED=%u\n",slicks_race_inner_profile_enabled
  if g_slicks_diag_profile_all!=2 && !slicks_race_inner_profile_enabled
    printf "BENCHMARK_INVALID rebuild with make INNER_PROFILE=1\n"
    quit 1
  end
  if g_slicks_diag_race_error || !g_slicks_diag_bench_frames || g_slicks_audio_vbi_spills || g_slicks_diag_audio_blank_spills
    printf "BENCHMARK_INVALID error=%u frames=%lu spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_bench_frames,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "BENCHMARK frames=%lu max_work_lines=%lu max_frame=%lu over_budget=%lu max_wall_lines=%lu cadence_lines=%lu cadence_count=%lu\n",g_slicks_diag_bench_frames,g_slicks_diag_bench_work_max,g_slicks_diag_bench_work_max_frame,g_slicks_diag_bench_work_over,g_slicks_diag_bench_wall_max,g_slicks_diag_bench_cadence_sum,g_slicks_diag_bench_cadence_count
  printf "INITIAL_CACHE_CONTROL=0x%lx\n",g_slicks_diag_initial_cache_control
  printf "PROFILE_DETAIL=%u (stage breakdowns valid only when 1)\n",g_slicks_diag_profile_all==1
  printf "LIVE_STATS=%u (dirty-area/sparse/audio snapshots valid only when 1)\n",g_slicks_diag_live_stats
  printf "PROFILE_MODE=%u (1=full,2=outer,3=stages,4=actors,5=simulation,6=tail,7=cars,8=actor_motion)\n",g_slicks_diag_profile_all
  if g_slicks_diag_profile_all==2 && (g_slicks_diag_bench_stage_sum[0] || g_slicks_diag_bench_stage_sum[1] || g_slicks_diag_bench_stage_sum[2] || g_slicks_diag_bench_stage_sum[3] || g_slicks_diag_bench_stage_sum[4])
    printf "BENCHMARK_INVALID unexpected intra-update callbacks\n"
    quit 1
  end
  printf "WORK_SUM=%lu frames=%lu\n",g_slicks_diag_bench_work_sum,g_slicks_diag_bench_frames
  printf "LATE_PUBLICATIONS=%lu\n",g_slicks_diag_late_publications
  if g_slicks_diag_bench_frames>704
    printf "BENCHMARK_INVALID sample capacity exceeded\n"
    quit 1
  end
  set $sample=0
  set $sample_sum=0
  while $sample<g_slicks_diag_bench_frames
    printf "WORK_SAMPLE index=%u lines=%lu particles=%u\n",$sample,g_slicks_diag_bench_work_samples[$sample],g_slicks_diag_bench_particle_samples[$sample]
    set $sample_sum=$sample_sum+g_slicks_diag_bench_work_samples[$sample]
    set $sample=$sample+1
  end
  if $sample_sum!=g_slicks_diag_bench_work_sum
    printf "BENCHMARK_INVALID sample sum mismatch\n"
    quit 1
  end
  printf "STAGE_SUM restore=%lu advance=%lu update=%lu hud=%lu draw=%lu audio=%lu c2p=%lu diag=%lu\n",g_slicks_diag_bench_stage_sum[0],g_slicks_diag_bench_stage_sum[1],g_slicks_diag_bench_stage_sum[2],g_slicks_diag_bench_stage_sum[3],g_slicks_diag_bench_stage_sum[4],g_slicks_diag_bench_stage_sum[5],g_slicks_diag_bench_stage_sum[6],g_slicks_diag_bench_stage_sum[7]
  printf "TAIL_SUM wheels=%lu collision_surface=%lu smoke_contact=%lu finish=%lu\n",g_slicks_diag_bench_tail_sum[0],g_slicks_diag_bench_tail_sum[1],g_slicks_diag_bench_tail_sum[2],g_slicks_diag_bench_tail_sum[3]
  printf "SIMULATION_SUM prepare=%lu weapons=%lu tail=%lu\n",g_slicks_diag_bench_simulation_sum[0],g_slicks_diag_bench_simulation_sum[1],g_slicks_diag_bench_simulation_sum[2]
  printf "ACTOR_SUM restore_index=%lu restore_high=%lu restore_cars_p3=%lu restore_low=%lu draw_index=%lu draw_low=%lu draw_cars_p3=%lu draw_high=%lu\n",g_slicks_diag_bench_actor_sum[0],g_slicks_diag_bench_actor_sum[1],g_slicks_diag_bench_actor_sum[2],g_slicks_diag_bench_actor_sum[3],g_slicks_diag_bench_actor_sum[4],g_slicks_diag_bench_actor_sum[5],g_slicks_diag_bench_actor_sum[6],g_slicks_diag_bench_actor_sum[7]
  printf "CAR_SUM cars=%lu points_priority3=%lu\n",g_slicks_diag_bench_car_sum[0],g_slicks_diag_bench_car_sum[1]
  printf "MOTION_SUM track=%lu points=%lu sprites=%lu\n",g_slicks_diag_bench_motion_sum[0],g_slicks_diag_bench_motion_sum[1],g_slicks_diag_bench_motion_sum[2]
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  if $race && $race->raster_clock
    printf "REALTIME_CLOCK ticks=%lu lines=%lu frames=%lu\n",$race->game_clock_ticks,$race->physics_clock_at-$race->physics_clock_origin,$race->frame_count
  end
  if $race
    set $masked=0
    set $unmasked=0
    set $masked_pixels=0
    set $unmasked_pixels=0
    set $i=1
    while $i<$race->weapons.slots.high_water
      set $a=&$race->weapons.actors[$i]
      if $a->kind==3 && $a->saved
        if $a->occlusion
          set $masked=$masked+1
          set $masked_pixels=$masked_pixels+$a->old_width*$a->old_height
        else
          set $unmasked=$unmasked+1
          set $unmasked_pixels=$unmasked_pixels+$a->old_width*$a->old_height
        end
      end
      set $i=$i+1
    end
    printf "TRACK_SPRITE_LOAD masked=%u pixels=%u unmasked=%u pixels=%u\n",$masked,$masked_pixels,$unmasked,$unmasked_pixels
    set $width=1
    while $width<=16
      set $count=0
      set $i=1
      while $i<$race->weapons.slots.high_water
        set $a=&$race->weapons.actors[$i]
        if $a->kind==3 && $a->saved && $a->old_width==$width
          set $count=$count+1
        end
        set $i=$i+1
      end
      if $count
        printf "TRACK_SPRITE_WIDTH width=%u count=%u\n",$width,$count
      end
      set $width=$width+1
    end
  end
  printf "BENCHMARK_LAST step=%lu audio=%lu c2p=%lu diag=%lu restore=%lu advance=%lu update=%lu hud=%lu draw=%lu\n",g_slicks_diag_profile_step_lines,g_slicks_diag_profile_audio_lines,g_slicks_diag_profile_c2p_lines,g_slicks_diag_profile_diag_lines,g_slicks_diag_profile_restore_lines,g_slicks_diag_profile_advance_lines,g_slicks_diag_profile_update_lines,g_slicks_diag_profile_hud_lines,g_slicks_diag_profile_draw_lines
  printf "ACTOR_RESTORE index=%lu high=%lu cars_p3=%lu low=%lu\n",g_slicks_diag_profile_actor_lines[1],g_slicks_diag_profile_actor_lines[2],g_slicks_diag_profile_actor_lines[3],g_slicks_diag_profile_actor_lines[4]
  printf "ACTOR_DRAW index=%lu low=%lu cars_p3=%lu high=%lu\n",g_slicks_diag_profile_actor_lines[11],g_slicks_diag_profile_actor_lines[12],g_slicks_diag_profile_actor_lines[13],g_slicks_diag_profile_actor_lines[14]
  printf "BENCHMARK_LOAD particles=%u dirty_pixels=%u dirty_ranges=%u dirty_equivalent_rows=%u\n",g_slicks_diag_particles,g_slicks_diag_dirty_pixels,g_slicks_diag_dirty_ranges,g_slicks_diag_dirty_rows
  printf "SPARSE_CONVERTED=%u\n",g_slicks_diag_sparse_converted
  printf "SIMULATION_MAX prepare=%lu weapons=%lu tail=%lu\n",g_slicks_diag_bench_max_simulation[0],g_slicks_diag_bench_max_simulation[1],g_slicks_diag_bench_max_simulation[2]
  printf "SPRITE_MAX setup=%lu paint=%lu dirty=%lu\n",g_slicks_diag_bench_max_sprite[0],g_slicks_diag_bench_max_sprite[1],g_slicks_diag_bench_max_sprite[2]
  printf "TAIL_MAX wheels=%lu collision_surface=%lu smoke_contact=%lu finish=%lu\n",g_slicks_diag_bench_max_tail[0],g_slicks_diag_bench_max_tail[1],g_slicks_diag_bench_max_tail[2],g_slicks_diag_bench_max_tail[3]
  printf "DRAW_MAX index=%lu low=%lu cars_p3=%lu high=%lu cars_only=%lu p3_only=%lu rect_pixels=%lu sparse=%u\n",g_slicks_diag_bench_max_actors[4],g_slicks_diag_bench_max_actors[5],g_slicks_diag_bench_max_actors[6],g_slicks_diag_bench_max_actors[7],g_slicks_diag_bench_max_car_draw[0],g_slicks_diag_bench_max_car_draw[1],g_slicks_diag_bench_max_rect_pixels,g_slicks_diag_bench_max_sparse
  printf "BENCHMARK_MAX restore=%lu advance=%lu update=%lu hud=%lu draw=%lu audio=%lu c2p=%lu diag=%lu particles=%u\n",g_slicks_diag_bench_max_stages[0],g_slicks_diag_bench_max_stages[1],g_slicks_diag_bench_max_stages[2],g_slicks_diag_bench_max_stages[3],g_slicks_diag_bench_max_stages[4],g_slicks_diag_bench_max_stages[5],g_slicks_diag_bench_max_stages[6],g_slicks_diag_bench_max_stages[7],g_slicks_diag_bench_max_particles
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "BENCHMARK_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
