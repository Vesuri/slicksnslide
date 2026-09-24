break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    printf "SLICKS_BITMAP_AUDIT_ENABLED=%u FRAME=%u\n", g_slicks_diag_audit_bitmap, g_slicks_diag_race_frame
    if !g_slicks_diag_audit_bitmap
      quit 1
    end
    disable 1
  end
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "SLICKS_BITMAP_AUDIT_FAILED FRAME=%u X=%u Y=%u PLANE=%u OFFSET=%u BEFORE=%02x AFTER=%02x\n", g_slicks_diag_audit_frame, g_slicks_diag_audit_x, g_slicks_diag_audit_y, g_slicks_diag_audit_plane, g_slicks_diag_audit_offset, g_slicks_diag_audit_before, g_slicks_diag_audit_after
  quit 1
end
break slicks_diag_gameplay_ready
commands
  silent
  if !g_slicks_diag_audit_bitmap
    printf "SLICKS_BITMAP_AUDIT_NOT_ENABLED\n"
    quit 1
  end
  printf "SLICKS_BITMAP_AUDIT_PASSED FRAME=%u\n", g_slicks_diag_race_frame
  quit
end
continue
