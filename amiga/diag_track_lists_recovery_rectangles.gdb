# TRACKSR with an isolated SLICKS.TRK.new or SLICKS.TRK.bak.
source diag_menu_rectangles.gdb
set $track_recovery_loads=0
break slicks_amiga_load_track_lists
commands
  silent
  set $track_recovery_loads=$track_recovery_loads+1
  if $track_recovery_loads!=1
    printf "TRACK_RECOVERY_RELOADED_ON_NAVIGATION\n"
    quit 1
  end
  continue
end
set $expected_load=4
source diag_track_lists_failure.gdb
