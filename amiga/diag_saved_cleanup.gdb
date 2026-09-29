# CHAMPSAVB with a private pre-existing E2E.SSS.
source diag_menu_rectangles.gdb
set $notices=0
set $returns=0
break championship_notice
commands
  silent
  printf "CLEANUP_NOTICE %s\n",text
  if $notices==0
    if text[0]!=79
      quit 1
    end
  else
    if $notices!=1 || text[0]!=71 || text[11]!=45 || text[13]!=66 || g_slicks_diag_backup_protect || saved_files_cache.count!=1
      quit 1
    end
  end
  set $notices=$notices+1
  continue
end
break slicks_diag_saved_closed
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $returns=$returns+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $notices!=2 || $returns!=1 || g_slicks_diag_force_exit || g_slicks_diag_restore_status!=31
    quit 1
  end
  printf "SAVED_BACKUP_CLEANUP_RETURN_EXIT_OK\n"
  quit
end
continue
