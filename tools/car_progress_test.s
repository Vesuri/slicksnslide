slicks_progress_checkpoint_crossed equ $90010
slicks_progress_lap_crossed equ $90020
mult320 equ $a0000
	include "src/game/car_progress.s"
	dc.l slicks_advance_car_progress_regs
