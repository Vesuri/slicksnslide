# Set $target to the race update to inspect before sourcing this script.
# Diagnostic read-only snapshots; interrupted timings are not benchmarks.
# Read-only pre/post inspection. Never use these interrupted timings.
set $race=(struct SlicksRaceRuntime *)0
define snapshot
  printf "SNAPSHOT phase=%d frame=%lu count=%u page=%u\n",$arg0,$race->frame_count,$race->trail_particle_count,$race->actor_page
  set $i=0
  while $i<$race->trail_particle_count
    set $p=&$race->trail_particles[$i]
    printf "POINT i=%u h=%u xy=%ld,%ld v=%d,%d old=%d,%d priority=%u saved=%u life=%u state=%d permanent=%u colour=%u mask=%u\n",$i,$race->weapons.trail_handle[$i],$p->x,$p->y,$p->velocity_x,$p->velocity_y,$p->old_x,$p->old_y,$p->priority,$p->saved_valid,$p->lifetime,$p->state,$p->permanent,$p->colour,$p->occlusion_limit
    set $i=$i+1
  end
  set $i=0
  while $i<4
    set $c=&$race->cars[$i]
    printf "CAR i=%u rect=%d,%d,%u,%u saved=%u priority=%u\n",$i,$c->old_x,$c->old_y,$c->old_width,$c->old_height,$c->saved_valid,($c->actor_layer?3:4)
    set $s=&$race->shadows[$i]
    printf "SHADOW i=%u rect=%u,%u,%u,%u saved=%u priority=2\n",$i,$s->old_x,$s->old_y,$s->old_width,$s->old_height,$s->saved_valid
    set $i=$i+1
  end
  set $i=1
  while $i<$race->weapons.slots.high_water
    set $a=&$race->weapons.actors[$i]
    if $race->weapons.trail_index[$i]<0 && $a->saved
      printf "SPRITE h=%u rect=%d,%d,%u,%u priority=%u retain=%u\n",$i,$a->old_x,$a->old_y,$a->old_width,$a->old_height,$a->priority,$a->retain
    end
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
    snapshot 0
  end
  continue
end
break *slicks_race_clear_dirty_rows
commands
  silent
  if $race && $race->frame_count==$target
    snapshot 1
    set $i=0
    while $i<$race->dirty_row_count
      set $r=&$race->dirty_rows[$i]
      printf "RECT %u,%u,%u,%u\n",$r->left,$r->top,$r->right,$r->bottom
      set $i=$i+1
    end
    printf "INSPECTION_OK\n"
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "INSPECTION_EARLY_EXIT\n"
  quit 1
end
continue
