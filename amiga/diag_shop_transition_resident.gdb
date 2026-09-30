# NATURALP/R: after initial preparation, entries from intermission or a
# retry notice must retain the custom display.
source diag_menu_rectangles.gdb
set $preparations=0
break *prepare_race
commands
  silent
  if $preparations && !g_slicks_diag_profile_platform->active
    printf "SHOP_TRANSITION_PREMATURE_TEARDOWN\n"
    quit 1
  end
  set $preparations=$preparations+1
  printf "SHOP_TRANSITION_PREPARATION %u active=%u\n",$preparations,g_slicks_diag_profile_platform->active
  continue
end
break *slicks_resource_archive_open
commands
  silent
  if $preparations && g_slicks_diag_profile_platform->active
    printf "SHOP_TRANSITION_DISK_WHILE_ACTIVE\n"
    quit 1
  end
  continue
end
source diag_weapon_transitions.gdb
