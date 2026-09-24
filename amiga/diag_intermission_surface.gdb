set $phases = 0
break slicks_diag_intermission_checkpoint
commands
  silent
  set $phases = $phases+1
  if g_slicks_diag_intermission_phase != $phases || !g_slicks_diag_intermission_menu
    quit 1
  end
  set $m = g_slicks_diag_intermission_menu
  if $phases == 4 || $phases == 8 || $phases == 10
    set $platform = g_slicks_diag_profile_platform
    set $bitmap = $platform->views[0].bitmap
    if !$platform->active || $bitmap->BytesPerRow != 320 || $bitmap->Depth != 8
      quit 1
    end
    if $phases == 4
      dump binary memory .run/intermission-owner-v1/open.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      dump binary memory .run/intermission-owner-v1/open.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    end
    if $phases == 8
      dump binary memory .run/intermission-owner-v1/edited.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      dump binary memory .run/intermission-owner-v1/edited.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    end
    if $phases == 10
      dump binary memory .run/intermission-owner-v1/closed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      dump binary memory .run/intermission-owner-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    end
  end
  printf "INTERMISSION_OWNER_PHASE %u\n", $phases
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $phases != 17 || g_slicks_diag_intermission_menu || g_slicks_diag_restore_status != 0x1f
    printf "INTERMISSION_OWNER_FAILED phases=%u restore=%x\n", $phases, g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_INTERMISSION_OWNER_FAILURE_REOPEN_CHANGE_CARS_RESTORE_OK\n"
  quit
end
continue
