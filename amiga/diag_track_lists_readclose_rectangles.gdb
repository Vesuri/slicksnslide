# TRACKSQ with a valid nonempty SLICKS.TRK in a private fixture.
# TRACKSC is also selected by the scroll-cost probe, so it is not a usable
# unattended list-Close workflow. Adapter host tests cover Close failures.
# Controlled Read/Close result faults; real handles are always closed.
break read_file
commands
  silent
  if g_slicks_diag_track_read_fault
    set $expected_track_fault=g_slicks_diag_track_read_fault
    printf "TRACK_LIST_INJECTED_IO_FAULT %u\n",$expected_track_fault
  end
  continue
end
source diag_track_lists_io_rectangles.gdb
