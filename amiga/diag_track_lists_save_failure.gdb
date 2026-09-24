set $warning = 0
break slicks_amiga_store_track_lists
commands
  silent
  set $process = (struct Process *)SysBase->ThisTask
  set $window = $process->pr_WindowPtr
  continue
end
break slicks_diag_track_lists_ready
commands
  silent
  set $m = g_slicks_track_menu
  if !$m || $m->error || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $m->message
    if $m->track_lists || $m->picker || $m->name_dialog || g_slicks_track_lists_save.result != 1 || !g_slicks_track_lists_save.io_error || $process->pr_WindowPtr != $window
      printf "TRACK_LIST_SAVE_WARNING_FAILED\n"
      quit 1
    end
    set $warning = $warning+1
    printf "TRACK_LIST_SAVE_WARNING io_error=%ld path=%s\n", g_slicks_track_lists_save.io_error, g_slicks_track_lists_save.path
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if $warning != 1 || g_slicks_diag_race_error || g_slicks_track_menu || g_slicks_track_playlist.count != 2 || g_slicks_track_playlist.tracks[0] != 0 || g_slicks_track_playlist.tracks[1] != 1
      printf "TRACK_LIST_SAVE_WARNING_RETURN_FAILED\n"
      quit 1
    end
    printf "TRACK_LIST_NATIVE_READ_ONLY_SAVE_WARNING_DISMISS_RACE_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TRACK_LIST_SAVE_WARNING_EARLY_EXIT\n"
  quit 1
end
continue
