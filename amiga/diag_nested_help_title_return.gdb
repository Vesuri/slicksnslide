# OPTIONSH/J/O: nested Help visits, parent close, then original title pixels.
# Compare title-return with verify_title_return_pixels RUN 1; compare all
# before/after title-font files. Captures may contain private registration data.
set $return_pictures=0
set $return_selection=0
set $title_fonts_captured=0
define capture_title_fonts
  set $font_index=0
  while $font_index<3
    set $font=$font_index==0?slicks_title_font:($font_index==1?slicks_title_small_font:title_arcade_font)
    set $font_bytes=6+$font[5]+2*$font[0]
    set $widths=$font+6+$font[5]+$font[0]
    set $glyph=0
    while $glyph<$font[0]
      set $font_bytes=$font_bytes+(($widths[$glyph]+3)&~3)*$font[2]
      set $glyph=$glyph+1
    end
    if $font_bytes<7 || $font_bytes>8192
      quit 1
    end
    eval "dump binary memory .run/title-fonts/%u-%u.font %p %p",$arg0,$font_index,$font,$font+$font_bytes
    set $font_index=$font_index+1
  end
end
break slicks_amiga_help_open
commands
  silent
  if !$title_fonts_captured
    set $title_font0=slicks_title_font
    set $title_font1=slicks_title_small_font
    set $title_font2=title_arcade_font
    capture_title_fonts 0
    set $title_fonts_captured=1
  end
  continue
end
break redraw_title_configuration
commands
  silent
  set $return_selection=selection
  if $closed==2 && !g_slicks_options_menu && !g_slicks_player_menu && !g_slicks_track_menu && !$return_pictures
    if !$title_fonts_captured || slicks_title_font!=$title_font0 || slicks_title_small_font!=$title_font1 || title_arcade_font!=$title_font2
      printf "TITLE_FONT_ALIAS_CHANGED\n"
      quit 1
    end
    capture_title_fonts 1
  end
  continue
end
break publish_title_dirty
commands
  silent
  if $closed==2 && !g_slicks_options_menu && !g_slicks_player_menu && !g_slicks_track_menu && !g_slicks_diag_ingame && !$return_pictures
    printf "TITLE_RETURN_STATE %u %u %u %d %d %d %d %d %d %d %d %d %u %u %d %d %d\n",$return_pictures,$return_selection,slicks_title_counter,g_slicks_track_playlist.count,g_slicks_diag_track_files,title_configuration->options[0],title_configuration->options[7],title_configuration->options[8],g_slicks_setup_session.players.participation[0],g_slicks_setup_session.players.participation[1],g_slicks_setup_session.players.participation[2],g_slicks_setup_session.players.participation[3],*(unsigned short *)&slicks_title_phase,title_configuration->field_05e1,setup_resources.override_count,title_configuration->options[13],title_configuration->options[14]
    eval "dump binary memory .run/title-return/%u.bin %p %p",$return_pictures,logical,logical+262144
    eval "dump binary memory .run/title-return/%u.owner %p %p",$return_pictures,registration.name,registration.name+61
    set $return_pictures=$return_pictures+1
  end
  continue
end
source diag_nested_help_rectangles.gdb
