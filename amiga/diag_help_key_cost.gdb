# B6: SLICKS_HELP_MENU=9 (HELPK). Title Help, 12 queued Downs, Escape.
# Prints raster lines (64 us) per key: navigation+render, then publication.
break slicks_diag_help_closed
commands
  silent
  set $i=0
  while $i<g_slicks_diag_key_cost_count
    printf "HELP_KEY %u render=%lu publish=%lu\n",$i,g_slicks_diag_key_cost[$i][0],g_slicks_diag_key_cost[$i][1]
    set $i=$i+1
  end
  printf "HELP_KEY_COST_OK keys=%u\n",g_slicks_diag_key_cost_count
  quit 0
end
continue
