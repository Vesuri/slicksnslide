# Normal build; require a real delayed-frame call, then audit all 600 updates.
# Read-only: no debugger injection into race or input state.
set $display_delay_seen=0
set $display_delay_required=1
break *slicks_draw_car_native
commands
  silent
  set $display_race=*(struct SlicksRaceRuntime **)($sp+4)
  set $display_car=*(unsigned int *)($sp+8)
  if $display_car<4 && $display_race->car_display_ready && $display_race->car_display[$display_car].frame!=$display_race->cars[$display_car].heading/1200
    printf "CAR_DISPLAY_DELAY_EXERCISED frame=%lu car=%u heading=%d displayed=%d\n",$display_race->frame_count,$display_car,$display_race->cars[$display_car].heading,$display_race->car_display[$display_car].frame
    set $display_delay_seen=1
    disable 1
  end
  continue
end
source diag_dirty_sprites.gdb
