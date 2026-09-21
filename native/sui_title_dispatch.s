	section	code
	xdef	sui_title_dispatch

; Native classification of the eight-key dispatch table in the observed title
; loop at runtime offsets 1A2B2h..1A2C8h.  The returned values are native
; semantic case identifiers; the original near-IP table was:
;   01/44 -> cancel, 1C/1D/39 -> activate, 3B -> help,
;   43 -> result 99, 58 -> reset/configuration.
;
; In:  d0.w = DOS scan code (zero means no event)
; Out: d0.w = 0 unmatched/idle, 1 cancel, 2 activate, 3 help,
;              4 result-99 case, 5 reset/configuration case
; Preserves: d1-d7/a0-a6
sui_title_dispatch:
	cmpi.w	#$0001,d0
	beq.s	.cancel
	cmpi.w	#$0044,d0
	beq.s	.cancel
	cmpi.w	#$001c,d0
	beq.s	.activate
	cmpi.w	#$001d,d0
	beq.s	.activate
	cmpi.w	#$0039,d0
	beq.s	.activate
	cmpi.w	#$003b,d0
	beq.s	.help
	cmpi.w	#$0043,d0
	beq.s	.result_99
	cmpi.w	#$0058,d0
	beq.s	.reset
	moveq	#0,d0
	bra.s	.done
.cancel:
	moveq	#1,d0
	bra.s	.done
.activate:
	moveq	#2,d0
	bra.s	.done
.help:
	moveq	#3,d0
	bra.s	.done
.result_99:
	moveq	#4,d0
	bra.s	.done
.reset:
	moveq	#5,d0
.done:
	rts
