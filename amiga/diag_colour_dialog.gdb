set $editors = 0
set $colours = 0
set $cancelled = 0
break *slicks_diag_profile_editor_ready
commands
  silent
  set $editors = $editors+1
  set $m = g_slicks_player_menu
  printf "COLOUR_EDITOR visit=%u row=%u\n", $editors, $m->editor.row
  if $m->colour_dialog || $m->error || !$m->editor_active
    quit 1
  end
  if $editors == 3
    set $r0 = g_slicks_profiles.setup[3].colours[0]
    set $g0 = g_slicks_profiles.setup[3].colours[1]
    set $b0 = g_slicks_profiles.setup[3].colours[2]
    set $r1 = g_slicks_profiles.setup[3].colours[3]
    set $g1 = g_slicks_profiles.setup[3].colours[4]
    set $b1 = g_slicks_profiles.setup[3].colours[5]
    set $newr0 = $r0>60?63:$r0+3
    set $newg0 = $g0<3?0:$g0-3
    set $newr1 = $r1>60?63:$r1+3
    set $newg1 = $g1>60?63:$g1+3
    set $font = $m->fonts[0][6]
  end
  if $editors >= 7
    if g_slicks_profiles.setup[3].colours[0] != $newr0 || g_slicks_profiles.setup[3].colours[1] != $newg0 || g_slicks_profiles.setup[3].colours[2] != $b0 || $m->fonts[0][6] != $font
      printf "FIRST_COLOUR_COMMIT_OR_FONT_FAILED\n"
      quit 1
    end
    if $editors < 15
      if g_slicks_profiles.setup[3].colours[3] != $r1 || g_slicks_profiles.setup[3].colours[4] != $g1 || g_slicks_profiles.setup[3].colours[5] != $b1
        printf "SECOND_COLOUR_CANCEL_FAILED\n"
        quit 1
      end
    else
      if g_slicks_profiles.setup[3].colours[3] != $newr1 || g_slicks_profiles.setup[3].colours[4] != $newg1 || g_slicks_profiles.setup[3].colours[5] != $b1
        printf "SECOND_COLOUR_COMMIT_FAILED\n"
        quit 1
      end
    end
  end
  continue
end
break *slicks_diag_colour_dialog_ready
commands
  silent
  set $colours = $colours+1
  set $m = g_slicks_player_menu
  if !$m->colour_dialog || $m->error || !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $colours == 4
    if g_slicks_profiles.setup[3].colours[0] != $r0 || g_slicks_profiles.setup[3].colours[1] != $g0
      printf "COLOUR_EDIT_LEAKED_BEFORE_ACCEPT\n"
      quit 1
    end
    dump binary memory .run/colour-dialog-v1/prompt.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    dump binary memory .run/colour-dialog-v1/prompt.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  if $editors == 9
    set $cancelled = 1
  end
  if $editors == 15
    set $m = g_slicks_player_menu
    if !$cancelled || $colours != 10 || g_slicks_profiles.count != 4 || $m->editor_active || $m->colour_dialog || $m->name_dialog || g_slicks_profiles.names[3][2] != 67
      printf "COLOUR_LIFECYCLE_FAILED\n"
      quit 1
    end
    dump binary memory .run/colour-dialog-v1/closed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    dump binary memory .run/colour-dialog-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    printf "COLOUR_DIALOG_OK BOTH_ENDPOINTS_ACCEPT_CANCEL_REOPEN\n"
    quit
  end
  continue
end
continue
