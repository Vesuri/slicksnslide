# REGCHECKA: observe more than one complete 64-update pulse without input.
set $ticks = 0
set $changes = 0
break *slicks_tick_title_colours
commands
  silent
  set $counter = *(unsigned char *)&slicks_title_counter
  set $colour = *(unsigned short *)&slicks_title_selected_color
  if $ticks
    if $counter != (($previous+4)&255)
      printf "TITLE_ANIMATION_COUNTER_FAILED\n"
      quit 1
    end
    if $colour != $lastcolour
      set $changes = $changes+1
    end
  end
  set $previous = $counter
  set $lastcolour = $colour
  set $ticks = $ticks+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "TITLE_ANIMATION ticks=%u changes=%u checks=%lu errors=%lu full=%lu restore=%u\n",$ticks,$changes,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors,g_slicks_title_full_publications,g_slicks_diag_restore_status
  if $ticks < 65 || $changes < 2 || g_slicks_title_dirty_checks < 2 || g_slicks_title_dirty_errors || g_slicks_title_full_publications != 1 || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "TITLE_ANIMATION_OK\n"
  quit
end
continue
quit 1
