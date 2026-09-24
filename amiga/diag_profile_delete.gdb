set $prompts = 0
set $cancelled = 0
set $pickers = 0
break *slicks_diag_profile_picker_ready
commands
  silent
  set $pickers = $pickers + 1
  if !g_slicks_player_menu->picker || g_slicks_player_menu->picker->renderer.state.action != 1
    printf "DELETE_ACTION_FAILED\n"
    quit 1
  end
  continue
end
break *slicks_diag_player_menu_ready
commands
  silent
  set $m = g_slicks_player_menu
  if !$m || $m->error || !g_slicks_diag_profile_platform->active
    printf "DELETE_MENU_FAILED\n"
    quit 1
  end
  if $m->delete_pending
    set $prompts = $prompts + 1
    if $m->delete_index != 1 || g_slicks_profiles.count != 3
      printf "DELETE_PROMPT_FAILED\n"
      quit 1
    end
    if $prompts == 1
      set $oldcolour = $m->delete_old_colour
      dump binary memory .run/profile-delete-v1/before.profiles &g_slicks_profiles (&g_slicks_profiles)+1
      dump binary memory .run/profile-delete-v1/prompt.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
      dump binary memory .run/profile-delete-v1/prompt.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    end
  else
    if $prompts == 1
      if g_slicks_profiles.count != 3 || $m->fonts[0][6] != $oldcolour
        printf "DELETE_CANCEL_FAILED\n"
        quit 1
      end
      dump binary memory .run/profile-delete-v1/cancelled.profiles &g_slicks_profiles (&g_slicks_profiles)+1
      set $cancelled = 1
    end
    if $prompts == 2
      if !$cancelled || $pickers != 4 || g_slicks_profiles.count != 2 || $m->picker || $m->fonts[0][6] != $oldcolour
        printf "DELETE_CONFIRM_FAILED\n"
        quit 1
      end
      if setup_resources.profile_count != 2
        printf "DELETE_SETUP_COUNT_FAILED\n"
        quit 1
      end
      dump binary memory .run/profile-delete-v1/closed.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
      set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
      dump binary memory .run/profile-delete-v1/closed.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
      printf "PROFILE_DELETE_OK CANCEL_CONFIRM_RESTORE_COUNT\n"
      quit
    end
  end
  continue
end
continue
