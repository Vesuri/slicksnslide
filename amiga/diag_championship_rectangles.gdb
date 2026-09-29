source diag_menu_rectangles.gdb
set $capture_cup_pixels = 1
break slicks_diag_record_results_ready
commands
  silent
  if g_slicks_diag_record_results_phase == 2
    set $record_palette = $menu->renderer.ui.palette
    set $record_colour = 0
    while $record_colour < 256
      set $r8 = ($record_palette[3*$record_colour]<<2)|($record_palette[3*$record_colour]>>4)
      set $g8 = ($record_palette[3*$record_colour+1]<<2)|($record_palette[3*$record_colour+1]>>4)
      set $b8 = ($record_palette[3*$record_colour+2]<<2)|($record_palette[3*$record_colour+2]>>4)
      set $high = (($r8&240)<<4)|($g8&240)|($b8>>4)
      set $low = (($r8&15)<<8)|(($g8&15)<<4)|($b8&15)
      set $copper = g_slicks_diag_profile_platform->views[0].copper
      if ($copper[palette_words[0][0][$record_colour]]&65535) != $high || ($copper[palette_words[0][1][$record_colour]]&65535) != $low
        printf "RECORD_PALETTE_MISMATCH %u high=%x/%x low=%x/%x\n",$record_colour,$copper[palette_words[0][0][$record_colour]]&65535,$high,$copper[palette_words[0][1][$record_colour]]&65535,$low
        quit 1
      end
      set $record_colour = $record_colour+1
    end
    printf "RECORD_PALETTE_OK\n"
  end
  continue
end
source diag_championship_resident.gdb
