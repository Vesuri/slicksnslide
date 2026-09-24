set $calls = 0
set $warnings = 0
set $closed = 0
set $opened = 0
set $helpclosed = 0
break open_title_help
commands
  silent
  if !$calls
    set $pixels = chunky
    dump binary memory .run/title-help-failure-v1/before.chunky $pixels $pixels+64000
  end
  set $calls = $calls+1
  continue
end
break slicks_diag_help_failed
commands
  silent
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $warnings < 3
    if !g_slicks_title_help_warning || g_slicks_title_help
      quit 1
    end
  else
    if !g_slicks_title_help || !g_slicks_title_help->help_warning || g_slicks_title_help->help
      quit 1
    end
  end
  set $warnings = $warnings+1
  continue
end
break slicks_diag_help_warning_closed
commands
  silent
  if g_slicks_title_help || g_slicks_title_help_warning || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  if $closed == 1
    dump binary memory .run/title-help-failure-v1/after-archive.chunky $pixels $pixels+64000
  end
  if $closed == 2
    dump binary memory .run/title-help-failure-v1/after-surface.chunky $pixels $pixels+64000
  end
  if $closed == 3
    dump binary memory .run/title-help-failure-v1/after-viewer.chunky $pixels $pixels+64000
  end
  if $closed == 4
    dump binary memory .run/title-help-failure-v1/after-navigation.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_help_ready
commands
  silent
  if !g_slicks_title_help || !g_slicks_title_help->help || !g_slicks_title_help->help->renderer.active
    quit 1
  end
  set $opened = $opened+1
  continue
end
break slicks_diag_help_closed
commands
  silent
  if g_slicks_title_help
    quit 1
  end
  set $helpclosed = $helpclosed+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $calls != 5 || $warnings != 4 || $closed != 4 || $opened != 2 || $helpclosed != 1 || g_slicks_diag_restore_status != 0x1f
    printf "TITLE_HELP_FAILURE_GATE_FAILED calls=%d warnings=%d closed=%d opened=%d helpclosed=%d\n",$calls,$warnings,$closed,$opened,$helpclosed
    quit 1
  end
  printf "TITLE_HELP_ARCHIVE_SURFACE_VIEWER_NAVIGATION_RECOVERY_OK\n"
  quit
end
continue
