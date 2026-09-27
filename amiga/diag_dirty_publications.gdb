# Read-only capture. Set $target before sourcing; interrupted timings are invalid.
set $race=(struct SlicksRaceRuntime *)0
set $capturing=0
define status_snapshot
  printf "STATUS_CLOCK phase=%d ticks=%lu vblank=%lu\n",$arg0,status_clock.ticks,status_clock_vblank
  set $hud_pixels=0
  set $pixel=0
  while $pixel<$race->dirty_pixel_count
    if $race->dirty_pixels[$pixel].y>=187 && $race->dirty_pixels[$pixel].y<190
      set $hud_pixels=$hud_pixels+1
    end
    set $pixel=$pixel+1
  end
  printf "SPARSE phase=%d count=%u hud_bar_pixels=%u\n",$arg0,$race->dirty_pixel_count,$hud_pixels
  set $car=0
  while $car<4
    set $c=&$race->cars[$car]
    printf "STATUS_CAR phase=%d car=%u service=%u fuel=%lu capacity=%lu damage=%d\n",$arg0,$car,$c->service_flags,$c->fuel,$c->fuel_capacity,$c->damage[0]
    set $car=$car+1
  end
end
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
    status_snapshot 0
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
    status_snapshot 1
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
