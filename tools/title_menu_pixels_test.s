	section code
	dc.l sui_title_menu,slicks_title_labels,slicks_title_font,slicks_title_third_color
	dc.l sui_font_measure,sui_font_string_planar
	include "src/ui/sui_title_menu.s"
	include "src/ui/sui_text.s"
	include "src/ui/sui_bevel.s"
	include "src/util/sutil_palette_nearest.s"
	include "src/graphics/sgfx_span_fill.s"
SLICKS_FONT_PLANAR equ 1
	include "src/ui/sui_font_string.s"
	include "src/ui/sui_font_measure.s"
	include "src/ui/sui_font_glyph_planar.s"
	section data,data
slicks_title_font: dc.l 0
slicks_title_small_font: dc.l 0
slicks_title_third_color: dc.w 0
slicks_title_text_page: dc.w 0
