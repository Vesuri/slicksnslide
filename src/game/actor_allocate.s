	section code,code
	xdef slicks_actor_allocate_native
; C ABI: pool, resource_present. Only d0/d1/a0/a1 are changed.
; Original 330ab: lowest zero-state handle, otherwise extend high-water.
; All negative states remain reserved. Null resource does not activate slots.
slicks_actor_allocate_native:
	movea.l 4(sp),a0
	tst.w 202(a0)
	beq.s .none
	move.w 200(a0),d0
	subq.w #2,d0
	bmi.s .extend
	lea 1(a0),a1
.scan:
	tst.b (a1)+
	dbeq d0,.scan
	bne.s .extend
	move.l a1,d0
	sub.l a0,d0
	subq.l #1,d0
	bra.s .activate
.extend:
	tst.l 8(sp)
	beq.s .none
	moveq #0,d0
	move.w 200(a0),d0
	cmp.w 202(a0),d0
	bcc.s .none
	addq.w #1,200(a0)
.activate:
	tst.l 8(sp)
	beq.s .return
	move.b #1,(a0,d0.w)
.return:
	rts
.none:
	moveq #0,d0
	rts

	xdef slicks_actor_reset_native
; Clear 33 metadata bytes plus alignment padding, leaving pixels untouched.
; C ABI: actor. Only a0/d0 change; actor buffers are longword-aligned.
slicks_actor_reset_native:
	movea.l 4(sp),a0
	moveq #0,d0
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)
	rts
