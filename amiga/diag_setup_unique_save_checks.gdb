set $steps = 0
set $saved = 0
break slicks_diag_unique_selection
commands
  silent
  set $p = &g_slicks_setup_session.players
  set $stage = g_slicks_diag_unique_stage
  if $stage != $steps || g_slicks_profiles.count != 4 || g_slicks_profiles.setup[3].flags & 2
    quit 1
  end
  set $owner = 0
  if $stage >= 1 && $stage <= 3
    set $owner = $stage
  end
  if $stage == 5
    set $owner = 2
  end
  set $i = 0
  while $i < 4
    set $expected = 1
    if $i == $owner
      set $expected = 3
    else
      if ($stage >= 1 && $stage <= 3 && $i == 0) || ($stage >= 4 && $i == 3)
        set $expected = 0
      end
    end
    if $p->selected[$i] != $expected
      printf "UNIQUE_ASSIGNMENT_FAILED stage=%d driver=%d actual=%d expected=%d\n",$stage,$i,$p->selected[$i],$expected
      quit 1
    end
    set $i = $i+1
  end
  set $steps = $steps+1
  continue
end
break slicks_diag_setup_saved
commands
  silent
  if $steps != 6 || g_slicks_setup_save_report.result
    quit 1
  end
  eval "dump binary memory %s/saved-profile.bin &g_slicks_profiles.setup[3] &g_slicks_profiles.setup[3]+1", $unique_run
  set $saved = 1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "UNIQUE_PROFILE_ALL_FOUR_SLOTS_DISPLACEMENT_SAVE_OK\n"
  quit
end
continue
