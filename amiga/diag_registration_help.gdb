# REGCHECKY / REGCHECKF supply ordinary raw-key events, not help state.
# Never dump the title, private key, owner name or registration structure.
set $exit=0
set $opened=0
set $closed=0
set $optional=0
set $registration_help_owned=0
init-if-undefined $capture_registration_return = 0
break *slicks_amiga_platform_end
commands
  silent
  if $registration_help_owned
    printf "REGISTRATION_HELP_UNEXPECTED_DISPLAY_RELEASE\n"
    quit 1
  end
  continue
end
break registration_screen
commands
  silent
  if kind == 1
    set $pixels=chunky
  end
  if kind == 2
    set $optional=$optional+1
  end
  continue
end
break slicks_diag_registration_screen_ready
commands
  silent
  if g_slicks_registration_screen == 4
    printf "UNEXPECTED_OPTIONAL_IMAGE test requires original data without webf_ord.bmp\n"
    quit 1
  end
  if g_slicks_registration_screen == 2 || g_slicks_registration_screen == 3
    set $exit=$exit+1
    dump binary memory .run/registration-help/before.chunky $pixels $pixels+64000
  end
  continue
end
break open_help
commands
  silent
  if !$exit || topic[0] != 'r' || topic[1] != 'e' || topic[2] != 'g' || topic[3]
    quit 1
  end
  set $menu=menu
  set $registration_help_owned=1
  continue
end
break slicks_diag_help_ready
commands
  silent
  set $viewer=$menu->help
  if !$viewer || !$viewer->renderer.active || $viewer->navigation.done || $viewer->navigation.chapter != 353 || $viewer->navigation.page != 0 || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $opened=$opened+1
  dump binary memory .run/registration-help/help.chunky $viewer->renderer.ui.pixels $viewer->renderer.ui.pixels+64000
  dump binary memory .run/registration-help/help.palette $viewer->renderer.ui.palette $viewer->renderer.ui.palette+768
  printf "REGISTRATION_HELP_OPEN_OK chapter=353 page=0\n"
  continue
end
break slicks_diag_registration_help_closed
commands
  silent
  set $closed=$closed+1
  set $registration_help_owned=0
  dump binary memory .run/registration-help/after.chunky $pixels $pixels+64000
  if $capture_registration_return
    set $return_view=0
    while $return_view < 2
      set $return_planes=g_slicks_diag_profile_platform->views[$return_view].bitmap->Planes[0]
      eval "dump binary memory .run/menu-rectangles/%u.chunky $pixels $pixels+64000",10000+$return_view
      eval "dump binary memory .run/menu-rectangles/%u.planar $return_planes $return_planes+64000",10000+$return_view
      set $return_view=$return_view+1
    end
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $exit != 1 || $opened != 1 || $closed != 1 || $optional != (g_slicks_registration_status == 0) || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "REGISTRATION_EXIT_HELP_RESTORE_OK registered=%d\n",g_slicks_registration_status
  quit
end
continue
