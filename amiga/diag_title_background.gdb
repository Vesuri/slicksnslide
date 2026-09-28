set $title_background_seen = 0
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ready && !g_slicks_diag_ingame && !$title_background_seen
    dump binary memory ../tmp/fidelity-title.vga g_slicks_diag_logical g_slicks_diag_logical+262144
    set $title_background_seen = 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$title_background_seen || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "TITLE_BACKGROUND_CAPTURE_RESTORE_OK\n"
  quit
end
continue
