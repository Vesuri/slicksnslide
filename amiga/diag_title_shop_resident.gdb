# STARTGO: title stays owned until shop completion or actual disk preparation.
source diag_menu_rectangles.gdb
set $preparing=0
set $shop_seen=0
break *prepare_race
commands
  silent
  if !g_slicks_diag_profile_platform->active
    printf "TITLE_SHOP_PREMATURE_TEARDOWN\n"
    quit 1
  end
  set $preparing=1
  continue
end
break slicks_diag_shop_ready
commands
  silent
  if !$preparing || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $shop_seen=$shop_seen+1
  printf "TITLE_SHOP_RESIDENT_READY\n"
  continue
end
break *slicks_resource_archive_open
commands
  silent
  if $preparing && g_slicks_diag_profile_platform->active
    printf "TITLE_SHOP_DISK_WHILE_ACTIVE\n"
    quit 1
  end
  continue
end
source diag_title_start.gdb
