# REGCHECK (missing), REGCHECKK (read error), REGCHECKL (close error).
# Error fixtures contain bytes in webf_ord.bmp, never replacement order artwork.
set $external_reads=0
set $exit_screens=0
break slicks_diag_registration_external_loaded
commands
  silent
  set $external_reads=$external_reads+1
  if g_slicks_registration_external_bytes!=-1 || !g_slicks_diag_profile_platform->active || !g_slicks_diag_profile_platform->io_active || g_slicks_diag_profile_platform->gfx_base->ActiView
    quit 1
  end
  if registration_external_test==1
    if !g_slicks_diag_plain_read_reached || g_slicks_diag_plain_read_fault
      quit 1
    end
  end
  if registration_external_test==2
    if !g_slicks_diag_plain_close_reached || g_slicks_diag_plain_close_fault
      quit 1
    end
  end
  continue
end
break slicks_diag_registration_screen_ready
commands
  silent
  if g_slicks_registration_screen==4
    printf "EXTERNAL_FAILED_IMAGE_PRESENTED\n"
    quit 1
  end
  if g_slicks_registration_screen==2
    set $exit_screens=$exit_screens+1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if $external_reads!=1 || $exit_screens!=1 || g_slicks_registration_status || g_slicks_diag_restore_status!=31
    quit 1
  end
  printf "REGISTRATION_EXTERNAL_REJECTED_OK fault=%u reads=%u restore=31\n",registration_external_test,$external_reads
  quit
end
continue
quit 1
