# REGCHECKU: no in-loop breakpoints or full-frame pixel diagnostics.
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || g_slicks_title_timing_count!=65 || g_slicks_title_dirty_checks
    printf "TITLE_TIMING_INVALID\n"
    quit 1
  end
  set $i=1
  while $i<65
    if g_slicks_title_timing_counters[$i]!=((g_slicks_title_timing_counters[$i-1]+4)&255)
      printf "TITLE_TIMING_COUNTER_INVALID\n"
      quit 1
    end
    printf "TITLE_TIMING_INTERVAL %u %lu\n",$i,g_slicks_title_timing_vblanks[$i]-g_slicks_title_timing_vblanks[$i-1]
    set $i=$i+1
  end
  printf "TITLE_TIMING_OK updates=64 vblanks=%lu restore=31\n",g_slicks_title_timing_vblanks[64]-g_slicks_title_timing_vblanks[0]
  quit
end
continue
quit 1
