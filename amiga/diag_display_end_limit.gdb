# Launch NATURALQB: real racing, no simulation-state injection. Publication
# is paced; simulation starts need not be separated by a VBlank interrupt.
set $updates=0
set $previous=0
set $first=0
set $armed=0
set $minimum=312
set $maximum=0
break slicks_amiga_platform_wait_display_end
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
    printf "DISPLAY_END_LIMIT_OK publications=%lu elapsed_vblanks=%lu raster_range=%u..%u\n",$updates,$previous-$first,$minimum,$maximum
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
