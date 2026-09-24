set $edited = 0
set $saved = 0
break slicks_diag_options_closed
commands
  silent
  set $c = g_slicks_options_configuration
  if $c->options[0] != 5 || $c->options[13] != 5 || $c->options[14] != 2
    printf "ARCADE_SAVE_MENU_EDIT_FAILED\n"
    quit 1
  end
  dump binary memory .run/arcade-persistence-v1/saved-options.bin &$c->options[0] &$c->options[15]
  set $edited = 1
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if !$edited || g_slicks_setup_save_report.result
    printf "ARCADE_NATIVE_SAVE_FAILED\n"
    quit 1
  end
  set $saved = 1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "ARCADE_NATIVE_MENU_SAVE_RESTORE_OK mode=5 seconds=5 tracks=2\n"
  quit
end
continue
