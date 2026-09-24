set $editors = 0
set $cancelled = 0
break *slicks_diag_profile_editor_ready
commands
  silent
  set $editors = $editors + 1
  set $m = g_slicks_player_menu
  if !$m || !$m->editor_active || $m->editor_index != 3 || $m->error || $m->editor_pending || !$m->editor_new || g_slicks_profiles.count != 3
    printf "EDITOR_STATE_FAILED\n"
    quit 1
  end
  if $editors == 1
    set $oldcolour = $m->editor_old_colour
    set $vehicle = g_slicks_profiles.setup[3].vehicle
  end
  if $editors == 6
    if g_slicks_profiles.setup[3].flags != 1 || g_slicks_profiles.setting[3] != 101 || g_slicks_profiles.setup[3].vehicle != $vehicle+1
      printf "EDITOR_CONTROLS_FAILED\n"
      quit 1
    end
    dump binary memory .run/profile-editor-v1/edited.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    dump binary memory .run/profile-editor-v1/edited.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $editors == 7
    if !$cancelled || g_slicks_profiles.setup[3].flags || g_slicks_profiles.setting[3] != 100 || g_slicks_profiles.setup[3].vehicle != $vehicle+1 || $m->editor_name[0]
      printf "EDITOR_REOPEN_FAILED\n"
      quit 1
    end
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  if $editors >= 6
    set $m = g_slicks_player_menu
    if $m->editor_active || $m->fonts[0][6] != $oldcolour || g_slicks_profiles.count != 3 || $m->error
      printf "EDITOR_CLOSE_FAILED\n"
      quit 1
    end
    if $editors == 6
      if g_slicks_profiles.setup[3].flags != 1 || g_slicks_profiles.setting[3] != 101 || g_slicks_profiles.setup[3].vehicle != $vehicle+1
        printf "EDITOR_CANCEL_PROPERTY_PRESERVATION_FAILED\n"
        quit 1
      end
      set $cancelled = 1
    else
      if $editors != 7 || !$cancelled
        quit 1
      end
      dump binary memory .run/profile-editor-v1/closed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
      dump binary memory .run/profile-editor-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
      printf "PROFILE_EDITOR_OK CONTROLS_CANCEL_REOPEN_EMPTY_REJECT\n"
      quit
    end
  end
  continue
end
continue
