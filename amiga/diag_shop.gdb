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
    up
    dump binary memory .run/shop/screen.chunky m->renderer.ui.pixels m->renderer.ui.pixels+64000
    dump binary memory .run/shop/screen.palette m->palette m->palette+768
    down
  end
  continue
end
break enter_prepared_race
commands
  silent
  if g_slicks_shop_test_phase != 3
    printf "SHOP_INPUT_FAILED\n"
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
