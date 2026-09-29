set $owned=0
set $warnings=0
set $closed=0
set $optional=0
break registration_screen
commands
  silent
  if kind==1
    set $pixels=chunky
  end
  if kind==2
    set $optional=$optional+1
  end
  continue
end
break registration_help_unavailable
commands
  silent
  if registration_help_test!=4 && registration_help_test!=5 && registration_help_test!=6
    quit 1
  end
  set $owned=1
  continue
end
break slicks_diag_registration_screen_ready
commands
  silent
  if g_slicks_registration_screen==2
    dump binary memory .run/registration-early/before.chunky $pixels $pixels+64000
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $owned
    printf "EARLY_HELP_RELEASED_DISPLAY\n"
    quit 1
  end
  continue
end
break *slicks_chunky_rows_to_amiga
commands
  silent
  if $owned
    printf "EARLY_HELP_FULL_ROW_CONVERSION\n"
    quit 1
  end
  continue
end
break slicks_diag_registration_warning_ready
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $warnings=$warnings+1
  set $planes=g_slicks_diag_profile_platform->views[0].bitmap->Planes[0]
  dump binary memory .run/menu-rectangles/0.chunky $pixels $pixels+64000
  dump binary memory .run/menu-rectangles/0.planar $planes $planes+64000
  continue
end
break slicks_diag_registration_help_closed
commands
  silent
  if !$owned || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $owned=0
  set $closed=$closed+1
  dump binary memory .run/registration-early/after.chunky $pixels $pixels+64000
  set $view=0
  while $view<2
    set $planes=g_slicks_diag_profile_platform->views[$view].bitmap->Planes[0]
    eval "dump binary memory .run/menu-rectangles/%u.chunky $pixels $pixels+64000",$view+1
    eval "dump binary memory .run/menu-rectangles/%u.planar $planes $planes+64000",$view+1
    set $view=$view+1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $warnings!=1 || $closed!=1 || $optional!=1 || g_slicks_diag_restore_status!=31 || g_slicks_registration_status!=0
    quit 1
  end
  printf "REGISTRATION_EARLY_HELP_FAILURE_RESTORE_OK case=%u\n",registration_help_test
  quit
end
continue
