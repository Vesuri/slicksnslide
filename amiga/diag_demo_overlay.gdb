# Renderer integration only: SLICKS_DEMO_RENDER_TEST=1 selects a negative flag
# in the native launch mode. This fixture only reads state; no debugger writes.
# This deliberately does not claim title entry/return lifecycle coverage.
# Use the ordinary dirty-sprite audit launch (muted by debug.sh).
break *slicks_race_set_demo
commands
  silent
  if *(signed char *)($sp+11)!=-1
    printf "DEMO_RENDER_BIND_FAILED\n"
    quit 1
  end
  printf "DEMO_RENDER_BIND_OK\n"
  continue
end
tbreak *draw_arcade_timer
commands
  silent
  set $demo_race=*(struct SlicksRaceRuntime **)($sp+4)
  if $demo_race->demo_flag!=-1 || !$demo_race->demo_palette || !$demo_race->demo_label[0]
    printf "DEMO_RENDER_STATE_FAILED\n"
    quit 1
  end
  printf "DEMO_RENDER_STATE_OK\n"
  continue
end
tbreak *race_demo_text
commands
  silent
  if *(short *)($sp+14)!=11 || *(short *)($sp+18)!=11 || *(unsigned char *)($sp+23)!=0
    printf "DEMO_RENDER_TEXT_FAILED\n"
    quit 1
  end
  printf "DEMO_RENDER_TEXT_OK\n"
  continue
end
source diag_dirty_sprites.gdb
