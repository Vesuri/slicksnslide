	section	code
	xdef	sui_font_cache
	xdef	sui_font_glyph_flush
	xdef	slicks_font_glyph_map

; Glyph offsets are the original's low-word sum of padded width*height over
; all preceding glyphs (2ff0c..2ff2b). Computing them per character costs
; O(glyph index) multiplies. Together with the linear code searches in the
; string and measure routines this made one Help page take about 0.4 s on the
; target. Cache the identical offsets for the four most recent fonts, plus the
; character->glyph map (first matching index wins; absent codes map to the
; count). Entries are keyed by address and the first four header bytes.
; Font decoding calls sui_font_glyph_flush; palette bytes (+6..) do not
; affect either table. Storage stays in the code section so the
; raw-binary oracle harnesses can include this file unchanged.
SUI_GLYPH_CACHE_ENTRIES	set	4
SUI_GLYPH_CACHE_OFFSETS	set	8
SUI_GLYPH_CACHE_MAP	set	8+2*256
SUI_GLYPH_CACHE_ENTRY	set	8+2*256+256
; a1 = runtime font. Returns a4 = its cache entry, filling it on a miss.
; Preserves every other register.
sui_font_cache:
	lea	sui_glyph_cache(pc),a4
	movem.l	d5/d6,-(sp)
	move.l	(a1),d5		; count, advance, height, spacing
	moveq	#SUI_GLYPH_CACHE_ENTRIES-1,d6
.probe:
	cmpa.l	(a4),a1
	bne.s	.next
	cmp.l	4(a4),d5
	beq.w	.hit
.next:
	lea	SUI_GLYPH_CACHE_ENTRY(a4),a4
	dbra	d6,.probe
	movem.l	d0-d5/d7/a0/a2/a5,-(sp)
	lea	sui_glyph_cache_next(pc),a0
	move.w	(a0),d6
	move.w	d6,d5
	addq.w	#1,d5
	andi.w	#SUI_GLYPH_CACHE_ENTRIES-1,d5
	move.w	d5,(a0)
	mulu.w	#SUI_GLYPH_CACHE_ENTRY,d6
	lea	sui_glyph_cache(pc),a4
	adda.l	d6,a4
	move.l	a1,(a4)
	move.l	(a1),4(a4)
	moveq	#0,d3
	move.b	(a1),d3		; count
	moveq	#0,d4
	move.b	2(a1),d4		; height
	moveq	#0,d2
	move.b	5(a1),d2
	lea	6(a1,d2.w),a2	; codes
	lea	(a2,d3.w),a5	; widths
	lea	SUI_GLYPH_CACHE_OFFSETS(a4),a0
	moveq	#0,d7
	moveq	#0,d6
.offset:
	cmp.w	d3,d6
	bge.s	.map
	move.w	d7,(a0)+
	moveq	#0,d5
	move.b	(a5,d6.w),d5
	addq.w	#3,d5
	and.w	#$fffc,d5
	mulu.w	d4,d5
	add.w	d5,d7		; original low-word offset accumulation
	addq.w	#1,d6
	bra.s	.offset
.map:
	lea	SUI_GLYPH_CACHE_MAP(a4),a0
	move.w	#255,d6
.absent:
	move.b	d3,(a0,d6.w)
	dbra	d6,.absent
	move.w	d3,d6
	moveq	#0,d5
.code:
	subq.w	#1,d6
	bmi.s	.filled
	move.b	(a2,d6.w),d5
	move.b	d6,(a0,d5.w)
	bra.s	.code
.filled:
	movem.l	(sp)+,d0-d5/d7/a0/a2/a5
.hit:
	movem.l	(sp)+,d5/d6
	rts

; GCC ABI: const unsigned char *slicks_font_glyph_map(const unsigned char *font)
slicks_font_glyph_map:
	move.l	a4,-(sp)
	movea.l	8(sp),a1
	bsr.w	sui_font_cache
	lea	SUI_GLYPH_CACHE_MAP(a4),a0
	move.l	a0,d0
	movea.l	(sp)+,a4
	rts

; Invalidate every cached font; called after any font resource is decoded.
sui_font_glyph_flush:
	lea	sui_glyph_cache(pc),a0
	moveq	#SUI_GLYPH_CACHE_ENTRIES-1,d0
.flush:
	clr.l	(a0)
	lea	SUI_GLYPH_CACHE_ENTRY(a0),a0
	dbra	d0,.flush
	rts

	cnop	0,4
sui_glyph_cache:
	ds.b	SUI_GLYPH_CACHE_ENTRIES*SUI_GLYPH_CACHE_ENTRY
sui_glyph_cache_next:
	ds.w	1
