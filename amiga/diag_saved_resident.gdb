# Shared by saved-game lifecycle fixtures. Closures must retain the display;
# filesystem functions must run after the owner's explicit release.
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
  if g_slicks_diag_profile_platform->active
    printf "SAVED_ENUMERATION_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_amiga_saved_file_exists
commands
  silent
  if g_slicks_diag_profile_platform->active
    printf "SAVED_EXISTS_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_amiga_saved_file_delete
commands
  silent
  set $saved_deletes = $saved_deletes+1
  if g_slicks_diag_profile_platform->active
    printf "SAVED_DELETE_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_amiga_store_saved_game
commands
  silent
  set $saved_process = (struct Process *)SysBase->ThisTask
  set $saved_window = $saved_process->pr_WindowPtr
  if g_slicks_diag_profile_platform->active
    printf "SAVED_STORE_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
break slicks_amiga_load_saved_game
commands
  silent
  if g_slicks_diag_profile_platform->active
    printf "SAVED_FILE_IO_WITH_HARDWARE_OWNED\n"
    quit 1
  end
  continue
end
