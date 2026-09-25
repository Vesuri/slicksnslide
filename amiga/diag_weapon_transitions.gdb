# NATURALP/R/E/C: ordinary shop, fire, pause and intermission keys only.
set $starts=0
set $pauses=0
set $closed=0
set $failures=0
set $saved=0
set $intermissions=0
break enter_prepared_race
commands
  silent
  set $starts=$starts+1
  set $r=race
  printf "WEAPON_TRANSITION_START %u inventory=%d selection=%d\n",$starts,$r->weapon_inventory[0][5],$r->selected_weapon[0]
  if !$r->weapons_enabled || $r->selected_weapon[0]!=0 || $r->weapon_inventory[0][5]!=7-$starts
    quit 1
  end
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  set $pauses=$pauses+1
  set $m=g_slicks_diag_pause_menu
  set $r=g_slicks_diag_paused_race
  set $clock=$r->game_clock_ticks
  set $seed=$r->random_state
  printf "WEAPON_PAUSE %u frame=%u inventory=%d shots=%lu\n",$pauses,$r->frame_count,$r->weapon_inventory[0][5],$r->weapons.shots
  if $r->weapon_inventory[0][5]!=5 || $r->selected_weapon[0]!=0
    quit 1
  end
  eval "dump binary memory .run/weapon-transitions/before%u.inventory &$r->weapon_inventory (char *)&$r->weapon_inventory+sizeof($r->weapon_inventory)",$pauses
  eval "dump binary memory .run/weapon-transitions/before%u.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)",$pauses
  eval "dump binary memory .run/weapon-transitions/before%u.chunky $m->saved $m->saved+64000",$pauses
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  set $closed=$closed+1
  if $closed!=$pauses || $r->game_clock_ticks!=$clock || $r->random_state!=$seed
    quit 1
  end
  eval "dump binary memory .run/weapon-transitions/after%u.inventory &$r->weapon_inventory (char *)&$r->weapon_inventory+sizeof($r->weapon_inventory)",$pauses
  eval "dump binary memory .run/weapon-transitions/after%u.cars &$r->cars (char *)&$r->cars+sizeof($r->cars)",$pauses
  eval "dump binary memory .run/weapon-transitions/after%u.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000",$pauses
  continue
end
break slicks_diag_race_load_failed
commands
  silent
  set $failures=$failures+1
  printf "WEAPON_LOAD_FAILURE %u\n",$failures
  if shop_transition_test!=2 || $failures!=1 || g_slicks_setup_session.inventory[0][5]!=5
    quit 1
  end
  continue
end
break slicks_diag_intermission_checkpoint
commands
  silent
  set $intermissions=$intermissions+1
  if $r->weapon_inventory[0][5]!=5 || $r->selected_weapon[0]!=0
    quit 1
  end
  continue
end
break slicks_diag_saved_ready
commands
  silent
  if g_slicks_diag_saved_phase==3
    if g_slicks_setup_session.inventory[0][5]!=5
      quit 1
    end
    dump binary memory .run/weapon-transitions/saved.inventory &g_slicks_setup_session.inventory (char *)&g_slicks_setup_session.inventory+104
    dump binary memory .run/weapon-transitions/saved.cash &g_slicks_setup_session.cash (char *)&g_slicks_setup_session.cash+8
    dump binary memory .run/weapon-transitions/saved.points &g_slicks_setup_session.points (char *)&g_slicks_setup_session.points+8
    set $saved=1
  end
  continue
end
break slicks_diag_weapon_hud_checked
commands
  silent
  if g_slicks_diag_weapon_hud_failures
    printf "WEAPON_TRANSITION_HUD_FAILED\n"
    quit 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "WEAPON_TRANSITIONS_END mode=%u starts=%u pauses=%u closed=%u failures=%u saved=%u intermissions=%u restore=%u error=%u\n",shop_transition_test,$starts,$pauses,$closed,$failures,$saved,$intermissions,g_slicks_diag_restore_status,g_slicks_diag_race_error
  set $expected=2
  if shop_transition_test>=3
    set $expected=1
  end
  if $starts!=$expected || $pauses!=$expected+1 || $closed!=$pauses || $failures!=(shop_transition_test==2) || $saved!=(shop_transition_test==4) || $intermissions!=(shop_transition_test!=3) || g_slicks_diag_restore_status!=0x1f || g_slicks_diag_race_error || g_slicks_diag_weapon_hud_failures
    quit 1
  end
  printf "NATIVE_WEAPON_TRANSITIONS_OK\n"
  quit
end
continue
