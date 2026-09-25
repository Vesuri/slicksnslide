# Use SLICKS_TRACK_ACTOR_TEST=1 for the native, bounded BASIC race.
# First race effect marks active racing without per-update debugger stops.
# Take listening excerpts AFTER this one-off marker, once uninterrupted.
set $race=(struct SlicksRaceRuntime *)0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
tbreak slicks_amiga_audio_play_effect
commands
  silent
  printf "AUDIO_CAPTURE_FIRST_EFFECT frame=%u racing=%u speeds=%ld,%ld,%ld,%ld\n",$race->frame_count,$race->racing,$race->cars[0].measured_speed,$race->cars[1].measured_speed,$race->cars[2].measured_speed,$race->cars[3].measured_speed
  shell wc -c "$SLICKS_AUDIO_CAPTURE_FILE"
  continue
end
break slicks_diag_results_ready
commands
  silent
  printf "AUDIO_CAPTURE_RESULTS spills=%lu\n",g_slicks_audio_vbi_spills
  continue
end
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_race_error || g_slicks_diag_restore_status!=31 || g_slicks_audio_vbi_spills
    printf "AUDIO_CAPTURE_FAILED error=%u restore=%u spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_restore_status,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "AUDIO_CAPTURE_RESTORED\n"
  quit
end
continue
printf "AUDIO_CAPTURE_UNEXPECTED_STOP\n"
bt
quit 1
