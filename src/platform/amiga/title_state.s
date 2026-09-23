	section	data,data
	xdef	slicks_title_counter
	xdef	slicks_title_third_color
	xdef	slicks_title_render_state
	xdef	slicks_title_fallback_color
	xdef	slicks_title_phase
	xdef	slicks_title_ordinary_color
	xdef	slicks_title_selected_color

slicks_title_counter:
	dc.b	0
	even
slicks_title_third_color:
	dc.w	0
slicks_title_render_state:
	dc.b	0,0,0,0,0,8
	dc.b	0,0,0,0,0,0,0,0
	even
slicks_title_fallback_color:
	dc.w	0
slicks_title_phase:
	dc.w	0
slicks_title_ordinary_color:
	dc.w	0
slicks_title_selected_color:
	dc.w	0
