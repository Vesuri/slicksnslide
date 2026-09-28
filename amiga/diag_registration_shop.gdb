set $seen = 0
break slicks_amiga_shop_create
commands
  silent
  if g_slicks_diag_weapon_case != 0 || c->extra != (g_slicks_registration_status == 1)
    quit 1
  end
  set $seen = 1
  printf "REGISTRATION_SHOP_STATE_OK registered=%d\n",g_slicks_registration_status
  continue
end
break slicks_diag_shop_ready
commands
  silent
  if !$seen
    quit 1
  end
  printf "REGISTRATION_NATIVE_SHOP_READY\n"
  quit
end
continue
quit 1
