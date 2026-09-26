# SLICKS_BITMAP_AUDIT=3: a native fixture intentionally omits one pixel.
break slicks_diag_bitmap_audit_failed
commands
  silent
  if g_slicks_diag_audit_bitmap!=3 || g_slicks_diag_audit_frame!=1 || g_slicks_diag_audit_x!=0 || g_slicks_diag_audit_y!=100 || g_slicks_diag_audit_plane!=0
    printf "MISSING_PIXEL_CONTROL_WRONG_FAILURE\n"
    quit 1
  end
  printf "MISSING_PIXEL_CONTROL_DETECTED frame=1 x=0 y=100 plane=0\n"
  quit
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "MISSING_PIXEL_CONTROL_NOT_DETECTED\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  printf "MISSING_PIXEL_CONTROL_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
