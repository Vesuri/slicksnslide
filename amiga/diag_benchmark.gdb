# SLICKS_BENCHMARK=1: no debugger stops during measured updates.
break slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_error || !g_slicks_diag_bench_frames || g_slicks_audio_vbi_spills || g_slicks_diag_audio_blank_spills
    printf "BENCHMARK_INVALID error=%u frames=%lu spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_bench_frames,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "BENCHMARK frames=%lu max_work_lines=%lu max_frame=%lu over_budget=%lu max_wall_lines=%lu cadence_lines=%lu cadence_count=%lu\n",g_slicks_diag_bench_frames,g_slicks_diag_bench_work_max,g_slicks_diag_bench_work_max_frame,g_slicks_diag_bench_work_over,g_slicks_diag_bench_wall_max,g_slicks_diag_bench_cadence_sum,g_slicks_diag_bench_cadence_count
  printf "BENCHMARK_LAST step=%lu audio=%lu c2p=%lu diag=%lu restore=%lu advance=%lu update=%lu hud=%lu draw=%lu\n",g_slicks_diag_profile_step_lines,g_slicks_diag_profile_audio_lines,g_slicks_diag_profile_c2p_lines,g_slicks_diag_profile_diag_lines,g_slicks_diag_profile_restore_lines,g_slicks_diag_profile_advance_lines,g_slicks_diag_profile_update_lines,g_slicks_diag_profile_hud_lines,g_slicks_diag_profile_draw_lines
  printf "ACTOR_RESTORE index=%lu high=%lu cars_p3=%lu low=%lu\n",g_slicks_diag_profile_actor_lines[1],g_slicks_diag_profile_actor_lines[2],g_slicks_diag_profile_actor_lines[3],g_slicks_diag_profile_actor_lines[4]
  printf "ACTOR_DRAW index=%lu low=%lu cars_p3=%lu high=%lu\n",g_slicks_diag_profile_actor_lines[11],g_slicks_diag_profile_actor_lines[12],g_slicks_diag_profile_actor_lines[13],g_slicks_diag_profile_actor_lines[14]
  printf "BENCHMARK_LOAD particles=%u dirty_pixels=%u dirty_ranges=%u dirty_equivalent_rows=%u\n",g_slicks_diag_particles,g_slicks_diag_dirty_pixels,g_slicks_diag_dirty_ranges,g_slicks_diag_dirty_rows
  printf "SPARSE_CONVERTED=%u\n",g_slicks_diag_sparse_converted
  printf "SIMULATION_MAX prepare=%lu weapons=%lu tail=%lu\n",g_slicks_diag_bench_max_simulation[0],g_slicks_diag_bench_max_simulation[1],g_slicks_diag_bench_max_simulation[2]
  printf "TAIL_MAX wheels=%lu collision_surface=%lu smoke_contact=%lu finish=%lu\n",g_slicks_diag_bench_max_tail[0],g_slicks_diag_bench_max_tail[1],g_slicks_diag_bench_max_tail[2],g_slicks_diag_bench_max_tail[3]
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
