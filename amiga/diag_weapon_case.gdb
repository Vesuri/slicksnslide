# Set SLICKS_WEAPON_CASE=1..8 for single weapons, 9 for manual/empty cycling.
# No inventory/projectile/position writes: ordinary purchases and input only.
set $race=0
break *run_shop
commands
  silent
  printf "WEAPON_CASE_CONFIGURED %u\n",g_slicks_diag_weapon_case
  if g_slicks_diag_weapon_case!=$weapon_case
    quit 1
  end
  continue
end
break *slicks_race_start
commands
  silent
  set $race=*(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_weapon_hud_checked
commands
  silent
  printf "WEAPON_HUD case=%u count=%d selection=%d checks=%u failures=%u\n",$weapon_case,weapon_hud_last_count,weapon_hud_last_selection,g_slicks_diag_weapon_hud_checks,g_slicks_diag_weapon_hud_failures
  if g_slicks_diag_weapon_hud_failures
    quit 1
  end
  continue
end
break slicks_diag_race_progress
commands
  silent
  set $item=$weapon_case+4
  if $weapon_case==9
    set $item=6
  end
  printf "WEAPON_CASE case=%u frame=%u shots=%lu hits=%lu explosions=%lu inventory=%d selection=%d error=%u\n",$weapon_case,$race->frame_count,$race->weapons.shots,$race->weapons.hits,$race->weapons.explosions,$race->weapon_inventory[0][$item],$race->selected_weapon[0],g_slicks_diag_race_error
  if $race->frame_count>=600
    if $weapon_case==9
      if $race->weapons.shots!=15 || $race->weapon_inventory[0][5]!=6 || $race->weapon_inventory[0][6]!=1 || $race->selected_weapon[0]!=0 || g_slicks_diag_weapon_hud_failures || g_slicks_diag_weapon_hud_checks<7 || g_slicks_diag_race_error
        quit 1
      end
      printf "NATIVE_TWO_WEAPON_MANUAL_AND_DEPLETION_CYCLING_OK\n"
      detach
      quit
    end
    set $expected=5
    if $weapon_case==2
      set $expected=15
    end
    if $weapon_case==4 || $weapon_case==6 || $weapon_case==8
      set $expected=1
    end
    if $race->weapons.shots!=$expected || $race->weapon_inventory[0][$weapon_case+4]!=1 || $race->selected_weapon[0]!=-1 || g_slicks_diag_race_error || g_slicks_diag_weapon_hud_failures || g_slicks_diag_weapon_hud_checks<2
      quit 1
    end
    printf "NATIVE_WEAPON_CASE_OK %u\n",$weapon_case
    detach
    quit
  end
  continue
end
break slicks_diag_collision_failed
commands
  silent
  printf "WEAPON_CASE_COLLISION_FAILURE %u\n",$weapon_case
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  printf "WEAPON_CASE_EARLY_EXIT %u error=%u\n",$weapon_case,g_slicks_diag_race_error
  quit 1
end
continue
