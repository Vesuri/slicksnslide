SLICKS_PARTICLE_WORD_COORDINATES equ 1
	include "src/game/particle_draw.s"
	dc.l slicks_particle_address_start
	dc.l slicks_particle_address_end
	dc.l slicks_draw_particle_batch
	dc.l slicks_draw_particle_chain
