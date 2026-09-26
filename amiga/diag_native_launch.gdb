# Interactive launch only. Run once with default debug snapshots, then
# SLICKS_DEBUG_SNAPSHOTS=0; expected target checkpoints are 200 and 0.
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ready
    if g_slicks_diag_ingame || g_slicks_diag_race_error || !g_slicks_diag_audio_ready || g_slicks_diag_profile_all
      printf "NATIVE_LAUNCH_INVALID ingame=%u error=%u audio=%u profile=%u\n",g_slicks_diag_ingame,g_slicks_diag_race_error,g_slicks_diag_audio_ready,g_slicks_diag_profile_all
      quit 1
    end
    printf "NATIVE_LAUNCH_OK checkpoint=%lu audio=%u profiles=%u\n",g_slicks_diag_target_frame,g_slicks_diag_audio_ready,g_slicks_profiles.count
    quit
  end
  continue
end
continue
quit 1
