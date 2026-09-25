# One vehicle-0 engine, speeds 0/1000/500/0, five seconds each.
# No per-stage stops that could contaminate the recorded audio.
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || g_slicks_audio_vbi_spills || g_slicks_diag_engine_frequency!=2200 || g_slicks_diag_engine_period!=1612 || g_slicks_diag_pitch_bank_mask!=1
    printf "ENGINE_DECELERATION_FAILED restore=%u spills=%lu frequency=%u period=%u banks=%u\n",g_slicks_diag_restore_status,g_slicks_audio_vbi_spills,g_slicks_diag_engine_frequency,g_slicks_diag_engine_period,g_slicks_diag_pitch_bank_mask
    quit 1
  end
  printf "ENGINE_DECELERATION_OK restore=31 spills=0 final_frequency=2200 final_period=1612 banks=1\n"
  quit
end
continue
quit 1
