slicks_actor_allocate_native equ $190000
slicks_track_material_sample equ $192000
slicks_race_disable_particles equ $191000
	include "src/game/car_emission.s"
emission_add_entry equ .add
	dc.l emission_add_entry,slicks_race_disable_particles
