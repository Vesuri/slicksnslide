set $cues = 0
set $audio = 0
set $start_vbi = 0
set $high_started = 0
break *slicks_amiga_audio_start_race
commands
  silent
  set $audio = *(struct SlicksAmigaAudio **)($sp+4)
  set $platform = (*(struct StartPresentation **)($sp+20))->platform
  continue
end
break *'play_effect_period.part.0'
commands
  silent
  if !$audio || *(unsigned long *)($sp+8) != 1
    continue
  end
  set $priority = *(unsigned long *)($sp+16)
  set $period = *(unsigned long *)($sp+20)
  if $cues == 0
    set $start_vbi = $platform->vblank_count
  end
  set $elapsed = $platform->vblank_count - $start_vbi
  if ($cues == 0 && ($elapsed != 0 || $priority != 70 || $period != 0)) || ($cues == 1 && ($elapsed != 20 || $priority != 70 || $period != 0)) || ($cues == 2 && ($elapsed != 45 || $priority != 80 || $period != 0)) || ($cues == 3 && ($elapsed != 45 || $priority != 70 || $period != 272)) || $cues > 3
    printf "AUDIO_START_CUE_FAILED cue=%u VBI=%u priority=%u period=%u\n", $cues, $elapsed, $priority, $period
    quit 1
  end
  set $cues = $cues + 1
  continue
end
break *slicks_amiga_audio_vblank
commands
  silent
  if $audio && $cues == 4
    set $c = 0
    while $c < 4
      if $audio->pending_sample[$c] == &$audio->samples[1] && $audio->pending_period[$c] == 272 && $audio->pending_start[$c] == 0 && $audio->effect_ticks[$c] > 0
        set $high_started = 1
      end
      set $c = $c + 1
    end
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $audio && g_slicks_diag_ingame && g_slicks_diag_race_frame >= 5
    if $cues != 4 || !$high_started || !$audio->engine_started || g_slicks_diag_race_error
      printf "AUDIO_START_RACE_FAILED\n"
      quit 1
    end
    printf "AUDIO_START_RACE_OK cues=4 VBI=0/20/45/45 final_period=272 engines=started\n"
    quit
  end
  continue
end
continue
printf "AUDIO_START_UNEXPECTED_STOP\n"
quit 1
