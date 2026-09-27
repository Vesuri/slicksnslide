# Read-only capture. Set $target before sourcing; interrupted timings are invalid.
set $race=(struct SlicksRaceRuntime *)0
set $capturing=0
define rectangles
  set $i=0
  while $i<$race->dirty_row_count
    set $r=&$race->dirty_rows[$i]
    printf "RECT phase=%d xy=%u,%u,%u,%u\n",$arg0,$r->left,$r->top,$r->right,$r->bottom
    set $i=$i+1
  end
end
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break *slicks_race_step
commands
  silent
  if $race && $race->frame_count==$target-1
    set $capturing=1
    enable 3
    printf "DIRTY_BEGIN frame=%lu\n",$race->frame_count
    rectangles 0
  end
  continue
end
break *slicks_mark_dirty_rect
disable 3
commands
  silent
  printf "PUBLICATION caller=%#lx xy=%d,%d,%d,%d\n",*(unsigned long *)$sp,*(short *)($sp+10),*(short *)($sp+14),*(short *)($sp+18),*(short *)($sp+22)
  continue
end
break *slicks_race_clear_dirty_rows
commands
  silent
  if $capturing && $race->frame_count==$target
    rectangles 1
    printf "DIRTY_CAPTURE_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "DIRTY_CAPTURE_EARLY_EXIT\n"
  quit 1
end
continue
