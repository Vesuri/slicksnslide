# CONFIG feeds the actual keyboard queue. All target checks are read-only.
set $queued = 0
set $setup_checked = 0
set $draws = 0
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SERVICE_MENU_LOAD_ERROR=%u\n", g_slicks_diag_race_error
    quit 1
  end
  if !$queued
    set $p = (struct SlicksAmigaPlatform *)g_slicks_diag_profile_platform
    if $p->key_head != 13 || $p->key_tail != 0 || g_slicks_diag_ingame
      printf "SERVICE_MENU_INPUT_FAILED\n"
      quit 1
    end
    set $queued = 1
  end
  if g_slicks_diag_ingame
    if !$setup_checked || $draws != 5 || $raceptr->cars[0].fuel_capacity != 3345
      printf "SERVICE_MENU_START_FAILED DRAWS=%u CAPACITY=%u\n", $draws, $raceptr->cars[0].fuel_capacity
      quit 1
    end
    printf "SERVICE_MENU_OK FUEL=10 DAMAGE=20 CAPACITY=%u DRAWS=%u\n", $raceptr->cars[0].fuel_capacity, $draws
    quit
  end
  continue
end
break slicks_race_start
commands
  silent
  if !$queued || race->fuel_option != 10 || race->damage_scale != 20 || !race->navigation.service_available
    printf "SERVICE_MENU_SETUP_FAILED FUEL=%d DAMAGE=%d SERVICE=%u\n", race->fuel_option, race->damage_scale, race->navigation.service_available
    quit 1
  end
  set $setup_checked = 1
  set $raceptr = race
  continue
end
break redraw_service_options
commands
  silent
  set $draws = $draws + 1
  printf "SERVICE_MENU_DRAW SELECTION=%u FUEL=%d DAMAGE=%d\n", selection, fuel, damage
  continue
end
continue
