# Production startup actors, weapons off/on through real Options fixtures.
# SLICKS_OPTIONS_MENU=16 (off) or SLICKS_WEAPON_CASE=1 (on).
# Both fixtures use real menus; assertions/captures are read-only.
set $race=(struct SlicksRaceRuntime *)0
set $seen=0
break *start_race_engines
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+8)
  set $platform=*(struct SlicksAmigaPlatform **)($sp+12)
  set $intro=5+$race->navigation.actor_count
  set $notice=$intro+1
  set $a=&$race->weapons.actors[$intro]
  set $acc=$race->start_light_x<130?-20:20
  if $a->kind!=4 || $a->priority!=10 || $a->motion.ax!=$acc || $a->motion.vx!=$acc || $a->motion.x!=$race->start_light_x*64+$acc || $a->motion.y!=$race->start_light_y*64 || $a->motion.lifetime!=29
    printf "START_ACTOR_CONSTRUCTOR_FAILED\n"
    quit 1
  end
  if $race->weapons_enabled
    set $n=&$race->weapons.actors[$notice]
    if $n->kind!=5 || $n->priority!=3 || $n->motion.x!=956 || $n->motion.y!=960 || $n->motion.vx!=-4 || $n->motion.lifetime!=199 || !$race->start_notice_ready
      printf "START_NOTICE_CONSTRUCTOR_FAILED\n"
      quit 1
    end
  end
  printf "START_ACTOR_SETUP weapons=%u slot=%u x=%d y=%d vx=%d ax=%d life=%d\n",$race->weapons_enabled,$intro,$a->motion.x,$a->motion.y,$a->motion.vx,$a->motion.ax,$a->motion.lifetime
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if $race && $race->frame_count
    set $f=$race->frame_count
    set $a=&$race->weapons.actors[$intro]
    if $f<=29 && ($race->weapons.slots.state[$intro]<=0 || $a->motion.x!=$race->start_light_x*64+$acc*($f+1)*($f+2)/2 || $a->motion.vx!=$acc*($f+1) || $a->motion.lifetime!=($f==29?1:29-$f))
      printf "START_ACTOR_MOTION_FAILED frame=%lu x=%d vx=%d life=%d\n",$f,$a->motion.x,$a->motion.vx,$a->motion.lifetime
      quit 1
    end
    if $f==30 && $race->weapons.slots.state[$intro]>0 && $a->kind==4
      printf "START_ACTOR_EXPIRY_FAILED\n"
      quit 1
    end
    if $race->weapons_enabled
      set $n=&$race->weapons.actors[$notice]
      if $f<=199 && ($race->weapons.slots.state[$notice]<=0 || $n->motion.x!=960-4*($f+1) || $n->motion.lifetime!=($f==199?1:199-$f))
        printf "START_NOTICE_MOTION_FAILED frame=%lu x=%d life=%d\n",$f,$n->motion.x,$n->motion.lifetime
        quit 1
      end
      if $f==200 && $race->weapons.slots.state[$notice]>0 && $n->kind==5
        printf "START_NOTICE_EXPIRY_FAILED\n"
        quit 1
      end
    end
    if $f==1 || $f==10 || $f==30 || $f==200
      set $bm=$platform->views[1].bitmap
      if $bm->BytesPerRow!=320 || $bm->Depth!=8 || $bm->Planes[1]-$bm->Planes[0]!=40
        quit 1
      end
      eval "dump binary memory ../tmp/start-actors-%u-%lu.chunky %lu %lu",$race->weapons_enabled,$f,$race->chunky,$race->chunky+64000
      eval "dump binary memory ../tmp/start-actors-%u-%lu.planes %lu %lu",$race->weapons_enabled,$f,$bm->Planes[0],$bm->Planes[0]+64000
      eval "dump binary memory ../tmp/start-actors-%u.palette %lu %lu",$race->weapons_enabled,&view_palettes[1][0],&view_palettes[1][0]+768
      set $seen=$seen+1
    end
    if $f==200
      if $seen!=4 || g_slicks_diag_race_error || $race->collision_error
        quit 1
      end
      printf "START_ACTORS_OK weapons=%u frames=200 intro_life=30 notice_life=200 captures=4\n",$race->weapons_enabled
      quit
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "START_ACTORS_EARLY_EXIT error=%u\n",g_slicks_diag_race_error
  quit 1
end
continue
quit 1
