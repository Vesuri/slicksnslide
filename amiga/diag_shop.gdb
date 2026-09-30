set $shop_draws=0
set $shop_second_driver=0
break *slicks_amiga_shop_refresh
commands
  silent
  set $shop_surface = *(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $shop_draws=$shop_draws+1
  if g_slicks_shop_menu && g_slicks_shop_menu->driver==1
    set $shop_second_driver=1
  end
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
  if !$_isvoid($shop_fault_expected)
    if g_slicks_shop_create_checks!=$shop_fault_expected || g_slicks_shop_create_free_before!=g_slicks_shop_create_free_after
      printf "SHOP_CREATE_CLEANUP_FAILED\n"
      quit 1
    end
    printf "SHOP_CREATE_FAILURE_CLEANUP_OK checks=%u\n",g_slicks_shop_create_checks
  end
  if g_slicks_shop_test_phase != 4 || g_slicks_shop_help_phase != 2 || !$shop_second_driver
    printf "SHOP_INPUT_FAILED phase=%u help=%u second_driver=%u\n",g_slicks_shop_test_phase,g_slicks_shop_help_phase,$shop_second_driver
    quit 1
  end
  if $shop_draws!=9
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
