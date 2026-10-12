# Fresh race: compare stage/cue/clock ordering and capture actual bitplanes.
# SLICKS_AUTO_RACE=1 SLICKS_DEBUG_WARP=1 ./debug.sh '' diag_start_lights.gdb
# tools/verify_amiga_frame.py compares each chunky/planar dump afterward.
set $race=(struct SlicksRaceRuntime *)0
set $cues=0
break *start_race_engines
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+8)
  set $platform=*(struct SlicksAmigaPlatform **)($sp+12)
  continue
end
break *'play_effect_period.part.0'
commands
  silent
  if $race && *(unsigned long *)($sp+8)==1
    set $stage=$cues==0?1:2
    if $cues>=4 || $race->countdown_stage!=$stage || $race->start_light_stage_mask!=((1<<($stage+1))-1) || !$race->start_light_visible || $race->frame_count || $race->game_clock_ticks || $race->cars[0].elapsed_time_units
      printf "START_LIGHT_ORDER_FAILED cue=%u stage=%u mask=%u clock=%lu\n",$cues,$race->countdown_stage,$race->start_light_stage_mask,$race->game_clock_ticks
      quit 1
    end
    printf "START_LIGHT_CUE %u stage=%u xy=%d,%d clock=0\n",$cues+1,$stage,$race->start_light_x,$race->start_light_y
    if $cues<2
      set $bm=$platform->views[1].bitmap
      if $bm->BytesPerRow!=320 || $bm->Depth!=8 || $bm->Planes[1]-$bm->Planes[0]!=40
        printf "START_LIGHT_BITMAP_LAYOUT_FAILED\n"
        quit 1
      end
      eval "dump binary memory ../tmp/start-light-%u.chunky %lu %lu",$stage,$race->chunky,$race->chunky+64000
      eval "dump binary memory ../tmp/start-light-%u.planes %lu %lu",$stage,$bm->Planes[0],$bm->Planes[0]+64000
    end
    set $cues=$cues+1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $race && $race->frame_count
    if $cues!=4 || $race->frame_count!=1 || $race->game_clock_ticks!=1 || $race->cars[0].elapsed_time_units!=2 || $race->start_light_visible || g_slicks_diag_race_error
      printf "START_LIGHT_HANDOVER_FAILED\n"
      quit 1
    end
    printf "START_LIGHT_ORDER_OK cues=4 stages=1/2/2/2 first_tick=1\n"
    quit
  end
  continue
end
continue
printf "START_LIGHT_UNEXPECTED_STOP\n"
quit 1
