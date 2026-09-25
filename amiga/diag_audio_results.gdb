# SLICKS_NATURAL_RESULTS=fuel: actual computer race and results transition.
set $audio=(struct SlicksAmigaAudio *)0
set $results=0
set $effects=0
set $music=0
break *start_race_engines
commands
  silent
  set $audio=*(struct SlicksAmigaAudio **)($sp+4)
  continue
end
break slicks_amiga_audio_play_effect
commands
  silent
  set $effects=$effects+1
  continue
end
break slicks_amiga_audio_start_music
commands
  silent
  set $music=$music+1
  continue
end
break slicks_diag_results_ready
commands
  silent
  if !$audio || $music!=1 || $audio->engine_started || $audio->channels.engine_mask
    printf "AUDIO_RESULTS_ENGINES_NOT_STOPPED\n"
    quit 1
  end
  set $i=0
  while $i<4
    if $audio->channels.owner[$i]==1 || $audio->channels.owner[$i]==2
      printf "AUDIO_RESULTS_STALE_RACE_CHANNEL channel=%u owner=%u\n",$i,$audio->channels.owner[$i]
      quit 1
    end
    set $i=$i+1
  end
  set $results=$results+1
  printf "AUDIO_RESULTS_READY effects=%u\n",$effects
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$results || !$effects || g_slicks_diag_restore_status!=31 || g_slicks_diag_race_error || g_slicks_audio_vbi_spills
    printf "AUDIO_RESULTS_FAILED results=%u effects=%u restore=%u error=%u spills=%lu\n",$results,$effects,g_slicks_diag_restore_status,g_slicks_diag_race_error,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "AUDIO_NATIVE_RESULTS_OK restore=31 spills=0\n"
  quit
end
continue
printf "AUDIO_RESULTS_UNEXPECTED_STOP\n"
quit 1
