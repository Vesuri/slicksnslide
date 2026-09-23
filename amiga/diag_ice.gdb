break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_ICE_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
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
  set $animated = g_slicks_diag_material_count[22] + g_slicks_diag_material_count[23]
  set $animated = $animated + g_slicks_diag_material_count[24]
  set $animated = $animated + g_slicks_diag_material_count[25] + g_slicks_diag_material_count[26]
  set $ok = g_slicks_diag_race_frame == 200 && g_slicks_diag_track_zones > 0
  set $ok = $ok && g_slicks_diag_skidmarks == 1040
  set $ok = $ok && g_slicks_diag_track_collisions == 0
  set $ok = $ok && g_slicks_diag_checksum == 0xed49462a
  set $ok = $ok && g_slicks_diag_display_checksum == 0xb7c815dc
  if !$ok
    printf "SLICKS_ICE_FAILED FRAME=%u ZONES=%u\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones
    quit 1
  end
  printf "SLICKS_ICE_OK FRAME=%u ZONES=%u ANIMATED=%u COUNTS=%u,%u,%u,%u,%u SKIDS=%u TRACKCOLL=%u CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones, $animated, g_slicks_diag_material_count[22], g_slicks_diag_material_count[23], g_slicks_diag_material_count[24], g_slicks_diag_material_count[25], g_slicks_diag_material_count[26], g_slicks_diag_skidmarks, g_slicks_diag_track_collisions, g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
