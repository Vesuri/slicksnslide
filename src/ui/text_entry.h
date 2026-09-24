#ifndef SLICKS_TEXT_ENTRY_H
#define SLICKS_TEXT_ENTRY_H

struct SlicksTextEntry { unsigned short position; unsigned char character; };

/* 2f753..2f784 with the caller's initial position=0. Preserve mode positions
 * after the existing text; caller requests the initial redraw. */
static inline int slicks_text_entry_begin(struct SlicksTextEntry *state,
    unsigned char *text,unsigned capacity,unsigned flags)
{
    if(!state || !text || !capacity) return -1;
    unsigned length=0;
    if(flags&512) {
        while(length<capacity && text[length]) ++length;
        if(length==capacity || length>32767) return -1;
    }
    state->position=(unsigned short)length;
    state->character=0;
    if(!(flags&512)) text[0]=0;
    return 0;
}

/* Original 2feaf character lookup. Caller supplies a validated runtime font. */
static inline int slicks_text_has_glyph(const unsigned char *font,unsigned char c)
{
    for(unsigned i=0;i<font[0];++i) if(font[6+font[5]+i]==c) return 1;
    return 0;
}

/* Character-state portion 2f92d..2faa7. Input is a DOS character, not a scan
 * code; platform keyboard translation/extended-key consumption is separate.
 * Return 0 continuing, 1 accepted, 2 cancelled. Buffer has limit+1 bytes and
 * position is in 0..limit; original profile-name caller uses limit=20,
 * flags=0x203 (including preserve-existing-text on entry).
 * VGA saved-under restoration and text/cursor redraw are caller operations. */
static inline unsigned slicks_text_entry_key(struct SlicksTextEntry *state,
    unsigned char *text,unsigned limit,unsigned flags,const unsigned char *font,
    unsigned char character)
{
    state->character=character;
    if(character==13) return 1;
    if(character==27) { text[0]=0; return 2; }
    if(character==8 && state->position>0) {
        text[--state->position]=0; character=0;
    }
    if(flags&32) {
        if(character>='a' && character<='z' && !slicks_text_has_glyph(font,character)) character-=32;
        if(character>='A' && character<='Z' && !slicks_text_has_glyph(font,character)) character+=32;
    }
    if((flags&2) && character>='a' && character<='z') character-=32;
    if((flags&4) && character>='A' && character<='Z') character+=32;
    if((flags&8) && character==' ') character=0;
    if((flags&1) && !slicks_text_has_glyph(font,character)) character=0;
    state->character=character;
    if(state->position<limit && character) {
        text[state->position++]=character; text[state->position]=0;
    }
    return 0;
}
#endif
