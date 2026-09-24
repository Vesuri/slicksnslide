break slicks_diag_bitmap_audit_failed
commands
  silent
  if g_slicks_diag_audit_frame != 1 || g_slicks_diag_audit_x != 0 || g_slicks_diag_audit_y != 100 || g_slicks_diag_audit_plane != 0
    printf "SLICKS_BITMAP_FAULT_WRONG_LOCATION\n"
    quit 1
  end
  printf "SLICKS_BITMAP_FAULT_DETECTED FRAME=1 X=0 Y=100 PLANE=0\n"
  quit
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "SLICKS_BITMAP_FAULT_MISSED\n"
  quit 1
end
continue
