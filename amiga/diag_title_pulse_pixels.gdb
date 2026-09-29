# REGCHECKA: capture a complete colour cycle, including no-publication pulses.
# Compare with build/verify_title_return_pixels RUN_DIRECTORY 65.
set $return_pictures=0
set $return_selection=0
set $return_phase=-1
break redraw_title_configuration
commands
  silent
  set $return_selection=selection
  continue
end
break publish_title_dirty
commands
  silent
  if g_slicks_diag_ready && !g_slicks_diag_ingame && $return_pictures<65 && $return_phase!=*(unsigned short *)&slicks_title_phase
    set $return_phase=*(unsigned short *)&slicks_title_phase
    printf "TITLE_RETURN_STATE %u %u %u %d %d %d %d %d %d %d %d %d %u %u %d %d %d\n",$return_pictures,$return_selection,slicks_title_counter,g_slicks_track_playlist.count,g_slicks_diag_track_files,title_configuration->options[0],title_configuration->options[7],title_configuration->options[8],g_slicks_setup_session.players.participation[0],g_slicks_setup_session.players.participation[1],g_slicks_setup_session.players.participation[2],g_slicks_setup_session.players.participation[3],*(unsigned short *)&slicks_title_phase,title_configuration->field_05e1,setup_resources.override_count,title_configuration->options[13],title_configuration->options[14]
    eval "dump binary memory .run/title-return/%u.bin %p %p",$return_pictures,logical,logical+262144
    eval "dump binary memory .run/title-return/%u.owner %p %p",$return_pictures,registration.name,registration.name+61
    set $return_pictures=$return_pictures+1
  end
  continue
end
source diag_title_animation.gdb
