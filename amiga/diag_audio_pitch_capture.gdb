# No playback breakpoints: three five-second, one-engine plateaus.
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || g_slicks_audio_vbi_spills || g_slicks_diag_engine_frequency!=9200 || g_slicks_diag_engine_period!=385
    printf "PITCH_CAPTURE_FAILED restore=%u spills=%lu frequency=%u period=%u\n",g_slicks_diag_restore_status,g_slicks_audio_vbi_spills,g_slicks_diag_engine_frequency,g_slicks_diag_engine_period
    quit 1
  end
  printf "PITCH_CAPTURE_OK restore=31 spills=0 final_frequency=9200 final_period=385\n"
  quit
end
continue
printf "PITCH_CAPTURE_UNEXPECTED_STOP\n"
bt
quit 1
