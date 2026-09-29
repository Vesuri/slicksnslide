# Create .run/title-help-failure-v1 and .run/title-help-publications first.
# Decode the six fallback captures with tools/check_menu_publications.py.
set $calls = 0
set $warnings = 0
set $closed = 0
set $opened = 0
set $helpclosed = 0
set $title_publications = 0
break *open_title_help
commands
  silent
  if !$calls
    set $pixels = *(unsigned char **)($sp+12)
    dump binary memory .run/title-help-failure-v1/before.chunky $pixels $pixels+64000
  end
  if $calls < 3
    eval "dump binary memory .run/title-help-failure-v1/warning-%u-before.chunky %p %p", $calls, $pixels, $pixels+64000
  end
  set $calls = $calls+1
  printf "TITLE_HELP_CALL %u\n",$calls
  continue
end
break slicks_diag_help_failed
commands
  silent
  printf "TITLE_HELP_FAILURE %u allocation_fault=%u keymap=%u\n",$warnings,g_slicks_diag_help_fail_allocation,menu_keymap_ready
  if !g_slicks_diag_profile_platform->active
    quit 1
  end
  if $warnings < 3
    if !g_slicks_title_help_warning || g_slicks_title_help
      quit 1
    end
    if g_slicks_title_last_pixels != 6336
      printf "TITLE_WARNING_BOUNDS_FAILED pixels=%lu\n",g_slicks_title_last_pixels
      quit 1
    end
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    eval "dump binary memory .run/title-help-publications/%u.chunky %p %p", $title_publications, $pixels, $pixels+64000
    eval "dump binary memory .run/title-help-publications/%u.planar %p %p", $title_publications, $bitmap->Planes[0], $bitmap->Planes[0]+64000
    set $title_publications = $title_publications+1
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
  printf "TITLE_HELP_WARNING_CLOSE %u\n",$closed
  if g_slicks_title_help || g_slicks_title_help_warning || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  if $closed <= 3
    eval "dump binary memory .run/title-help-failure-v1/warning-%u-after.chunky %p %p", $closed-1, $pixels, $pixels+64000
    if g_slicks_title_last_pixels != 6336
      quit 1
    end
    set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
    eval "dump binary memory .run/title-help-publications/%u.chunky %p %p", $title_publications, $pixels, $pixels+64000
    eval "dump binary memory .run/title-help-publications/%u.planar %p %p", $title_publications, $bitmap->Planes[0], $bitmap->Planes[0]+64000
    set $title_publications = $title_publications+1
  end
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
  printf "TITLE_HELP_READY %u\n",$opened
  if !g_slicks_title_help || !g_slicks_title_help->help || !g_slicks_title_help->help->renderer.active
    quit 1
  end
  if g_slicks_title_help->help->renderer.saved.pixels != &g_slicks_title_help->saved[0]
    printf "TITLE_HELP_BACKING_NOT_BORROWED\n"
    quit 1
  end
  set $opened = $opened+1
  continue
end
break slicks_diag_help_closed
commands
  silent
  printf "TITLE_HELP_CLOSE %u\n",$helpclosed
  if g_slicks_title_help
    quit 1
  end
  set $helpclosed = $helpclosed+1
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $calls != 5 || $warnings != 4 || $closed != 4 || $opened != 2 || $helpclosed != 1 || $title_publications != 6 || g_slicks_diag_restore_status != 0x1f
    printf "TITLE_HELP_FAILURE_GATE_FAILED calls=%d warnings=%d closed=%d opened=%d helpclosed=%d\n",$calls,$warnings,$closed,$opened,$helpclosed
    quit 1
  end
  printf "TITLE_HELP_ARCHIVE_SURFACE_VIEWER_NAVIGATION_RECOVERY_OK\n"
  quit
end
continue
