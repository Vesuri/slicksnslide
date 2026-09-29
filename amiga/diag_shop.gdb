set $shop_draws=0
break *slicks_amiga_shop_refresh
commands
  silent
  set $shop_surface = *(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $shop_draws=$shop_draws+1
  continue
end
break slicks_diag_shop_ready
commands
  silent
  printf "SHOP phase=%u driver=%d count=%d cash=%d inventory=",g_slicks_shop_test_phase,g_slicks_shop_menu->driver,g_slicks_shop_menu->count,g_slicks_setup_session.cash[0]
  set $i=0
  while $i<13
    printf "%d ",g_slicks_setup_session.inventory[0][$i]
    set $i=$i+1
  end
  printf "\n"
  if !g_slicks_shop_test_phase
    dump binary memory .run/shop/screen.chunky $shop_surface->renderer.ui.pixels $shop_surface->renderer.ui.pixels+64000
    dump binary memory .run/shop/screen.palette $shop_surface->palette $shop_surface->palette+768
  end
  continue
end
break enter_prepared_race
commands
  silent
  if g_slicks_shop_test_phase != 3 || g_slicks_shop_help_phase != 2
    printf "SHOP_INPUT_FAILED\n"
    quit 1
  end
  if $shop_draws!=7
    printf "SHOP_REDUNDANT_OR_MISSING_DRAW count=%u\n",$shop_draws
    quit 1
  end
  if g_slicks_setup_session.cash[0] != 891 || g_slicks_setup_session.inventory[0][5] != 5 || race->weapon_inventory[0][5] != 5 || race->selected_weapon[0] != 0 || !race->weapons_enabled
    printf "SHOP_RACE_INVENTORY_FAILED\n"
    quit 1
  end
  printf "SHOP_RACE_ENTRY_OK\n"
  detach
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "SHOP_EARLY_EXIT error=%u\n",g_slicks_diag_race_error
  quit 1
end
continue
