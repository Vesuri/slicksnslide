# SLICKS_TRACK_ACTOR_TEST=1. Native bounded exit; no debugger memory writes.
set $race=(struct SlicksRaceRuntime *)0
set $checked=0
set $grid=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  printf "TRACK_ACTOR_START count=%u ready=%u\n",$race->navigation.actor_count,$race->track_actors_ready
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame && $race && !$race->frame_count
    if !$race->chunky_authoritative || !g_slicks_diag_audit_bitmap || $race->trail_particle_count
      printf "TRACK_ACTOR_GRID_FAILED\n"
      quit 1
    end
    set $i=0
    while $i<$race->navigation.actor_count
      set $h=$race->track_actor_handles[$i]
      if $h!=$i+5 || $race->weapons.actors[$h].kind!=3 || $race->weapons.actors[$h].motion.period<10 || $race->weapons.actors[$h].motion.period>17
        printf "TRACK_ACTOR_GRID_SLOT_FAILED\n"
        quit 1
      end
      if $race->navigation.actors[$i].kind==2 && $race->weapons.actors[$h].saved
        printf "TRACK_ACTOR_HIDDEN_FLAG_FAILED\n"
        quit 1
      end
      set $i=$i+1
    end
    set $grid=1
    printf "TRACK_ACTOR_GRID_OK count=%u seed=%lx\n",$race->navigation.actor_count,$race->random_state
  end
  continue
end
break slicks_diag_race_progress
commands
  silent
  if $race->frame_count>=600
    if !$race->track_actors_ready || !$race->navigation.actor_count || $race->collision_error || g_slicks_diag_race_error
      printf "TRACK_ACTOR_TARGET_FAILED\n"
      quit 1
    end
    set $i=0
    while $i<$race->navigation.actor_count
      set $h=$race->track_actor_handles[$i]
      if $h != $i+5 || $race->weapons.actors[$h].kind!=3
        printf "TRACK_ACTOR_HANDLE_FAILED index=%u handle=%u\n",$i,$h
        quit 1
      end
      printf "TRACK_ACTOR index=%u kind=%u xy=%d,%d layer=%u frame=%d period=%d saved=%u\n",$i,$race->navigation.actors[$i].kind,$race->navigation.actors[$i].x,$race->navigation.actors[$i].y,$race->navigation.actors[$i].layer,$race->weapons.actors[$h].motion.frame,$race->weapons.actors[$h].motion.period,$race->weapons.actors[$h].saved
      set $i=$i+1
    end
    printf "TRACK_ACTOR_TARGET_OK frame=%u actors=%u slots=%u marks=%lu flags=%d\n",$race->frame_count,$race->navigation.actor_count,$race->weapons.slots.high_water,$race->skidmark_count,$race->track_flag_activations
    set $checked=1
  end
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "TRACK_ACTOR_COLLISION_ERROR frame=%u\n",$race->frame_count
  quit 1
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "TRACK_ACTOR_BITMAP_FAILED\n"
  quit 1
end
break slicks_diag_results_ready
commands
  silent
  if !$race->track_actors_ready || !$race->navigation.actor_count || !$race->finished_count || $race->collision_error
    printf "TRACK_ACTOR_FINISH_FAILED\n"
    quit 1
  end
  if $race->navigation.actor_count==2 && ($race->finished_count!=4 || $race->track_flag_activations!=4)
    printf "TRACK_ACTOR_BASIC_FINISH_FAILED\n"
    quit 1
  end
  printf "TRACK_ACTOR_NATURAL_FINISH frame=%u finished=%u flags=%d slots=%u marks=%lu\n",$race->frame_count,$race->finished_count,$race->track_flag_activations,$race->weapons.slots.high_water,$race->skidmark_count
  set $checked=1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$checked || !$grid || g_slicks_diag_race_error || g_slicks_diag_restore_status!=0x1f || g_slicks_audio_vbi_spills
    printf "TRACK_ACTOR_EXIT_FAILED error=%u restore=%u audio_spills=%lu\n",g_slicks_diag_race_error,g_slicks_diag_restore_status,g_slicks_audio_vbi_spills
    quit 1
  end
  printf "TRACK_ACTOR_RESTORE_OK audio_spills=0\n"
  quit
end
continue
printf "TRACK_ACTOR_UNEXPECTED_STOP\n"
info registers
bt
x/24wx $sp
quit 1
