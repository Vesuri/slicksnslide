# B8: SLICKS_TRACK_MENU=1 (TRACKS). The title palette on view 0 when Tracks
# opens must be back on view 0 after Tracks closes.
set $captured=0
set $closed=0
break slicks_amiga_track_menu_create
commands
  silent
  if !$captured
    dump binary memory .run/track-palette-before.bin 'amiga_platform.cpp'::view_palettes[0] 'amiga_platform.cpp'::view_palettes[0]+768
    set $captured=1
  end
  continue
end
break slicks_diag_tracks_closed
commands
  silent
  set $closed=$closed+1
  if $closed==1
    dump binary memory .run/track-palette-after.bin 'amiga_platform.cpp'::view_palettes[0] 'amiga_platform.cpp'::view_palettes[0]+768
    printf "TRACK_PALETTE_DUMPED\n"
    quit 0
  end
  continue
end
continue
