# SLICKS_BENCHMARK=1: no debugger stops during measured updates.
break slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_error || !g_slicks_diag_bench_frames || g_slicks_audio_vbi_spills
    printf "BENCHMARK_INVALID error=%u frames=%lu spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_bench_frames,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "BENCHMARK frames=%lu max_work_lines=%lu max_frame=%lu over_budget=%lu max_wall_lines=%lu cadence_lines=%lu cadence_count=%lu\n",g_slicks_diag_bench_frames,g_slicks_diag_bench_work_max,g_slicks_diag_bench_work_max_frame,g_slicks_diag_bench_work_over,g_slicks_diag_bench_wall_max,g_slicks_diag_bench_cadence_sum,g_slicks_diag_bench_cadence_count
  printf "BENCHMARK_LAST step=%lu audio=%lu c2p=%lu diag=%lu restore=%lu advance=%lu update=%lu hud=%lu draw=%lu\n",g_slicks_diag_profile_step_lines,g_slicks_diag_profile_audio_lines,g_slicks_diag_profile_c2p_lines,g_slicks_diag_profile_diag_lines,g_slicks_diag_profile_restore_lines,g_slicks_diag_profile_advance_lines,g_slicks_diag_profile_update_lines,g_slicks_diag_profile_hud_lines,g_slicks_diag_profile_draw_lines
  printf "ACTOR_RESTORE index=%lu high=%lu cars_p3=%lu low=%lu\n",g_slicks_diag_profile_actor_lines[1],g_slicks_diag_profile_actor_lines[2],g_slicks_diag_profile_actor_lines[3],g_slicks_diag_profile_actor_lines[4]
  printf "ACTOR_DRAW index=%lu low=%lu cars_p3=%lu high=%lu\n",g_slicks_diag_profile_actor_lines[11],g_slicks_diag_profile_actor_lines[12],g_slicks_diag_profile_actor_lines[13],g_slicks_diag_profile_actor_lines[14]
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
