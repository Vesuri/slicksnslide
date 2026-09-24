set $ready = 0
set $closed = 0
break slicks_diag_help_ready
commands
  silent
  set $v = g_slicks_title_help->help
  if !$v || !$v->renderer.active || $v->navigation.done || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $ready = $ready+1
  set $pixels = $v->renderer.ui.pixels
  printf "TITLE_HELP_READY %u chapter=%lu page=%d\n", $ready, $v->navigation.chapter, $v->navigation.page
  if $ready == 1
    # Original lookup on supplied HELP.TXT: reg -> chapter 353/page 0.
    if $v->navigation.chapter != 353 || $v->navigation.page != 0
      quit 1
    end
    dump binary memory .run/title-help-v1/before.chunky $v->saved $v->saved+64000
    dump binary memory .run/title-help-v1/read.chunky $pixels $pixels+64000
    dump binary memory .run/title-help-v1/help.palette $v->renderer.ui.palette $v->renderer.ui.palette+768
  else
    # Empty title F1 topic is a real anchor, not the in-viewer F1 body.
    # verify-help-index checks this exact result against original DOS code.
    if $v->navigation.chapter != 9589 || $v->navigation.page != 0
      quit 1
    end
    dump binary memory .run/title-help-v1/f1.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_help_closed
commands
  silent
  if g_slicks_title_help || !g_slicks_diag_profile_platform->active
    quit 1
  end
  set $closed = $closed+1
  if $closed == 1
    dump binary memory .run/title-help-v1/after-read.chunky $pixels $pixels+64000
  else
    dump binary memory .run/title-help-v1/after-f1.chunky $pixels $pixels+64000
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $ready != 2 || $closed != 2 || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "TITLE_HELP_READ_F1_RESTORE_OK\n"
  quit
end
continue
