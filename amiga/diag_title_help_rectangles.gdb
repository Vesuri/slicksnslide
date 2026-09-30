# Fresh .run/menu-rectangles plus .run/title-help-v1 required.
source diag_menu_rectangles.gdb
break *slicks_amiga_platform_end
commands
  silent
  if g_slicks_title_help
    printf "TITLE_HELP_UNEXPECTED_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
break *slicks_resource_archive_open_impl
commands
  silent
  if g_slicks_title_help
    printf "TITLE_HELP_UNEXPECTED_ARCHIVE_IO\n"
    quit 1
  end
  continue
end
source diag_title_help.gdb
