# SLICKS_NATURAL_RESULTS=damage, FSUAE_RUN=.run/native-results.
# Read-only original-font checkpoint before the natural results/save gate.
break slicks_diag_frame_ready
set $hud_font_checkpoint = $bpnum
commands
  silent
  if g_slicks_diag_ingame
    if !$race || !$race->font.ready || $race->font.height != 6 || $race->font.glyph_count != 147 || $race->font.pixel_count != 3600
      printf "ORIGINAL_HUD_FONT_FAILED\n"
      quit 1
    end
    dump binary memory .run/native-results/hud-start.chunky $race->chunky $race->chunky+64000
    printf "ORIGINAL_HUD_FONT_OK height=%u glyphs=%u\n",$race->font.height,$race->font.glyph_count
    disable $hud_font_checkpoint
  end
  continue
end
source diag_native_results.gdb
