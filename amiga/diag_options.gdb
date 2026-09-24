set $closed = 0
set $reopened = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  continue
end
break slicks_diag_options_ready
commands
  silent
  set $m = g_slicks_options_menu
  if !$m || $m->error || $m->renderer.error || !g_slicks_diag_profile_platform->active || g_slicks_options_action
    printf "OPTIONS_DRAW_FAILED\n"
    quit 1
  end
  if $closed == 1
    if g_slicks_options_configuration->options[0] != 4 || g_slicks_options_configuration->options[3] != 6 || g_slicks_options_state.row
      printf "OPTIONS_REOPEN_FAILED\n"
      quit 1
    end
    set $reopened = 1
    dump binary memory .run/options-v1/options.chunky $m->renderer.ui.pixels $m->renderer.ui.pixels+64000
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
      quit 1
    end
    dump binary memory .run/options-v1/options.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
    printf "OPTIONS_REOPEN_OK CUSTOM_LAPS_6\n"
  end
  continue
end
break slicks_diag_options_closed
commands
  silent
  set $closed = $closed+1
  if g_slicks_options_menu || g_slicks_options_renderer.surface
    quit 1
  end
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$reopened || $closed != 2 || g_slicks_diag_race_error || $race->laps_to_run != 6
      printf "OPTIONS_RACE_FAILED\n"
      quit 1
    end
    printf "OPTIONS_ENTRY_EDIT_RETURN_REOPEN_RACE_OK\n"
    quit
  end
  continue
end
continue
