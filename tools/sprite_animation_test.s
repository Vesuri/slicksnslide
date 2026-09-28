mult320 equ $80000
SLICKS_SPRITE_TEST_BASE equ $12000
	include "src/game/sprite_opaque.s"
	include "src/game/track_sprite_animation.s"
	dc.l slicks_draw_animated_track_sprite
