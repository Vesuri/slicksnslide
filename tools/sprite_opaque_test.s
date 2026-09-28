mult320 equ $80000
SLICKS_SPRITE_TEST_BASE equ $12000
slicks_mark_dirty_rect equ $8000
	include "src/game/sprite_opaque.s"
	include "src/game/track_sprite_animation.s"
	include "src/game/track_sprite_animation_publish.s"
	include "src/game/track_sprite_fast.s"
	dc.l slicks_draw_sprite_chain
	dc.l slicks_restore_sprite_chain
	dc.l slicks_restore_actor_sprite
	dc.l slicks_draw_unchanged_track_sprite
	dc.l slicks_draw_sprite_visible
