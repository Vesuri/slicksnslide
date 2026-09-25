# SLICKS_AUDIO_PITCH_TEST=2: full -> half -> quarter -> half -> full.
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || g_slicks_audio_vbi_spills || g_slicks_diag_pitch_bank_mask!=7 || g_slicks_diag_engine_frequency!=2200 || g_slicks_diag_engine_period!=1612
    printf "EXTREME_PITCH_FAILED restore=%u spills=%lu banks=%u frequency=%u period=%u\n",g_slicks_diag_restore_status,g_slicks_audio_vbi_spills,g_slicks_diag_pitch_bank_mask,g_slicks_diag_engine_frequency,g_slicks_diag_engine_period
    quit 1
  end
  printf "EXTREME_PITCH_OK restore=31 spills=0 banks=7 final_frequency=2200 final_period=1612\n"
  quit
end
continue
printf "EXTREME_PITCH_UNEXPECTED_STOP\n"
bt
quit 1
