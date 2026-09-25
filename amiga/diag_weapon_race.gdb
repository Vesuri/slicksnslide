# Natural shop purchases, configured human fire key and ordinary AI driving.
set $race=0
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_race_progress
commands
  silent
  printf "WEAPON_PROGRESS frame=%u shots=%lu hits=%lu explosions=%lu slots=%u error=%u\n",$race->frame_count,$race->weapons.shots,$race->weapons.hits,$race->weapons.explosions,$race->weapons.slots.high_water,g_slicks_diag_race_error
  set $d=0
  while $d<4
    printf "DRIVER %u role=%d selection=%d request=%u probe=%u service=%d clock=%u\n",$d,$race->participation[$d],$race->selected_weapon[$d],$race->weapons.controls[$d].request,$race->weapons.controls[$d].probe_counter,$race->cars[$d].ai_service_state,$race->game_clock_ticks
    p $race->weapon_inventory[$d]
    set $d=$d+1
  end
  if $race->frame_count>=600
    if $race->weapons.shots != 4 || $race->weapon_inventory[0][5] != 1 || $race->selected_weapon[0] != -1 || g_slicks_diag_race_error
      quit 1
    end
    printf "NATIVE_WEAPON_SHOP_FIRE_DEPLETION_OK\n"
    detach
    quit
  end
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "WEAPON_COLLISION_FAILED frame=%u shots=%lu hits=%lu\n",$race->frame_count,$race->weapons.shots,$race->weapons.hits
  p $race->weapons.projectiles[0]
  p $race->cars[0].x
  p $race->cars[0].y
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  printf "WEAPON_EARLY_EXIT error=%u\n",g_slicks_diag_race_error
  quit 1
end
continue
