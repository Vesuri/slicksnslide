# B7: SLICKS_TRACK_MENU=20 (TRACKSC). Tracks open plus 12 queued cursor Downs.
# Prints raster lines (64 us) per draw: list redraw, then publication.
set $ready=0
break slicks_diag_tracks_ready
commands
  silent
  set $ready=$ready+1
  if $ready==13
    set $i=0
    while $i<g_slicks_diag_key_cost_count
      printf "TRACK_KEY %u render=%lu publish=%lu\n",$i,g_slicks_diag_key_cost[$i][0],g_slicks_diag_key_cost[$i][1]
      set $i=$i+1
    end
    printf "TRACK_KEY_COST_OK keys=%u\n",g_slicks_diag_key_cost_count
    quit 0
  end
  continue
end
continue
