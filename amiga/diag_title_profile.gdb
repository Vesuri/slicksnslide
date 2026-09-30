# TITLEPROFILE=1 build, REGCHECKU. Read only after normal cleanup.
break slicks_diag_system_restored
commands
  silent
  if g_slicks_diag_restore_status!=31 || g_slicks_title_timing_count!=65 || g_slicks_title_dirty_checks
    quit 1
  end
  set $i=0
  while $i<65
    set $p=&g_slicks_title_profile[$i]
    if $p->publications>1
      quit 1
    end
    set $j=1
    while $j<6
      if $p->marks[$j]<$p->marks[$j-1]
        printf "TITLE_PROFILE_TIME_INVALID sample=%u mark=%u\n",$i,$j
        quit 1
      end
      set $j=$j+1
    end
    if $i>0
      if g_slicks_title_timing_counters[$i]!=((g_slicks_title_timing_counters[$i-1]+4)&255)
        quit 1
      end
    end
    printf "TITLE_PROFILE sample=%u start=%lu draw=%lu unpack=%lu wait=%lu c2p=%lu tail=%lu pixels=%lu publications=%u\n",$i,$p->marks[0],$p->marks[1]-$p->marks[0],$p->marks[2]-$p->marks[1],$p->marks[3]-$p->marks[2],$p->marks[4]-$p->marks[3],$p->marks[5]-$p->marks[4],$p->pixels,$p->publications
    set $i=$i+1
  end
  printf "TITLE_PROFILE_OK samples=65 restore=31\n"
  quit
end
continue
quit 1
