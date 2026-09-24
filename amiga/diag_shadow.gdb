# Run with SLICKS_SHADOW_TEST=1. Program-side injection avoids relying on
# debugger memory writes. This fixture is not a natural-jump race comparison.
break slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_shadow_check != 2 || g_slicks_diag_race_frame != 5 || !g_slicks_diag_audit_bitmap
    printf "SLICKS_SHADOW_FAILED CHECK=%u FRAME=%u AUDIT=%u\n", g_slicks_diag_shadow_check, g_slicks_diag_race_frame, g_slicks_diag_audit_bitmap
    quit 1
  end
  printf "SLICKS_SHADOW_OK FRAME=%u CHECK=%u AUDIT=%u\n", g_slicks_diag_race_frame, g_slicks_diag_shadow_check, g_slicks_diag_audit_bitmap
  quit
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "SLICKS_SHADOW_BITMAP_AUDIT_FAILED FRAME=%u\n", g_slicks_diag_audit_frame
  quit 1
end
continue
