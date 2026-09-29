set $loads = 0
set $steps = 1
set $intermission = 0
source diag_saved_resident.gdb
break prepare_race
commands
  silent
  if !new_game
    quit 1
  end
  set $loads = $loads+1
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  if g_slicks_diag_intermission_menu->intermission->content.track_index != 0 || g_slicks_track_playlist.count != 3
    quit 1
  end
  set $intermission = 1
  printf "REAL_FIRST_INTERMISSION cash=%d\n",g_slicks_setup_session.cash[0]
  continue
end
break slicks_diag_saved_ready
commands
  silent
  printf "CHAMPIONSHIP_EDIT step=%u phase=%u\n",championship_dialog_step,g_slicks_diag_saved_phase
  if championship_dialog_step != $steps
    quit 1
  end
  set $expected_scans = 1
  set $expected_names = 1
  if $steps >= 5
    set $expected_scans = 2
    set $expected_names = 2
  end
  if $steps >= 10
    set $expected_scans = 3
  end
  if $steps == 15
    set $expected_scans = 4
    set $expected_names = 1
  end
  if $saved_enumerations != $expected_scans || saved_files_cache.count != $expected_names
    printf "CHAMPIONSHIP_EDIT_CACHE_FAILED scans=%u names=%d\n",$saved_enumerations,saved_files_cache.count
    quit 1
  end
  set $steps = $steps+1
  continue
end
break championship_notice
commands
  silent
  printf "CHAMPIONSHIP_NOTICE %s\n",text
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $loads != 1 || !$intermission || $steps != 16 || g_slicks_diag_force_exit || g_slicks_diag_restore_status != 0x1f
    printf "CHAMPIONSHIP_EDIT_FAILED steps=%u\n",$steps
    quit 1
  end
  printf "NATIVE_CHAMPIONSHIP_RESAVE_OVERWRITE_DELETE_CANCEL_OK\n"
  quit
end
continue
