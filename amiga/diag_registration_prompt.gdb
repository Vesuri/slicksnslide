# REGCHECK, keyless isolated directory with registration_test_config's CFG.
set $prompt_seen=0
break *slicks_diag_registration_prompt_ready
commands
  silent
  set $prompt_seen=$prompt_seen+1
  if g_slicks_registration_screen != 1 || registration_painter.count != 1
    quit 1
  end
  set $r=&registration_painter.rectangles[0]
  printf "TRIAL_PROMPT_RECT %u %u %u %u\n",$r->left,$r->top,$r->right,$r->bottom
  if ($r->right-$r->left)*($r->bottom-$r->top) >= 3200
    quit 1
  end
  set $pixels=registration_painter.ui.pixels
  set $view=0
  while $view < 2
    set $planes=g_slicks_diag_profile_platform->views[$view].bitmap->Planes[0]
    eval "dump binary memory .run/menu-rectangles/%u.chunky $pixels $pixels+64000",$view
    eval "dump binary memory .run/menu-rectangles/%u.planar $planes $planes+64000",$view
    set $view=$view+1
  end
  continue
end
break *slicks_diag_system_restored
commands
  silent
  if $prompt_seen != 1 || g_slicks_registration_status || g_slicks_diag_restore_status != 31
    quit 1
  end
  printf "NATIVE_TRIAL_PROMPT_RECTANGLES_RESTORE_OK\n"
  quit
end
continue
