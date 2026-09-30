# Included after the shared pulse capture. Arcade advances its own counter;
# the 65-frame original-wrapper verifier checks counter/phase progression.
break slicks_diag_system_restored
commands
  silent
  if !title_configuration || title_configuration->options[0]!=5 || $return_pictures!=65 || g_slicks_title_arcade_draws<65 || g_slicks_title_dirty_checks<2 || g_slicks_title_dirty_errors || !$pulse_initial_full || g_slicks_title_full_publications!=$pulse_initial_full || g_slicks_diag_restore_status!=31
    printf "ARCADE_PULSE_FAILED pictures=%u draws=%lu checks=%lu errors=%lu\n",$return_pictures,g_slicks_title_arcade_draws,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors
    quit 1
  end
  printf "ARCADE_PULSE_OK pictures=%u draws=%lu checks=%lu full=%lu restore=%u\n",$return_pictures,g_slicks_title_arcade_draws,g_slicks_title_dirty_checks,g_slicks_title_full_publications,g_slicks_diag_restore_status
  quit
end
continue
quit 1
