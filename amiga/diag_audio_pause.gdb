# SLICKS_PAUSE_LIVE=1: original native pause menu, child dialogs, resume.
set $starts=0
set $paused=0
set $resumed=0
set $audio=(struct SlicksAmigaAudio *)0
break *start_race_engines
commands
  silent
  set $audio=*(struct SlicksAmigaAudio **)($sp+4)
  set $starts=$starts+1
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  if !$audio || $audio->engine_started || $audio->music_started || $audio->channels.engine_mask
    printf "AUDIO_PAUSE_NOT_STOPPED\n"
    quit 1
  end
  set $i=0
  while $i<4
    if $audio->channels.owner[$i] || $audio->pending_start[$i] || $audio->effect_ticks[$i] || $audio->silent_reload_ticks[$i]
      printf "AUDIO_PAUSE_STALE_CHANNEL %u\n",$i
      quit 1
    end
    set $i=$i+1
  end
  set $paused=$paused+1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $paused && g_slicks_diag_ingame && g_slicks_diag_race_frame>=145
    if $starts!=2 || !$audio->engine_started || $audio->channels.engine_mask!=15 || $audio->music_started
      printf "AUDIO_RESUME_FAILED starts=%u mask=%u\n",$starts,$audio->channels.engine_mask
      quit 1
    end
    set $resumed=1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$resumed || $paused!=13 || g_slicks_diag_restore_status!=31 || g_slicks_diag_race_error || g_slicks_audio_vbi_spills
    printf "AUDIO_PAUSE_FAILED resumed=%u paused=%u restore=%u error=%u spills=%lu\n",$resumed,$paused,g_slicks_diag_restore_status,g_slicks_diag_race_error,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "AUDIO_NATIVE_PAUSE_RESUME_OK starts=%u paused=%u restore=31 spills=0\n",$starts,$paused
  quit
end
continue
printf "AUDIO_PAUSE_UNEXPECTED_STOP\n"
quit 1
