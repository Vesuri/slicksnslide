# CHAMPLOADW in the NATURALC isolated run. Inspect before the next shop.
set $prepared=0
set $started=0
break slicks_diag_shop_ready
commands
  silent
  up
  printf "WEAPON_RESUME_SHOP track=%d total=%d\n",c.track,c.total
  if c.track!=2 || c.total!=2
    quit 1
  end
  down
  continue
end
break prepare_race
commands
  silent
  if new_game || !session || !session->saved_position_scale_valid || session->inventory[0][5]!=5
    quit 1
  end
  dump binary memory .run/weapon-transitions/loaded.inventory &session->inventory (char *)&session->inventory+104
  dump binary memory .run/weapon-transitions/loaded.cash &session->cash (char *)&session->cash+8
  dump binary memory .run/weapon-transitions/loaded.points &session->points (char *)&session->points+8
  set $prepared=$prepared+1
  continue
end
break enter_prepared_race
commands
  silent
  printf "WEAPON_RESUME inventory=%d selected=%d enabled=%d position=%d\n",race->weapon_inventory[0][5],race->selected_weapon[0],race->weapons_enabled,shop_track_position
  if race->weapon_inventory[0][5]!=5 || race->selected_weapon[0]!=0 || !race->weapons_enabled || shop_track_position!=1 || g_slicks_track_playlist.count!=2
    quit 1
  end
  set $started=$started+1
  continue
end
break slicks_diag_weapon_hud_checked
commands
  silent
  if g_slicks_diag_weapon_hud_failures
    quit 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $prepared!=1 || $started!=1 || !g_slicks_diag_weapon_hud_checks || g_slicks_diag_weapon_hud_failures || g_slicks_diag_race_error || g_slicks_diag_restore_status!=0x1f
    printf "WEAPON_RESUME_FAILED prepared=%u started=%u error=%u restore=%u\n",$prepared,$started,g_slicks_diag_race_error,g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_WEAPON_FRESH_PROCESS_RESUME_OK\n"
  quit
end
continue
