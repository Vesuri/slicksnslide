break slicks_diag_frame_ready
commands
  silent
  printf "SLICKS_DIAG_READY=%u TRACKS=%u NAME=%s NEXT=%s CHECKSUM=%08x DISPLAY=%08x\n", g_slicks_diag_ready, g_slicks_diag_track_files, g_slicks_diag_first_track, g_slicks_diag_second_track, g_slicks_diag_checksum, g_slicks_diag_display_checksum
  quit
end
continue
