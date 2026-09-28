# Read-only order snapshots from the normal binary. Stops invalidate timings.
# Run sequentially; move the local-only capture before starting another track.
set $race=(struct SlicksRaceRuntime *)0
set $last_frame=0
set $captures=0
set $point_calls=0
set $sprite_calls=0
set $general_calls=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break *slicks_draw_particle_chain
commands
  silent
  set $point_calls=$point_calls+1
  continue
end
break *slicks_draw_sprite_chain
commands
  silent
  set $sprite_calls=$sprite_calls+1
  continue
end
break *draw_weapon_actor_general
commands
  silent
  set $general_calls=$general_calls+1
  # Drawing precedes the increment; publication observes the completed frame.
  if $race && ($race->frame_count==480 || $race->frame_count==550 || $race->frame_count==612 || $race->frame_count==684)
    set $h=*(unsigned *)($sp+8)
    set $a=&$race->weapons.actors[$h]
    set $prev=&$race->sprite_dirty_previous[$h]
    printf "DRAW_FALLBACK frame=%lu handle=%u kind=%u/%u asset=%u/%u animation=%d/%u xy=%d,%d/%d,%d colour=%u/%u layer=%u/%u occlusion=%u/%u\n",$race->frame_count+1,$h,$a->kind,$prev->kind,$a->asset,$prev->asset,$a->motion.frame,$prev->frame,($a->motion.x>>6),($a->motion.y>>6),$prev->x,$prev->y,$a->colour,$prev->colour,$a->priority,$prev->priority,$a->occlusion,$prev->occlusion
  end
  continue
end
break *slicks_race_clear_dirty_rows
commands
  silent
  if $race && $race->frame_count>=98 && $race->frame_count<=700 && $race->frame_count!=$last_frame
    if !$race->actor_order_drawn || g_slicks_diag_race_error
      printf "DRAW_CHAIN_SCREEN_INVALID\n"
      quit 1
    end
    if !$captures
      dump binary memory ../tmp/draw-chain-screen.bin &$race->frame_count ((char *)&$race->frame_count)+4
    else
      append binary memory ../tmp/draw-chain-screen.bin &$race->frame_count ((char *)&$race->frame_count)+4
    end
    append binary memory ../tmp/draw-chain-screen.bin &$race->actor_order_head &$race->actor_order_head+1
    append binary memory ../tmp/draw-chain-screen.bin &$race->actor_order_next &$race->actor_order_next+1
    append binary memory ../tmp/draw-chain-screen.bin &$race->actor_order_max &$race->actor_order_max+1
    append binary memory ../tmp/draw-chain-screen.bin &$race->actor_order_tail &$race->actor_order_tail+1
    append binary memory ../tmp/draw-chain-screen.bin &$race->actor_order_previous &$race->actor_order_previous+1
    append binary memory ../tmp/draw-chain-screen.bin &$race->weapons.trail_index &$race->weapons.trail_index+1
    printf "DRAW_DISPATCH frame=%lu point=%u sprite=%u general=%u\n",$race->frame_count,$point_calls,$sprite_calls,$general_calls
    set $captures=$captures+1
    set $last_frame=$race->frame_count
    if $last_frame==700
      if $captures!=603
        printf "DRAW_CHAIN_SCREEN_INVALID captures=%u\n",$captures
        quit 1
      end
      printf "DRAW_CHAIN_SCREEN_OK captures=%u\n",$captures
      printf "FINAL_STATE marks=%lu x=%ld,%ld,%ld,%ld y=%ld,%ld,%ld,%ld\n",$race->skidmark_count,$race->cars[0].x,$race->cars[1].x,$race->cars[2].x,$race->cars[3].x,$race->cars[0].y,$race->cars[1].y,$race->cars[2].y,$race->cars[3].y
      quit
    end
  end
  set $point_calls=0
  set $sprite_calls=0
  set $general_calls=0
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "DRAW_CHAIN_SCREEN_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
