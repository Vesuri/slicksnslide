# REGCHECKM, keyless fixture. Preserve the normal exit/help/restoration gate.
set $modifier_keys=0
break slicks_diag_registration_help_key
commands
  silent
  set $v=g_slicks_registration_help_viewer
  printf "REGISTRATION_HELP_MODIFIER_KEY n=%u ascii=%u modifiers=%u chapter=%lu\n",$modifier_keys,g_slicks_registration_help_ascii,g_slicks_registration_help_modifiers,$v->navigation.chapter
  if $modifier_keys==0
    set $contents=$v->navigation.chapter
  end
  if $modifier_keys==2
    if g_slicks_registration_help_ascii!=13 || $v->navigation.chapter==$contents || $v->navigation.selections[0]<0
      quit 1
    end
  end
  if $modifier_keys==3
    if g_slicks_registration_help_ascii!=75 || !(g_slicks_registration_help_modifiers & 1)
      quit 1
    end
  end
  if $modifier_keys==4
    if g_slicks_registration_help_ascii!=98 || g_slicks_registration_help_modifiers || $v->navigation.chapter!=$contents || $v->navigation.selections[0]!=-1
      printf "REGISTRATION_HELP_RELEASE_HISTORY_FAILED\n"
      quit 1
    end
  end
  if $modifier_keys==5
    if g_slicks_registration_help_ascii!=27 || !$v->navigation.done
      quit 1
    end
    printf "REGISTRATION_HELP_MODIFIER_RELEASE_HISTORY_OK\n"
  end
  set $modifier_keys=$modifier_keys+1
  continue
end
source diag_menu_rectangles.gdb
source diag_registration_help.gdb
