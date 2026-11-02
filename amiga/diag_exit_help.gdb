# REGCHECKY / REGCHECKF supply ordinary raw-key events, not help state.
# Never dump the title, private key or owner name.
set $exit=0
set $opened=0
set $closed=0
set $exit_help_owned=0
init-if-undefined $capture_exit_return = 0
init-if-undefined $exit_help_failure = 0
init-if-undefined $exit_help_backing_failure = 0
set $warnings=0
break *slicks_resource_archive_open_impl
commands
  silent
  if $exit_help_owned
    printf "EXIT_HELP_UNEXPECTED_ARCHIVE_OPEN\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $exit_help_owned
    printf "EXIT_HELP_UNEXPECTED_DISPLAY_RELEASE\n"
    quit 1
  end
  continue
end
break exit_screen
commands
  silent
  set $pixels=chunky
  continue
end
break slicks_diag_exit_screen_ready
commands
  silent
  if g_slicks_exit_screen != 1
    quit 1
  end
  set $exit=$exit+1
  dump binary memory .run/exit-help/before.chunky $pixels $pixels+64000
  continue
end
break open_help
commands
  silent
  if !$exit || topic[0] != 'r' || topic[1] != 'e' || topic[2] != 'g' || topic[3]
    quit 1
  end
  set $menu=menu
  set $exit_help_owned=1
  continue
end
break slicks_diag_help_ready
commands
  silent
  if $exit_help_failure
    printf "EXIT_HELP_FAULT_NOT_REACHED\n"
    quit 1
  end
  set $viewer=$menu->help
  if !$viewer || !$viewer->renderer.active || $viewer->navigation.done || $viewer->navigation.chapter != 353 || $viewer->navigation.page != 0 || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $opened=$opened+1
  dump binary memory .run/exit-help/help.chunky $viewer->renderer.ui.pixels $viewer->renderer.ui.pixels+64000
  dump binary memory .run/exit-help/help.palette $viewer->renderer.ui.palette $viewer->renderer.ui.palette+768
  printf "EXIT_HELP_OPEN_OK chapter=353 page=0\n"
  continue
end
break slicks_diag_help_failed
commands
  silent
  if !$exit_help_failure || !$menu || $menu->help || !$menu->help_warning || !g_slicks_diag_profile_platform->active || g_slicks_diag_help_fail_allocation
    printf "EXIT_HELP_WARNING_FAILED\n"
    quit 1
  end
  set $warnings=$warnings+1
  if $exit_help_backing_failure
    if exit_help_test!=8 || g_slicks_diag_help_fail_backing || g_slicks_diag_help_backing_fault_reached!=1
      printf "EXIT_HELP_BACKING_FAULT_NOT_REACHED\n"
      quit 1
    end
  end
  continue
end
break slicks_diag_exit_help_closed
commands
  silent
  set $closed=$closed+1
  set $exit_help_owned=0
  dump binary memory .run/exit-help/after.chunky $pixels $pixels+64000
  if $capture_exit_return
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
  if $exit != 1 || $opened != 1-$exit_help_failure || $warnings != $exit_help_failure || $closed != 1 || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "EXIT_HELP_RESTORE_OK owner=%d\n",g_slicks_registration_status
  if $exit_help_failure
    printf "EXIT_HELP_ALLOCATION_WARNING_RESTORE_OK\n"
  end
  if $exit_help_backing_failure
    printf "EXIT_HELP_BACKING_WARNING_RESTORE_OK\n"
  end
  quit
end
continue
