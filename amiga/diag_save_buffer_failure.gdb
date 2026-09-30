# CHAMPSAVM: reject the first reserved playlist-name workspace, then real retry.
# Start with a fresh Classic-mode CFG and no saved championships.
set $warnings = 0
set $saved = 0
set $pickers = 0
set $names = 0
set $returns = 0
source diag_menu_rectangles.gdb
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_diag_save_buffer_fault == 2
    printf "SAVE_BUFFER_WARNING_RELEASED_DISPLAY\n"
    quit 1
  end
  continue
end
break *slicks_resource_archive_open
commands
  silent
  if g_slicks_diag_save_buffer_fault == 2
    printf "SAVE_BUFFER_WARNING_REOPENED_ARCHIVE\n"
    quit 1
  end
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  if g_slicks_diag_saved_phase == 1
    set $pickers = $pickers+1
  end
  if g_slicks_diag_saved_phase == 2
    set $names = $names+1
  end
  if g_slicks_diag_saved_phase == 3
    if g_slicks_diag_save_buffer_fault == 2
      set $warnings = $warnings+1
      if $pickers || $names || saved_files_cache.count
        quit 1
      end
    else
      if g_slicks_diag_save_buffer_fault != 3 || saved_files_cache.count != 1
        quit 1
      end
      set $saved = $saved+1
    end
  end
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $returns = $returns+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $warnings != 1 || $saved != 1 || $pickers != 2 || $names != 1 || $returns != 2 || g_slicks_diag_save_buffer_fault != 3 || g_slicks_diag_restore_status != 31 || g_slicks_diag_force_exit
    printf "SAVE_BUFFER_FAILURE_PIXEL_GATE_FAILED\n"
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK\n"
  quit
end
continue
