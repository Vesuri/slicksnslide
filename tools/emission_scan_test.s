	include "src/game/car_emission.s"
; Entry and both exits of the actual production scan. The verifier stops
; before allocation; unrelated external entries must never be called.
	dc.l .add_scan,.add_found,.add_extend
slicks_track_material_sample:
slicks_actor_allocate_native:
	illegal
slicks_race_disable_particles:
	dc.b 0,0
