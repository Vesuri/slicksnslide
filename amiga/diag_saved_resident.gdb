# Shared by saved-game lifecycle fixtures. Widgets retain takeover; filesystem
# calls inside the dialog require an OS-service window with the display held.
define check_saved_io
  if g_slicks_diag_saved_menu
    if !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
      printf "SAVED_IO_WITHOUT_RETAINED_DISPLAY\n"
      quit 1
    end
  else
    if g_slicks_diag_profile_platform->active
      printf "STARTUP_SAVED_IO_WITH_HARDWARE_OWNED\n"
      quit 1
    end
  end
end
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_diag_saved_menu
    printf "SAVED_DIALOG_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
set $saved_enumerations = 0
set $saved_deletes = 0
break slicks_amiga_profile_picker_close
commands
  silent
  if g_slicks_diag_saved_menu && !g_slicks_diag_profile_platform->active
    printf "SAVED_PICKER_CLOSE_WITH_DISPLAY_RELEASED\n"
    quit 1
  end
  continue
end
break slicks_amiga_saved_filename_open
commands
  silent
  if !g_slicks_diag_profile_platform->active
    printf "SAVED_NAME_OPEN_WITH_DISPLAY_RELEASED\n"
    quit 1
  end
  continue
end
break slicks_amiga_name_dialog_close
commands
  silent
  if g_slicks_diag_saved_menu && !g_slicks_diag_profile_platform->active
    printf "SAVED_NAME_CLOSE_WITH_DISPLAY_RELEASED\n"
    quit 1
  end
  continue
end
break slicks_amiga_message_close
commands
  silent
  if g_slicks_diag_saved_menu && !g_slicks_diag_profile_platform->active
    printf "SAVED_NOTICE_CLOSE_WITH_DISPLAY_RELEASED\n"
    quit 1
  end
  continue
end
break slicks_amiga_saved_files
commands
  silent
  set $saved_enumerations = $saved_enumerations+1
  check_saved_io
  continue
end
break slicks_amiga_saved_file_exists
commands
  silent
  check_saved_io
  continue
end
break slicks_amiga_saved_file_delete
commands
  silent
  set $saved_deletes = $saved_deletes+1
  check_saved_io
  continue
end
break slicks_amiga_store_saved_game
commands
  silent
  set $saved_process = (struct Process *)SysBase->ThisTask
  set $saved_window = $saved_process->pr_WindowPtr
  check_saved_io
  continue
end
break slicks_amiga_load_saved_game
commands
  silent
  check_saved_io
  continue
end
