mult320 equ $80000
SLICKS_SPRITE_TEST_BASE equ $12000
slicks_mark_dirty_rect equ $8000
	include "src/game/sprite_opaque.s"
	include "src/game/track_sprite_animation.s"
	include "src/game/track_sprite_animation_publish.s"
	include "race_offsets.i"
	dc.l slicks_draw_animated_track_sprite_publish
	dc.l RACE_DIRTY_ROWS,RACE_DIRTY_ROW_COUNT
