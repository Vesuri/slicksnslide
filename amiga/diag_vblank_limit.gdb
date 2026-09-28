# Launch NATURALQB: real racing, no simulation-state injection. Check each
# update, including the countdown, on both normal and accelerated CPUs.
set $updates=0
set $previous=0
set $first=0
break *slicks_race_step
commands
  silent
  set $now=g_slicks_diag_profile_platform->vblank_count
  if $updates && $now == $previous
    printf "VBLANK_LIMIT_FAILED duplicate update at VBlank %lu\n",$now
    quit 1
  end
  if !$updates
    set $first=$now
  end
  set $previous=$now
  set $updates=$updates+1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $updates >= 120
    if g_slicks_diag_race_error
      quit 1
    end
    printf "VBLANK_LIMIT_OK updates=%lu elapsed_vblanks=%lu\n",$updates,$previous-$first
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "VBLANK_LIMIT_EARLY_EXIT\n"
  quit 1
end
continue
