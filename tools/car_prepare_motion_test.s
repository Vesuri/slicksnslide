slicks_prepare_ai_controls equ $90010
slicks_prepare_weapon_controls equ $90020
slicks_resolve_track_velocity equ $90040
slicks_car_direction_x equ $30000
slicks_car_direction_y equ $30020
SLICKS_PREPARE_INTEGRATOR_BASE equ $10000
SLICKS_MOTION_REGISTER_ENTRY equ 1
	include "src/game/car_prepare.s"
	include "src/game/car_motion.s"
	dc.l slicks_prepare_all_car_motion,slicks_integrate_car_motion_regs
