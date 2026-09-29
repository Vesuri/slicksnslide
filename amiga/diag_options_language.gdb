# Run OPTIONS on an isolated install with persisted language 2.
# Inspect the real constructor argument, not merely the chosen filename.
break *slicks_amiga_options_menu_create
commands
  silent
  set $heading = *(unsigned char **)($sp+16)
  if menu_language_name[4]!=50 || !title_language_used || $heading < &title_language[0] || $heading >= &title_language[0]+title_language_used
    printf "OPTIONS_LANGUAGE_HEADING_FAILED\n"
    quit 1
  end
  printf "OPTIONS_RESIDENT_LANGUAGE_HEADING_OK\n"
  continue
end
source diag_options_rectangles.gdb
