# REGCHECKY / REGCHECKF supply ordinary raw-key events, not help state.
# Never dump the title, private key, owner name or registration structure.
set $exit=0
set $opened=0
set $closed=0
set $optional=0
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
  dump binary memory .run/registration-help/after.chunky $pixels $pixels+64000
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
