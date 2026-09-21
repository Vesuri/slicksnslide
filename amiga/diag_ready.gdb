break slicks_diag_frame_ready
commands
  silent
  printf "SLICKS_DIAG_READY=%u CHECKSUM=%08x\n", g_slicks_diag_ready, g_slicks_diag_checksum
  quit
end
continue
