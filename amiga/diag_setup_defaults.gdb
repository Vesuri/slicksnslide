# AUTO with the original configuration, no service/weapon fixture overrides.
set $checked = 0
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  if $race->fuel_option != 0 || $race->damage_scale != 0
    printf "SETUP_DEFAULTS_FAILED FUEL=%d DAMAGE=%d\n",$race->fuel_option,$race->damage_scale
    quit 1
  end
  set $checked = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_race_error
    printf "SETUP_DEFAULTS_LOAD_ERROR=%u\n",g_slicks_diag_race_error
    quit 1
  end
  if g_slicks_diag_ingame
    if !$checked
      quit 1
    end
    if g_slicks_profiles.count != 3 || g_slicks_profiles.setup[0].flags != 7 || g_slicks_profiles.setup[1].flags != 7 || g_slicks_profiles.setup[2].flags != 6 || g_slicks_profiles.setup[1].vehicle != 11 || g_slicks_profiles.setup[2].vehicle != 10 || g_slicks_profiles.setting[0] != 100 || g_slicks_profiles.setting[1] != 100 || g_slicks_profiles.setting[2] != 100
      printf "SETUP_BUILTIN_PROFILES_FAILED COUNT=%d\n",g_slicks_profiles.count
      quit 1
    end
    printf "SETUP_BUILTIN_PROFILES_OK COUNT=%d\n",g_slicks_profiles.count
    printf "SETUP_DEFAULTS_OK FUEL=%d DAMAGE=%d ERROR=%u\n",$race->fuel_option,$race->damage_scale,g_slicks_diag_race_error
    quit
  end
  continue
end
continue
