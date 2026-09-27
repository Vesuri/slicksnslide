	include "src/game/track_motion.s"
; Not entered by this test; an accidental call must not silently succeed.
slicks_track_actor_probe:
slicks_track_material_sample:
	illegal
	dc.l slicks_advance_weapon_actors
