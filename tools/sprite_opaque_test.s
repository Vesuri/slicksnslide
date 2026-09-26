mult320 equ $80000
SLICKS_SPRITE_TEST_BASE equ $12000
	include "src/game/sprite_opaque.s"
	include "src/game/track_sprite_fast.s"
	dc.l slicks_draw_unchanged_track_sprite
	dc.l slicks_draw_sprite_visible
