set $warning_owned = 0
set $recovery_returns = 0
set $recovery_captures = 0
define capture_intermission_warning
  if !help_warning.renderer.active || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $recovery_pixels = help_warning.renderer.painter.ui.pixels
  set $warning_bitmap = g_slicks_diag_profile_platform->views[0].bitmap
  set $race_bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  eval "dump binary memory .run/intermission-warnings/%u.chunky %p %p",$recovery_captures,$recovery_pixels,$recovery_pixels+64000
  eval "dump binary memory .run/intermission-warnings/%u.planar %p %p",$recovery_captures,$warning_bitmap->Planes[0],$warning_bitmap->Planes[0]+64000
  eval "dump binary memory .run/intermission-return/%u.entry %p %p",$recovery_captures,$race_bitmap->Planes[0],$race_bitmap->Planes[0]+64000
  set $recovery_captures = $recovery_captures+1
end
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
  if $recovery_captures != 1 || help_warning.renderer.active
    quit 1
  end
  dump binary memory .run/intermission-return/0.chunky $recovery_pixels $recovery_pixels+64000
  dump binary memory .run/intermission-return/0.planar $race_bitmap->Planes[0] $race_bitmap->Planes[0]+64000
  continue
end
