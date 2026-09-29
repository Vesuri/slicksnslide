# NATURALWX: cash one below price; NATURALWY: first visible item at capacity.
# NATURALWZ: no remaining carrying capacity for the first weapon.
# Native fixture setup only; all debugger operations are read-only.
set $draws=0
set $seen=0
break *slicks_amiga_shop_refresh
commands
  silent
  set $surface=*(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $draws=$draws+1
  continue
end
break slicks_diag_shop_ready
commands
  silent
  if g_slicks_shop_menu->driver!=0 || g_slicks_shop_menu->row!=0 || $draws!=2
    quit 1
  end
  if !g_slicks_shop_test_phase
    set $initial_cash=g_slicks_setup_session.cash
    set $initial_inventory=g_slicks_setup_session.inventory
    set $publications=$menu_publications
    set $item=g_slicks_shop_rejection_item
    if $item<0 || $item>=13
      quit 1
    end
    if shop_rejection_test==1
      if g_slicks_setup_session.inventory[0][$item] || g_slicks_setup_session.cash[0]!=slicks_original_shop_rules.base_price[$item]/10-1
        quit 1
      end
    else
      if shop_rejection_test==2
        if g_slicks_setup_session.inventory[0][$item]!=slicks_original_shop_rules.capacity[$item]
          quit 1
        end
      else
        if shop_rejection_test!=3 || g_slicks_setup_session.inventory[0][$item] || g_slicks_setup_session.cash[0]<slicks_original_shop_rules.base_price[$item]/10
          quit 1
        end
        set $remaining=slicks_original_shop_rules.vehicle_capacity[g_slicks_setup_session.players.vehicle[0]]
        set $i=0
        set $weapon=0
        while $i<13
          if slicks_original_shop_rules.flags[$i]&2
            if g_slicks_setup_session.inventory[0][$i]>0 || $i==$item
              set $remaining=$remaining-slicks_original_shop_rules.weapon_weight[$weapon]
            end
            set $weapon=$weapon+1
          end
          set $i=$i+1
        end
        if $remaining>0
          quit 1
        end
      end
    end
    if shop_rejection_test<1 || shop_rejection_test>3
        quit 1
    end
  else
    set $d=0
    while $d<4
      if g_slicks_setup_session.cash[$d]!=$initial_cash[$d]
        quit 1
      end
      set $i=0
      while $i<13
        if g_slicks_setup_session.inventory[$d][$i]!=$initial_inventory[$d][$i]
          quit 1
        end
        set $i=$i+1
      end
      set $d=$d+1
    end
    if $menu_publications!=$publications || g_slicks_shop_test_phase>2
      quit 1
    end
    set $seen=$seen+1
  end
  continue
end
break enter_prepared_race
commands
  silent
  if $seen!=2 || g_slicks_shop_test_phase!=2 || $draws!=2 || $menu_publications!=$publications
    printf "SHOP_REJECTION_FAILED\n"
    quit 1
  end
  printf "SHOP_REJECTION_NO_STATE_CHANGE_OR_PUBLICATION_OK case=%u\n",shop_rejection_test
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "SHOP_REJECTION_EARLY_EXIT\n"
  quit 1
end
source diag_menu_rectangles.gdb
continue
