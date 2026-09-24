set $loaded = 0
set $audio = 0
break prepare_race
commands
  silent
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present || configuration->options[1] != 25 || configuration->options[2] != 20
    printf "VOLUME_FRESH_RELOAD_FAILED\n"
    quit 1
  end
  set $loaded = 1
  continue
end
break *slicks_amiga_audio_start_engines
commands
  silent
  set $audio = *(struct SlicksAmigaAudio **)($sp+4)
  if !$loaded || $audio->sound_volume != 16 || $audio->music_volume != 3
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $audio
    if !$audio->engine_started || $audio->volume_dirty || $audio->sound_volume != 16 || $audio->music_volume != 3 || g_slicks_diag_race_error
      quit 1
    end
    printf "VOLUME_FRESH_RELOAD_RACE_OK sounds=25 background=20 Paula=16/3\n"
    quit
  end
  continue
end
continue
