	section code
	dc.l slicks_title_font_text
	include "src/platform/amiga/native_bridge.s"
SLICKS_FONT_PLANAR equ 1
	include "src/ui/sui_font_string.s"
	include "src/ui/sui_font_measure.s"
	include "src/ui/sui_font_glyph_planar.s"
sui_font_string:
sui_draw_text:
sui_draw_small_text:
sui_title_menu:
sgfx_title_pages:
sgfx_title_crop:
sui_title_step:
sui_title_tail:
sui_title_dispatch:
sgfx_mode_setup:
sgfx_chunky_asset_to_planar:
	rts
slicks_title_counter:
slicks_title_background:
slicks_title_text_page:
slicks_title_render_state:
slicks_title_fallback_color:
slicks_title_phase:
slicks_title_font:
slicks_title_small_font:
slicks_title_third_color:
slicks_title_ordinary_color:
slicks_title_selected_color:
	dc.l 0
