set $fidelity_opened = 0
set $fidelity_inputs = 0
break slicks_diag_intermission_checkpoint
commands
  silent
  set $fidelity_opened = $fidelity_opened+1
  if !g_slicks_diag_intermission_menu || g_slicks_diag_intermission_menu->intermission->state.selected != 2
    quit 1
  end
  continue
end
break slicks_diag_intermission_input
commands
  silent
  set $fidelity_inputs = $fidelity_inputs+1
  if g_slicks_diag_intermission_phase != $fidelity_inputs
    quit 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "INTERMISSION_FIDELITY opened=%u inputs=%u restore=%u error=%u\n", $fidelity_opened, $fidelity_inputs, g_slicks_diag_restore_status, g_slicks_diag_race_error
  if $fidelity_opened != 1 || $fidelity_inputs != 9 || g_slicks_diag_restore_status != 31 || g_slicks_diag_race_error
    quit 1
  end
  printf "INTERMISSION_FIDELITY_LIVE_OK\n"
  quit
end
continue
