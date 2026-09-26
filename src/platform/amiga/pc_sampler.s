	section	code,code
	xdef	slicks_pc_sampler_handler
	xref	g_slicks_diag_bench_frames
	xref	g_slicks_pc_samples
	xref	g_slicks_pc_sample_count
	xref	g_slicks_pc_sample_capacity
	xref	g_slicks_pc_sample_missed
	xref	g_slicks_pc_sample_period
	xref	g_slicks_pc_sample_timer

; Diagnostic-only CIA-B timer ICR handler (NATURALS benchmark mode).
; ciab.resource calls it through exec's level-6 dispatcher; d0/d1/a0/a1/a5/a6
; are scratch. Each call reloads the running timer's latch with a jittered
; period so samples cannot phase-lock to the display beam, then finds the
; level-6 exception frame (SR.w, PC.l, format/vector $0078) on the
; supervisor stack. Samples are 8 bytes: pc.l, measured-frame index.w,
; stack offset.b, SR high byte.b. Only measured frames 1..602 are kept.
slicks_pc_sampler_handler:
	movem.l	d2-d3,-(sp)
	move.l	g_slicks_pc_sample_period,d0	; xorshift32 jitter
	move.l	d0,d1
	lsl.l	#7,d1
	eor.l	d1,d0
	move.l	d0,d1
	lsr.l	#8,d1
	lsr.l	#1,d1
	eor.l	d1,d0
	move.l	d0,d1
	lsl.l	#8,d1
	eor.l	d1,d0
	move.l	d0,g_slicks_pc_sample_period
	andi.w	#511,d0
	addi.w	#900,d0			; 900..1411 E-clock ticks
	movea.l	g_slicks_pc_sample_timer,a0	; low latch; high is +$100
	move.b	d0,(a0)
	lsr.w	#8,d0
	move.b	d0,$100(a0)
	move.l	g_slicks_diag_bench_frames,d1
	subq.l	#1,d1
	cmpi.l	#602,d1
	bcc.s	.done
	lea	12(sp),a0			; above saved d2/d3 and return
	moveq	#95,d2
.scan:
	cmpi.w	#$0078,6(a0)
	beq.s	.candidate
.next:
	addq.l	#2,a0
	dbf	d2,.scan
	addq.l	#1,g_slicks_pc_sample_missed
	bra.s	.done
.candidate:
	move.b	(a0),d3			; SR high byte: user or supervisor, no trace
	andi.b	#$d8,d3
	bne.s	.next
	btst	#0,5(a0)		; even PC
	bne.s	.next
	move.l	g_slicks_pc_sample_count,d0
	cmp.l	g_slicks_pc_sample_capacity,d0
	bcc.s	.done
	movea.l	g_slicks_pc_samples,a1
	lea	(a1,d0.l*8),a1
	addq.l	#1,d0
	move.l	d0,g_slicks_pc_sample_count
	move.l	2(a0),(a1)+
	move.w	d1,(a1)+
	move.l	a0,d0
	sub.l	sp,d0
	move.b	d0,(a1)+
	move.b	(a0),(a1)
.done:
	movem.l	(sp)+,d2-d3
	moveq	#0,d0
	rts
