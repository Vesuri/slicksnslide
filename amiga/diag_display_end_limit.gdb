# Launch NATURALQB: real racing, no simulation-state injection. Publication
# is adaptive: on time at a fresh display-end edge, or at once when the
# update overran that edge. On-time publications must start within 15 rows
# of the display end ($100 PAL, $f4 NTSC) and never share a display frame;
# late publications are counted.
set $updates=0
set $late=0
set $previous=0
set $first=0
set $armed=0
set $minimum=999
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
  set $now=g_slicks_diag_profile_platform->frame_count
  if late_publication
    set $late=$late+1
    set $updates=$updates+1
    continue
  end
  set $line=((*(unsigned short *)0xdff004 & 7)<<8) | (*(unsigned short *)0xdff006 >> 8)
  if $line < slicks_video_timing.display_end || $line > slicks_video_timing.display_end+14
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
    printf "DISPLAY_END_LIMIT_FAILED duplicate publication at frame %lu\n",$now
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
    printf "DISPLAY_END_LIMIT_OK publications=%lu late=%lu elapsed_frames=%lu raster_range=%u..%u frame_lines=%u\n",$updates,$late,$previous-$first,$minimum,$maximum,slicks_video_timing.frame_lines
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
