# Read-only order snapshots from the normal binary. Stops invalidate timings.
# Run sequentially; move the local-only capture before starting another track.
set $race=(struct SlicksRaceRuntime *)0
set $last_frame=0
set $captures=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break *slicks_race_clear_dirty_rows
commands
  silent
  if $race && $race->frame_count>=98 && $race->frame_count<=700 && $race->frame_count!=$last_frame
    if !$race->actor_order_drawn || g_slicks_diag_race_error
      printf "ORDER_SCREEN_INVALID\n"
      quit 1
    end
    if !$captures
      dump binary memory ../tmp/order-reuse-screen.bin &$race->frame_count ((char *)&$race->frame_count)+4
    else
      append binary memory ../tmp/order-reuse-screen.bin &$race->frame_count ((char *)&$race->frame_count)+4
    end
    append binary memory ../tmp/order-reuse-screen.bin &$race->actor_order_head &$race->actor_order_head+1
    append binary memory ../tmp/order-reuse-screen.bin &$race->actor_order_next &$race->actor_order_next+1
    append binary memory ../tmp/order-reuse-screen.bin &$race->actor_order_max &$race->actor_order_max+1
    append binary memory ../tmp/order-reuse-screen.bin &$race->actor_order_tail &$race->actor_order_tail+1
    append binary memory ../tmp/order-reuse-screen.bin &$race->actor_order_previous &$race->actor_order_previous+1
    set $captures=$captures+1
    set $last_frame=$race->frame_count
    if $last_frame==700
      if $captures!=603
        printf "ORDER_SCREEN_INVALID captures=%u\n",$captures
        quit 1
      end
      printf "ORDER_SCREEN_OK captures=%u\n",$captures
      printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",$race->skidmark_count,$race->cars[0].x,$race->cars[1].x,$race->cars[2].x,$race->cars[3].x,$race->cars[0].y,$race->cars[1].y,$race->cars[2].y,$race->cars[3].y
      quit
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "ORDER_SCREEN_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
