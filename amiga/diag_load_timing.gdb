break slicks_diag_frame_ready
commands
  silent
  if !g_slicks_diag_ingame && g_slicks_diag_ready
    dump binary memory .run/load-timing/title.logical g_slicks_diag_logical g_slicks_diag_logical+0x40000
    up
    dump binary memory .run/load-timing/title.palette source_palette source_palette+768
    down
    disable 1
  end
  continue
end
break enter_prepared_race
commands
  silent
  printf "RACE_LOAD_TICKS "
  set $i=0
  while $i<13
    printf "%lu ",g_slicks_load_ticks[$i]-g_slicks_load_ticks[0]
    set $i=$i+1
  end
  printf "\nTITLE_FONT height=%u glyphs=%u\n",slicks_title_font[2],slicks_title_font[0]
  detach
  quit
end
continue
