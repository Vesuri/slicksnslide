# REGCHECKD, with a persisted language-2 configuration; read-only checks.
break *sui_title_menu
commands
  silent
  if menu_language_name[4]!=50 || !title_language_used
    printf "TITLE_MENU_LANGUAGE_SELECTION_FAILED\n"
    quit 1
  end
  set $i=0
  while $i<7
    if slicks_title_labels[$i] < &title_language[0] || slicks_title_labels[$i] >= &title_language[0]+title_language_used
      printf "TITLE_MENU_LANGUAGE_POINTER_FAILED row=%u\n",$i
      quit 1
    end
    set $i=$i+1
  end
  printf "TITLE_MENU_RESIDENT_LABELS_OK\n"
  continue
end
source diag_title_dirty.gdb
