# SLICKS_AUDIO_PCM_TEST=1. Actual target PCM, after active racing begins.
# Dumps remain local-only under .run; compare with read-only PC PCM captures.
set $dumped=0
set $audio=(struct SlicksAmigaAudio *)0
tbreak *start_race_engines
commands
  silent
  set $audio=*(struct SlicksAmigaAudio **)($sp+4)
  continue
end
tbreak slicks_amiga_audio_play_effect
commands
  silent
  if !$audio || $audio->samples[17].bytes!=2050 || $audio->samples[20].bytes!=6346 || $audio->engine_vehicles[0]!=0 || $audio->engine_vehicles[1]!=0 || $audio->engine_vehicles[2]!=1 || $audio->engine_vehicles[3]!=6
    printf "NATIVE_ENGINE_PCM_SETUP_FAILED\n"
    quit 1
  end
  dump binary memory .run/audio-native-pcm-17.bin $audio->samples[17].data $audio->samples[17].data+$audio->samples[17].bytes
  dump binary memory .run/audio-native-pcm-20.bin $audio->samples[20].data $audio->samples[20].data+$audio->samples[20].bytes
  printf "NATIVE_ENGINE_PCM_DUMPED vehicles=0,0,1,6 lengths=2050,6346\n"
  set $dumped=1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$dumped || g_slicks_diag_race_error || g_slicks_diag_restore_status!=31 || g_slicks_audio_vbi_spills
    printf "NATIVE_ENGINE_PCM_RESTORE_FAILED\n"
    quit 1
  end
  printf "NATIVE_ENGINE_PCM_RESTORED restore=31 spills=0\n"
  quit
end
continue
quit 1
