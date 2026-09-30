#ifndef SLICKS_SAVED_GAME_H
#define SLICKS_SAVED_GAME_H

/* Match the original track-discovery caller's bound, not a native staging size. */
#define SLICKS_SAVED_GAME_TRACK_MAX 10000
#define SLICKS_SAVED_GAME_MAX_BYTES (6+8*SLICKS_SAVED_GAME_TRACK_MAX+4*53)

/* Original .SSS writer 1d587..1d769. This is not a raw memory snapshot:
 * word fields are big endian, names are bounded variable-length byte strings,
 * track names are exactly eight catalogue bytes (not paths or extensions).
 * The original does NOT serialize options, colour ramps or the RNG stream.
 * next_track is caller's current-track index plus one, with word wrapping. */
struct SlicksSavedGame {
    short track_count,next_track;
    const unsigned char (*tracks)[8];
    unsigned char names[4][21],position_scale[4];
    signed char vehicles[4],participation[4];
    short points[4],cash[4],inventory[4][13];
};

static inline long slicks_saved_game_size(const struct SlicksSavedGame *game)
{
    if(!game || game->track_count<0 || game->track_count>SLICKS_SAVED_GAME_TRACK_MAX ||
       (game->track_count && !game->tracks)) return -1;
    long size=6+8L*game->track_count;
    for(unsigned slot=0;slot<4;++slot) {
        unsigned length=0;
        /* Original emits a NUL when encountered before byte 20; a full
         * 20-byte name has no terminator in the file. */
        do { ++length; } while(length<20 && game->names[slot][length-1]);
        size+=length+33;
    }
    return size;
}
static inline void slicks_saved_game_word(unsigned char *out,short value)
{
    out[0]=(unsigned char)((unsigned short)value>>8);
    out[1]=(unsigned char)value;
}
/* Transactional memory encoding: insufficient capacity/invalid inputs leave
 * the destination untouched. Disk commit/short-write handling belongs to the
 * platform adapter, not the original's unchecked fputc return behavior. */
static inline long slicks_save_game_bytes(const struct SlicksSavedGame *game,
    unsigned char *out,unsigned long capacity)
{
    long size=slicks_saved_game_size(game);
    if(size<0 || !out || capacity<(unsigned long)size) return -1;
    unsigned at=0;
    out[at++]=0x53; out[at++]=8;
    slicks_saved_game_word(out+at,game->track_count); at+=2;
    for(unsigned i=0;i<(unsigned)game->track_count;++i)
        for(unsigned j=0;j<8;++j) out[at++]=game->tracks[i][j];
    slicks_saved_game_word(out+at,game->next_track); at+=2;
    for(unsigned slot=0;slot<4;++slot) {
        for(unsigned j=0;j<20;++j) {
            unsigned char value=game->names[slot][j]; out[at++]=value;
            if(!value) break;
        }
        out[at++]=(unsigned char)game->vehicles[slot];
        out[at++]=(unsigned char)game->participation[slot];
        out[at++]=game->position_scale[slot];
        slicks_saved_game_word(out+at,game->points[slot]); at+=2;
        slicks_saved_game_word(out+at,game->cash[slot]); at+=2;
        for(unsigned j=0;j<13;++j) { slicks_saved_game_word(out+at,game->inventory[slot][j]); at+=2; }
    }
    return (long)at;
}

/* Decode the original .SSS stream (reader 1d20c..1d586). This layer only
 * decodes stored values; catalogue/profile resolution and the original
 * out-of-range vehicle fallback belong to the resume boundary. Validate the
 * entire stream before publishing anything, unlike the DOS reader's partial
 * writes on failure. The caller owns the track-name storage. */
static inline int slicks_load_game_bytes(struct SlicksSavedGame *game,
    unsigned char (*tracks)[8],unsigned capacity,const unsigned char *bytes,unsigned long size)
{
    if(!game || !tracks || !bytes || size<6 || bytes[0]!=0x53 || bytes[1]!=8) return -1;
    unsigned count=((unsigned)bytes[2]<<8)|bytes[3];
    if(count>SLICKS_SAVED_GAME_TRACK_MAX || count>capacity || size<6UL+8UL*count) return -1;
    unsigned long at=6UL+8UL*count;
    for(unsigned slot=0;slot<4;++slot) {
        for(unsigned j=0;j<20;++j) {
            if(at>=size) return -1;
            if(!bytes[at++]) break;
        }
        if(size-at<33) return -1;
        at+=33;
    }
    if(at!=size) return -1;
    struct SlicksSavedGame decoded={0};
    decoded.track_count=(short)count; decoded.tracks=(const unsigned char (*)[8])tracks;
    at=4;
    for(unsigned i=0;i<count;++i) for(unsigned j=0;j<8;++j) tracks[i][j]=bytes[at++];
    decoded.next_track=(short)(((unsigned)bytes[at]<<8)|bytes[at+1]); at+=2;
    for(unsigned slot=0;slot<4;++slot) {
        for(unsigned j=0;j<20;++j) {
            decoded.names[slot][j]=bytes[at++];
            if(!decoded.names[slot][j]) break;
        }
        decoded.vehicles[slot]=(signed char)bytes[at++];
        decoded.participation[slot]=(signed char)bytes[at++];
        decoded.position_scale[slot]=bytes[at++];
        decoded.points[slot]=(short)(((unsigned)bytes[at]<<8)|bytes[at+1]); at+=2;
        decoded.cash[slot]=(short)(((unsigned)bytes[at]<<8)|bytes[at+1]); at+=2;
        for(unsigned j=0;j<13;++j) {
            decoded.inventory[slot][j]=(short)(((unsigned)bytes[at]<<8)|bytes[at+1]); at+=2;
        }
    }
    *game=decoded;
    return 0;
}
#endif
