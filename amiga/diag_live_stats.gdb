# NATURALO1: opt-in statistics still describe the actual converted regions.
# No game-state writes; debug.sh closes the emulator on exit.
set $race=(struct SlicksRaceRuntime *)0
set $checks=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "LIVE_STATS_BITMAP_FAILED\n"
  quit 1
end
break slicks_diag_collision_failed
commands
  silent
  printf "LIVE_STATS_COLLISION_FAILED\n"
  quit 1
end
# Inspect before the main loop clears the dirty list. race_progress runs
# afterwards and therefore cannot reconstruct this frame's converted area.
break *slicks_race_clear_dirty_rows
commands
  silent
  if $race && $race->frame_count>0
    set $area=0
    set $rows=0
    set $i=0
    while $i<$race->dirty_row_count
      set $r=&$race->dirty_rows[$i]
      set $pixels=($r->right-$r->left)*($r->bottom-$r->top)
      set $area=$area+$pixels
      set $rows=$rows+$pixels/320
      set $i=$i+1
    end
    if !g_slicks_diag_live_stats || g_slicks_diag_profile_rect_pixels!=$area || g_slicks_diag_dirty_rows!=$rows || g_slicks_diag_dirty_ranges!=$race->dirty_row_count || g_slicks_diag_sparse_converted!=$race->dirty_pixel_count || g_slicks_diag_dirty_pixels<g_slicks_diag_sparse_converted || g_slicks_diag_race_error
      printf "LIVE_STATS_FAILED frame=%lu area=%lu/%lu rows=%lu/%lu\n",$race->frame_count,g_slicks_diag_profile_rect_pixels,$area,g_slicks_diag_dirty_rows,$rows
      quit 1
    end
    set $checks=$checks+1
    if $race->frame_count>=600
      printf "LIVE_STATS_OK frames=%lu checks=%u\n",$race->frame_count,$checks
      quit
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "LIVE_STATS_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
