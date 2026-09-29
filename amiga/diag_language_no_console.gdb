# HELPL9 <NIL: -- real non-interactive console rejection before takeover.
set $chooser=0
break choose_startup_language
commands
  silent
  set $chooser=$chooser+1
  set $p=g_slicks_diag_profile_platform
  set $view=$p->gfx_base->ActiView
  set $vbi=SysBase->IntVects[5]
  printf "LANGUAGE_CHOOSER_ENTRY fixture=%u active=%u\n",language_choice_test,$p->active
  if $p->active || language_choice_test!=2
    quit 1
  end
  continue
end
break slicks_amiga_store_setup
commands
  silent
  printf "LANGUAGE_FAILURE_UNEXPECTED_SAVE\n"
  quit 1
end
break slicks_amiga_platform_begin
commands
  silent
  printf "LANGUAGE_FAILURE_UNEXPECTED_TAKEOVER\n"
  quit 1
end
break slicks_diag_system_restored
commands
  silent
  printf "LANGUAGE_REJECTION_STATE chooser=%u modes=%u reads=%u ready=%u fixture=%u\n",$chooser,g_slicks_language_console_modes,g_slicks_language_console_bytes,g_slicks_diag_ready,language_choice_test
  printf "OS_VIEW before=%p after=%p active=%u\n",$view,$p->gfx_base->ActiView,$p->active
  printf "OS_VBI before=%p/%p/%p after=%p/%p/%p\n",$vbi.iv_Data,$vbi.iv_Code,$vbi.iv_Node,SysBase->IntVects[5].iv_Data,SysBase->IntVects[5].iv_Code,SysBase->IntVects[5].iv_Node
  # DOS may open its console View to print the error. The application must
  # never install its own display, change VBI ownership, or save setup.
  if $chooser!=1 || g_slicks_language_console_modes || g_slicks_language_console_bytes || $p->active || g_slicks_diag_ready
    quit 1
  end
  if SysBase->IntVects[5].iv_Data!=$vbi.iv_Data || SysBase->IntVects[5].iv_Code!=$vbi.iv_Code || SysBase->IntVects[5].iv_Node!=$vbi.iv_Node
    quit 1
  end
  printf "LANGUAGE_NONINTERACTIVE_REJECT_OK\n"
  detach
  quit
end
continue
