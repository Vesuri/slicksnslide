	include "src/game/velocity_division.i"
	section code,code
	movem.l d2-d3/d6-d7,-(sp)
	move.l 20(sp),d0
	move.l 24(sp),d2
	move.l 28(sp),d3
	VELOCITY_DIVIDE
	movem.l (sp)+,d2-d3/d6-d7
	rts
