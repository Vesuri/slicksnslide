# Run with SLICKS_BITMAP_AUDIT=1 and FSUAE_RUN=.run/track-hud.
# These dumps are output from the native port, never source assets.
set $font_calls = 0
break slicks_race_set_status_palette
commands
  silent
  set $hud_palette = *(unsigned char **)($sp+8)
  set $hud_race = *(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break sui_font_string
commands
  silent
  set $font_calls = $font_calls + 1
  set $hud_surface = (unsigned char *)$a0
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_HUD_LOAD_FAILED ERROR=%u\n",g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame
    if $font_calls < 10 || !g_slicks_diag_audit_bitmap || !$hud_race->hud_track_ready || $hud_race->hud_record_time != 822
      printf "SLICKS_HUD_START_FAILED CALLS=%u AUDIT=%u\n",$font_calls,g_slicks_diag_audit_bitmap
      quit 1
    end
    dump binary memory .run/track-hud/start.chunky $hud_surface $hud_surface+64000
    dump binary memory .run/track-hud/palette.bin $hud_palette $hud_palette+768
    printf "SLICKS_HUD_START CALLS=%u TRACK=%s RECORD=%u\n",$font_calls,$hud_race->hud_track_name,$hud_race->hud_record_time
    disable 3
  end
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "SLICKS_HUD_BITMAP_FAILED FRAME=%u X=%u Y=%u\n",g_slicks_diag_audit_frame,g_slicks_diag_audit_x,g_slicks_diag_audit_y
  quit 1
end
break slicks_diag_gameplay_ready
commands
  silent
  dump binary memory .run/track-hud/frame700.chunky $hud_surface $hud_surface+64000
  if g_slicks_diag_race_error || g_slicks_diag_race_frame != 700
    printf "SLICKS_HUD_TARGET_FAILED FRAME=%u FONT_CALLS=%u ERROR=%u\n",g_slicks_diag_race_frame,$font_calls,g_slicks_diag_race_error
    quit 1
  end
  printf "SLICKS_HUD_TARGET_OK FRAME=%u FONT_CALLS=%u ERROR=%u\n",g_slicks_diag_race_frame,$font_calls,g_slicks_diag_race_error
  quit
end
continue
