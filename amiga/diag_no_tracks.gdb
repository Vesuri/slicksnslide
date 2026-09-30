# STARTGO with no .SS files in either directory. Observe the real C return.
set $empty_discovery = 0
break *discover_tracks
commands
  silent
  set $discovery_return = *(unsigned long *)$sp
  tbreak *$discovery_return
  commands
    silent
    if ($d0 & 65535) != 0 || !track_files_in_current_directory
      printf "EMPTY_DISCOVERY_FABRICATED_TRACK\n"
      quit 1
    end
    set $empty_discovery = 1
    continue
  end
  continue
end
break *slicks_amiga_platform_begin
commands
  silent
  printf "EMPTY_DISCOVERY_TOOK_OVER_DISPLAY\n"
  quit 1
end
break *prepare_race
commands
  silent
  printf "EMPTY_DISCOVERY_REACHED_RACE\n"
  quit 1
end
break slicks_diag_frame_ready
commands
  silent
  printf "EMPTY_DISCOVERY_REACHED_TITLE\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  # No takeover snapshot exists on this early-error path. The OS remains
  # live and may rebuild its own View while servicing the console notice.
  if !$empty_discovery || !(g_slicks_diag_restore_status & 1)
    printf "EMPTY_DISCOVERY_RESTORE_FAILED zero=%u status=%u\n",$empty_discovery,g_slicks_diag_restore_status
    quit 1
  end
  printf "EMPTY_DISCOVERY_REJECT_WITHOUT_TAKEOVER_OK\n"
  quit
end
continue
