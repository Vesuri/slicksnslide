set $editors = 0
set $names = 0
set $repeat = 0
break *slicks_diag_name_dialog_ready
commands
  silent
  set $names = $names+1
  set $m = g_slicks_player_menu
  printf "NAME_DIALOG visit=%u text=%s\n", $names, $m->editor_name
  if !$m->name_dialog || !$m->editor_active || $m->error
    printf "NAME_DIALOG_STATE_FAILED\n"
    quit 1
  end
  if $names == 4
    if $m->editor_name[0] != 65 || $m->editor_name[1] != 66 || $m->editor_name[2] != 67 || $m->editor_name[3]
      printf "NAME_KEYMAP_FAILED\n"
      quit 1
    end
    dump binary memory .run/name-dialog-v1/typed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    dump binary memory .run/name-dialog-v1/typed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  end
  if $names == 5 && $m->name_dialog->renderer.entry.position != 3
    printf "NAME_PRESERVE_POSITION_FAILED\n"
    quit 1
  end
  continue
end
break *slicks_diag_profile_editor_ready
commands
  silent
  set $editors = $editors+1
  set $m = g_slicks_player_menu
  set $p = g_slicks_diag_profile_platform
  printf "NAME_EDITOR visit=%u index=%d text=%s keys=%u/%u\n", $editors, $m->editor_index, $m->editor_name, $p->key_tail, $p->key_head
  if !$m->editor_active || $m->name_dialog || $m->error || !$p->active
    quit 1
  end
  if $editors == 3
    if g_slicks_profiles.count != 4 || $m->editor_index != 3 || $m->editor_new || g_slicks_profiles.names[3][2] != 67 || $p->key_head != $p->key_tail
      printf "NAME_CREATE_REOPEN_FAILED\n"
      quit 1
    end
  end
  if $editors == 6 && $m->editor_name[0]
    printf "NAME_ESCAPE_DID_NOT_CLEAR_WORKING_NAME\n"
    quit 1
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  printf "NAME_MENU editors=%u row=%u count=%u\n", $editors, g_slicks_diag_player_menu_row, g_slicks_profiles.count
  if $editors == 4 || $editors == 6
    set $m = g_slicks_player_menu
    set $p = g_slicks_diag_profile_platform
    if $m->editor_active || $m->name_dialog || g_slicks_profiles.count != 4 || g_slicks_profiles.names[3][0] != 65 || g_slicks_profiles.names[3][1] != 66 || g_slicks_profiles.names[3][2] != 88 || g_slicks_profiles.names[3][3] || $p->key_head != $p->key_tail
      printf "NAME_EDIT_COMMIT_FAILED\n"
      quit 1
    end
    if $editors == 4
      set $repeat = 1
    else
      if !$repeat || $names != 9
        quit 1
      end
      dump binary memory .run/name-dialog-v1/closed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      set $bitmap = $p->views[0].bitmap
      dump binary memory .run/name-dialog-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
      printf "NAME_DIALOG_OK CREATE_EDIT_REOPEN_CANCEL\n"
      quit
    end
  end
  continue
end
continue
