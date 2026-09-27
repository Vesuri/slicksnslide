# SLICKS_BENCHMARK_DETAIL=8: dump target-side PC samples at the checkpoint.
# No debugger stops occur during measured updates.
break slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_error || !g_slicks_diag_bench_frames || g_slicks_audio_vbi_spills || g_slicks_diag_audio_blank_spills
    printf "BENCHMARK_INVALID error=%u frames=%lu spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_bench_frames,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "BENCHMARK frames=%lu max_work_lines=%lu max_frame=%lu over_budget=%lu max_wall_lines=%lu cadence_lines=%lu cadence_count=%lu\n",g_slicks_diag_bench_frames,g_slicks_diag_bench_work_max,g_slicks_diag_bench_work_max_frame,g_slicks_diag_bench_work_over,g_slicks_diag_bench_wall_max,g_slicks_diag_bench_cadence_sum,g_slicks_diag_bench_cadence_count
  printf "WORK_SUM=%lu frames=%lu\n",g_slicks_diag_bench_work_sum,g_slicks_diag_bench_frames
  printf "LIVE_STATS=%u (dirty-area/sparse/audio snapshots valid only when 1)\n",g_slicks_diag_live_stats
  printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",g_slicks_diag_skidmarks,g_slicks_diag_car_x[0],g_slicks_diag_car_x[1],g_slicks_diag_car_x[2],g_slicks_diag_car_x[3],g_slicks_diag_car_y[0],g_slicks_diag_car_y[1],g_slicks_diag_car_y[2],g_slicks_diag_car_y[3]
  printf "PC_SAMPLES count=%lu capacity=%lu missed=%lu timer_bit=%u\n",g_slicks_pc_sample_count,g_slicks_pc_sample_capacity,g_slicks_pc_sample_missed,g_slicks_pc_sampler_bit
  printf "PC_FRAME_INDEX=active-window first=%lu\n",g_slicks_pc_sample_first_frame
  set $sample=0
  while $sample<g_slicks_diag_bench_frames && $sample<704
    printf "WORK_SAMPLE index=%u lines=%lu particles=%u\n",$sample,g_slicks_diag_bench_work_samples[$sample],g_slicks_diag_bench_particle_samples[$sample]
    set $sample=$sample+1
  end
  printf "TEXT_BASE=0x%x\n", &slicks_race_step
  printf "CODE_BASE=0x%x\n", &slicks_advance_particles
  if g_slicks_pc_sample_count
    eval "dump binary memory ../tmp/pc-samples-latest.bin 0x%x 0x%x", g_slicks_pc_samples, g_slicks_pc_samples+g_slicks_pc_sample_count*8
  end
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
