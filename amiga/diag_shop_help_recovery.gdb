source diag_menu_rectangles.gdb
set $help_opens=0
set $warnings=0
set $dismissed=0
break *slicks_amiga_help_open
commands
  silent
  set $owner=*(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $help_opens=$help_opens+1
  if $help_opens==1
    dump binary memory .run/shop-help-recovery/before.chunky $owner->renderer.ui.pixels $owner->renderer.ui.pixels+64000
    set $cash=g_slicks_setup_session.cash[0]
  end
  continue
end
break slicks_diag_help_failed
commands
  silent
  if !$owner->help_warning || $owner->help || !shop_help_fault_sent || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $warnings=$warnings+1
  continue
end
break slicks_diag_help_warning_closed
commands
  silent
  if $owner->help_warning || $owner->help || !g_slicks_diag_profile_platform->active || g_slicks_setup_session.cash[0]!=$cash
    quit 1
  end
  dump binary memory .run/shop-help-recovery/after.chunky $owner->renderer.ui.pixels $owner->renderer.ui.pixels+64000
  set $dismissed=$dismissed+1
  continue
end
break enter_prepared_race
commands
  silent
  if $warnings!=1 || $dismissed!=1 || $help_opens!=2 || g_slicks_shop_help_phase!=2 || g_slicks_diag_race_error
    quit 1
  end
  printf "SHOP_HELP_PARSE_RECOVERY_RACE_OK\n"
  quit
end
break slicks_diag_system_restored
commands
  silent
  printf "SHOP_HELP_RECOVERY_EARLY_EXIT\n"
  quit 1
end
continue
