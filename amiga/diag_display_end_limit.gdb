# Launch NATURALQB: real racing, no simulation-state injection. Publication
# is adaptive: on time at a fresh display-end edge, or at once when the
# update overran that edge. On-time publications must start in rows 256..270
# and never share a VBlank; late publications are counted.
set $updates=0
set $late=0
set $previous=0
set $first=0
set $armed=0
set $minimum=312
set $maximum=0
break slicks_amiga_platform_wait_publication
commands
  silent
  set $armed=1
  continue
end
break update_race_engines
commands
  silent
  if !$armed
    continue
  end
  set $armed=0
  set $now=g_slicks_diag_profile_platform->vblank_count
  if late_publication
    set $late=$late+1
    set $updates=$updates+1
    continue
  end
  set $line=((*(unsigned short *)0xdff004 & 7)<<8) | (*(unsigned short *)0xdff006 >> 8)
  if $line < 256 || $line > 270
    printf "DISPLAY_END_PHASE_FAILED line=%u\n",$line
    quit 1
  end
  if $line < $minimum
    set $minimum=$line
  end
  if $line > $maximum
    set $maximum=$line
  end
  if $updates && $now == $previous
    printf "DISPLAY_END_LIMIT_FAILED duplicate publication at VBlank %lu\n",$now
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
    printf "DISPLAY_END_LIMIT_OK publications=%lu late=%lu elapsed_vblanks=%lu raster_range=%u..%u\n",$updates,$late,$previous-$first,$minimum,$maximum
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "DISPLAY_END_LIMIT_EARLY_EXIT\n"
  quit 1
end
continue
