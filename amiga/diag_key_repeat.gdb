# SLICKS_HOLD_TEST=1 (HOLDT): Down held on the title for 150 vblanks, then
# released. Prints the BIOS tick of every selection step for
# tools/check_key_repeat_steps.py (original 36ce0, title argument 2).
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_hold_release_tick
    printf "HOLD start=%lu release=%lu steps=%lu\n",g_slicks_diag_hold_start_tick,g_slicks_diag_hold_release_tick,g_slicks_diag_hold_steps
    set $i=0
    while $i<g_slicks_diag_hold_steps
      printf "HOLD_STEP %lu\n",g_slicks_diag_hold_ticks[$i]
      set $i=$i+1
    end
    printf "HOLD_DONE\n"
    quit
  end
  continue
end
continue
