set $warning_owned = 0
set $recovery_returns = 0
break slicks_amiga_platform_end
commands
  silent
  if $warning_owned
    printf "INTERMISSION_WARNING_CLOSE_TEARDOWN\n"
    quit 1
  end
  continue
end
break slicks_amiga_emergency_warning_close
commands
  silent
  if $warning_owned
    if !g_slicks_diag_profile_platform->active
      quit 1
    end
    set $warning_owned = 0
  end
  continue
end
break slicks_diag_intermission_closed
commands
  silent
  set $recovery_returns = $recovery_returns+1
  if !g_slicks_diag_profile_platform->active || g_slicks_diag_intermission_menu || $warning_owned
    printf "INTERMISSION_RECOVERY_RETURN_FAILED\n"
    quit 1
  end
  continue
end
