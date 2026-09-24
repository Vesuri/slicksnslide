	section code
	dc.l slicks_dispatch_title_key,slicks_draw_title_text
	dc.l slicks_draw_title_menu_selection,sui_draw_text,sui_title_menu
	dc.l slicks_draw_original_text,sui_font_string
	include "src/platform/amiga/native_bridge.s"
	include "src/ui/sui_title_dispatch.s"
; Drawing is a boundary here: verify incoming registers, not fake pixels.
sui_draw_text:
sui_font_string:
sui_title_menu:
sgfx_title_pages:
sui_title_step:
sui_title_tail:
sgfx_mode_setup:
sgfx_chunky_asset_to_planar:
	rts
slicks_title_counter:
slicks_title_third_color:
slicks_title_render_state:
slicks_title_fallback_color:
slicks_title_phase:
	dc.l 0
slicks_title_ordinary_color:
	dc.w $12
slicks_title_selected_color:
	dc.w $34
