set $audio = 0
set $engines_seen = 0
set $effects_seen = 0
set $resumed = 0
set $previous_effects = 0
set $hold_test = 0
break *start_race_engines
commands
  silent
  set $audio = *(struct SlicksAmigaAudio **)($sp+4)
  set $race = *(struct SlicksRaceRuntime **)($sp+8)
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $audio
    if !$audio->ready || !$audio->engine_started || $audio->channels.engine_mask != 15 || g_slicks_diag_race_error || g_slicks_diag_audio_blank_spills || g_slicks_audio_vbi_spills
      printf "FOUR_ENGINE_AUDIO_STATE_FAILED ready=%u started=%u mask=%x error=%u spills=%u frame=%u\n",$audio->ready,$audio->engine_started,$audio->channels.engine_mask,g_slicks_diag_race_error,g_slicks_diag_audio_blank_spills,$race->frame_count
      printf "VBI spills=%u last_line=%u\n",g_slicks_audio_vbi_spills,g_slicks_audio_vbi_last_line
      quit 1
    end
    set $effects = 0
    set $engines = 0
    set $i = 0
    while $i < 4
      set $v = $race->cars[$i].vehicle
      set $frequency = (engine_frequency_base[$v]*100+engine_frequency_slope[$v]*$race->cars[$i].measured_speed)&65535
      if $audio->engine_vehicles[$i] != $v || $audio->engine_frequencies[$i] != $frequency || $audio->engine_periods[$i] != 3546895/$frequency
        printf "FOUR_ENGINE_AUDIO_PITCH_FAILED car=%u\n",$i
        quit 1
      end
      if $audio->channels.owner[$i] == 1
        set $engines = $engines | (1 << $i)
      end
      if $audio->channels.owner[$i] == 2
        set $effects = $effects | (1 << $i)
      end
      set $i = $i+1
    end
    set $engines_seen = $engines_seen | $engines
    set $effects_seen = $effects_seen | $effects
    set $resumed = $resumed | ($previous_effects & $engines)
    set $previous_effects = $effects
    if $hold_test == 1
      if $engines != 15 || $effects || g_slicks_diag_audio_hold_frames
        printf "AUDIO_STALLED_GAME_FAILED engines=%x effects=%x\n",$engines,$effects
        quit 1
      end
      printf "AUDIO_STALLED_GAME_OK engines resumed without game updates\n"
      set $hold_test = 2
    end
    if $race->frame_count == 40 && $hold_test == 0
      set g_slicks_diag_audio_hold_frames = 120
      set $hold_test = 1
    end
    if $race->frame_count >= 300
      if $engines_seen != 15 || !$effects_seen || !$resumed
        printf "FOUR_ENGINE_AUDIO_COVERAGE_FAILED engines=%x effects=%x resumed=%x\n",$engines_seen,$effects_seen,$resumed
        quit 1
      end
      printf "FOUR_ENGINE_AUDIO_OK frames=%u engines=%x borrowed=%x resumed=%x blank_spills=%u\n",$race->frame_count,$engines_seen,$effects_seen,$resumed,g_slicks_diag_audio_blank_spills
      printf "VBI spills=%u last_line=%u\n",g_slicks_audio_vbi_spills,g_slicks_audio_vbi_last_line
      quit
    end
  end
  continue
end
continue
