; Experimental exact signed division by a positive, non-power-of-two divisor.
; d0: dividend, d2: ceil(2^(32+d3)/divisor), d3: shift.
; d6/d7 are scratch. Result d0; d1-d5 and all address registers unchanged.
; The multiplier may be unsigned > INT32_MAX: correct the signed high half.
VELOCITY_DIVIDE macro
	move.l d0,d6
	muls.l d2,d7:d0
	tst.l d2
	bpl.s .positive_multiplier\@
	add.l d6,d7
.positive_multiplier\@:
	asr.l d3,d7
	tst.l d6
	bpl.s .positive_dividend\@
	addq.l #1,d7
.positive_dividend\@:
	move.l d7,d0
	endm
