# OPTIONSTI with a saved language-2 configuration. Read target memory only.
break *slicks_amiga_intermission_open
commands
  silent
  if menu_language_name[4]!=50
    quit 1
  end
  set $content=*(struct SlicksIntermissionContent **)($sp+8)
  set $row=2
  while $row<4
    set $key=&slicks_original_race_menu_keys[$row+2][0]
    set $entry=&title_language[0]
    set $expected=(unsigned char *)0
    while $entry < &title_language[0]+title_language_used && *$entry && !$expected
      set $i=0
      while $key[$i] && $entry[$i]==$key[$i]
        set $i=$i+1
      end
      if !$key[$i] && $entry[$i]==61
        set $expected=$entry+$i+1
      else
        while *$entry
          set $entry=$entry+1
        end
        set $entry=$entry+1
      end
    end
    if !$expected
      quit 1
    end
    set $i=0
    while $i<64 && $content->labels[$row][$i]==$expected[$i] && $expected[$i]
      set $i=$i+1
    end
    if $i==64 || $content->labels[$row][$i] || $expected[$i]
      printf "INTERMISSION_LANGUAGE_LABEL_FAILED row=%u\n",$row
      quit 1
    end
    set $row=$row+1
  end
  printf "INTERMISSION_TRANSLATED_LABELS_OK\n"
  continue
end
source diag_intermission_live_rectangles.gdb
