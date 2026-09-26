# SLICKS_TRACK_ACTOR_TEST=1, with the full-frame stale-pixel audit enabled.
# No target memory writes. debug.sh closes this owned emulator on exit.
set $race=(struct SlicksRaceRuntime *)0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "DIRTY_SPRITE_AUDIT_FAILED frame=%lu xy=%u,%u plane=%u expected=%u actual=%u\n",g_slicks_diag_audit_frame,g_slicks_diag_audit_x,g_slicks_diag_audit_y,g_slicks_diag_audit_plane,g_slicks_diag_audit_before,g_slicks_diag_audit_after
  if $race
    printf "DIRTY_COUNTS rectangles=%u points=%u particles=%u\n",$race->dirty_row_count,$race->dirty_pixel_count,$race->trail_particle_count
    set $i=0
    while $i<$race->dirty_row_count
      printf "RECT %d,%d .. %d,%d\n",$race->dirty_rows[$i].left,$race->dirty_rows[$i].top,$race->dirty_rows[$i].right,$race->dirty_rows[$i].bottom
      set $i=$i+1
    end
    set $i=1
    while $i<$race->weapons.slots.high_water
      set $a=&$race->weapons.actors[$i]
      if $a->old_x<=g_slicks_diag_audit_x && $a->old_x+$a->old_width>g_slicks_diag_audit_x && $a->old_y<=g_slicks_diag_audit_y && $a->old_y+$a->old_height>g_slicks_diag_audit_y
        printf "SPRITE h=%u kind=%u asset=%u frame=%d xy=%d,%d size=%u,%u saved=%u priority=%u mask=%u trail=%d\n",$i,$a->kind,$a->asset,$a->motion.frame,$a->old_x,$a->old_y,$a->old_width,$a->old_height,$a->saved,$a->priority,$a->occlusion,$race->weapons.trail_index[$i]
      end
      set $i=$i+1
    end
    set $i=0
    while $i<$race->trail_particle_count
      set $p=&$race->trail_particles[$i]
      if $p->old_x>=g_slicks_diag_audit_x && $p->old_x<g_slicks_diag_audit_x+8 && $p->old_y==g_slicks_diag_audit_y
        printf "POINT i=%u xy=%d,%d colour=%u priority=%u saved=%u life=%u state=%d\n",$i,$p->old_x,$p->old_y,$p->colour,$p->priority,$p->saved_valid,$p->lifetime,$p->state
      end
      set $i=$i+1
    end
  end
  quit 1
end
break slicks_diag_collision_failed
commands
  silent
  printf "DIRTY_SPRITE_COLLISION_FAILED\n"
  quit 1
end
break slicks_diag_race_progress
commands
  silent
  if $race && $race->frame_count>=600
    if !g_slicks_diag_audit_bitmap || g_slicks_diag_race_error || $race->collision_error
      printf "DIRTY_SPRITE_INVALID\n"
      quit 1
    end
    printf "DIRTY_SPRITE_AUDIT_OK frames=%lu actors=%u marks=%lu\n",$race->frame_count,$race->navigation.actor_count,$race->skidmark_count
    quit
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "DIRTY_SPRITE_EARLY_EXIT\n"
  quit 1
end
continue
quit 1
