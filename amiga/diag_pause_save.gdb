set $opens = 0
set $steps = 0
set $closed = 0
set $saved = 0
break slicks_diag_pause_live_enter
commands
  silent
  set $opens = $opens+1
  set $r = g_slicks_diag_paused_race
  up-silently
  set $c = configuration
  down-silently
  if $opens == 1
    if $c->field_05de != 100 || $r->frame_count != 100
      quit 1
    end
    set $device = ($c->player_input[0]+1)%3
    set $clock = $r->game_clock_ticks
  else
    if $opens != 2 || $r->frame_count != 150 || $r->game_clock_ticks != $clock+96 || $c->field_05de != 106 || $c->player_input[0] != $device
      printf "PAUSE_SAVE_RESUME_FAILED\n"
      quit 1
    end
  end
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  set $steps = $steps+1
  if $opens == 2 && g_slicks_diag_pause_menu->race_menu->state.row != 5
    quit 1
  end
  continue
end
break slicks_diag_pause_live_closed
commands
  silent
  set $closed = $closed+1
  if $c->field_05de != 106 || $c->player_input[0] != $device || driver_device_configuration_storage.player_input[0] != $device
    quit 1
  end
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $opens != 2 || $closed != 2 || $steps != 14 || g_slicks_setup_save_report.result || g_slicks_diag_ingame
    printf "PAUSE_EDIT_SAVE_FAILED\n"
    quit 1
  end
  dump binary memory .run/pause-save-v1/expected.config $c (char *)$c+sizeof(*$c)
  dump binary memory .run/pause-save-v1/expected.profiles &g_slicks_profiles &g_slicks_profiles+1
  set $saved = 1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "NATIVE_PAUSE_EDIT_RESUME_END_SAVE_OK speed=106 device=%u\n", $device
  quit
end
continue
