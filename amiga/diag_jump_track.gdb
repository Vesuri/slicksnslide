# SLICKS_JUMP_TRACK=1: actual BUMPS assets, start grid and AI, no state injection.
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_JUMP_LOAD_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
    quit 1
  end
  if g_slicks_diag_ingame
    disable 1
  end
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "SLICKS_JUMP_TRACK FRAME=%u ZONES=%u TAKEOFFS=%u LANDINGS=%u SHADOW_FRAMES=%u PEAK=%u SOUNDS=%u\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones, g_slicks_diag_jump_takeoffs, g_slicks_diag_jump_landings, g_slicks_diag_jump_shadow_frames, g_slicks_diag_jump_peak, g_slicks_diag_sound_event_totals[7]
  printf "SLICKS_CONTACT_SOUNDS WALL=%u CAR=%u\n", g_slicks_diag_sound_event_totals[5], g_slicks_diag_sound_event_totals[6]
  set $sample = 10
  while $sample <= 16
    if g_slicks_diag_sound_event_totals[$sample]
      printf "SLICKS_UNEXPECTED_WEAPON_SOUND=%u\n", $sample
      quit 1
    end
    set $sample = $sample + 1
  end
  if g_slicks_diag_race_frame != 700 || g_slicks_diag_track_zones != 18 || !g_slicks_diag_jump_takeoffs || !g_slicks_diag_jump_landings || !g_slicks_diag_jump_shadow_frames || g_slicks_diag_jump_peak <= 500 || g_slicks_diag_sound_event_totals[7] != g_slicks_diag_jump_takeoffs
    quit 1
  end
  printf "SLICKS_JUMP_TRACK_OK\n"
  quit
end
break slicks_diag_collision_failed
commands
  silent
  printf "SLICKS_JUMP_COLLISION_ERROR=%u FRAME=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_frame
  quit 1
end
continue
