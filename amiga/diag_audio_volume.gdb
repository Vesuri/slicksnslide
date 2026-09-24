set $edited = 0
set $audio = 0
break slicks_diag_options_ready
commands
  silent
  if g_slicks_options_configuration->options[1] == 25 && g_slicks_options_configuration->options[2] == 20
    set $edited = 1
  end
  continue
end
break *slicks_amiga_audio_start_engines
commands
  silent
  set $audio = *(struct SlicksAmigaAudio **)($sp+4)
  if !$edited || $audio->sound_volume != 16 || $audio->music_volume != 3
    printf "AUDIO_MENU_VOLUME_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $audio
    if !$audio->engine_started || $audio->volume_dirty || $audio->sound_volume != 16 || $audio->music_volume != 3
      printf "AUDIO_RACE_VOLUME_FAILED\n"
      quit 1
    end
    printf "AUDIO_NATIVE_MENU_TO_RACE_VOLUME_OK sounds=25 background=20 Paula=16/3\n"
    quit
  end
  continue
end
continue
