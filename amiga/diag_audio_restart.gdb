# SLICKS_OPTIONS_MENU=12: native pause/skip, next race, pause/end game.
set $audio=(struct SlicksAmigaAudio *)0
set $starts=0
set $pauses=0
set $intermissions=0
break *start_race_engines
commands
  silent
  set $audio=*(struct SlicksAmigaAudio **)($sp+4)
  if $audio->engine_started || $audio->music_started || $audio->channels.engine_mask
    printf "AUDIO_RESTART_STALE_OWNERS\n"
    quit 1
  end
  set $i=0
  while $i<4
    if $audio->pending_start[$i] || $audio->effect_ticks[$i] || $audio->channels.owner[$i]
      quit 1
    end
    set $i=$i+1
  end
  set $starts=$starts+1
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  if !$audio || $audio->engine_started || $audio->channels.engine_mask || $audio->music_started
    quit 1
  end
  set $pauses=$pauses+1
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  if $audio->engine_started || $audio->music_started || $audio->channels.engine_mask
    quit 1
  end
  set $intermissions=$intermissions+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $starts!=2 || $pauses!=2 || $intermissions!=1 || g_slicks_diag_restore_status!=31 || g_slicks_diag_race_error || g_slicks_audio_vbi_spills
    printf "AUDIO_RESTART_FAILED starts=%u pauses=%u intermissions=%u restore=%u error=%u spills=%lu\n",$starts,$pauses,$intermissions,g_slicks_diag_restore_status,g_slicks_diag_race_error,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "AUDIO_NATIVE_SKIP_NEXT_RACE_END_OK starts=2 pauses=2 restore=31 spills=0\n"
  quit
end
continue
quit 1
