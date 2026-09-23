break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SLICKS_ZONE_ERROR=%u STAGE=%u\n", g_slicks_diag_race_error, g_slicks_diag_race_stage
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
  set $ok = g_slicks_diag_race_frame == 200 && g_slicks_diag_track_zones == 46
  set $ok = $ok && g_slicks_diag_skidmarks == 1112
  set $ok = $ok && g_slicks_diag_track_collisions == 0
  set $ok = $ok && g_slicks_diag_checksum == 0xcc686167
  set $ok = $ok && g_slicks_diag_display_checksum == 0x823431ce
  if !$ok
    printf "SLICKS_ZONE_FAILED FRAME=%u ZONES=%u\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones
    quit 1
  end
  printf "SLICKS_ZONE_OK FRAME=%u ZONES=%u SKIDS=%u TRACKCOLL=%u CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_race_frame, g_slicks_diag_track_zones, g_slicks_diag_skidmarks, g_slicks_diag_track_collisions, g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
