set $edited = 0
set $saved = 0
break slicks_diag_options_ready
commands
  silent
  if g_slicks_options_configuration->options[1] == 25 && g_slicks_options_configuration->options[2] == 20
    set $edited = 1
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if !$edited || g_slicks_setup_save_report.result || g_slicks_options_configuration->options[1] != 25 || g_slicks_options_configuration->options[2] != 20
    printf "VOLUME_NATIVE_SAVE_FAILED\n"
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
  printf "VOLUME_NATIVE_MENU_SAVE_RESTORE_OK sounds=25 background=20\n"
  quit
end
continue
