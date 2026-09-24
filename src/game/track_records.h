#ifndef SLICKS_TRACK_RECORDS_H
#define SLICKS_TRACK_RECORDS_H

struct SlicksTrackRecords {
    unsigned char entries[11][29];
    unsigned short trailer;
};

/* 1a971..1a999 clears the working table before the confirmation prompt. */
static inline void slicks_clear_track_records(struct SlicksTrackRecords *records)
{
    for(unsigned i=0;i<11;++i) for(unsigned j=0;j<29;++j) records->entries[i][j]=0;
    records->trailer=0;
}

/* Record portion of 1a7ea..1a96a. Preserve the track header and all bytes
 * after the record block. Return 1 written, 0 old format, -1 invalid.
 * The original promotes record 1 into record 0 before writing when its
 * signed time is better, or record 0 has no valid time. */
static inline int slicks_write_track_records(unsigned char *data,unsigned long size,
    struct SlicksTrackRecords *records)
{
    if(!data || !records || size<6) return -1;
    if(data[5]!=2) return 0;
    if(size<363) return -1;
    short first=(short)(records->entries[0][20]|records->entries[0][21]<<8);
    short second=(short)(records->entries[1][20]|records->entries[1][21]<<8);
    if(second<first || first<=1)
        for(unsigned j=0;j<29;++j) records->entries[0][j]=records->entries[1][j];
    unsigned at=8; unsigned short total=0;
    for(unsigned i=0;i<11;++i) {
        unsigned short sum=0;
        for(unsigned j=0;j<29;++j) {
            unsigned value=records->entries[i][j]; data[at++]=(unsigned char)value;
            sum=(unsigned short)(sum+(value^0x7b)+3);
        }
        data[at++]=(unsigned char)(sum>>8); data[at++]=(unsigned char)sum;
        data[at++]=(unsigned char)(sum+0x66);
        total=(unsigned short)(total+((sum|0x24)^0x64));
    }
    data[at++]=(unsigned char)(records->trailer>>8);
    data[at++]=(unsigned char)records->trailer; data[at]=(unsigned char)total;
    return 1;
}

/* 1a62e..1a7e9. Payload words are little-endian; the stream-word helper
 * 36275 reads big-endian checksums. Return 1 loaded, 0 old format, -1 invalid.
 * Unlike the DOS stream, truncated native input is rejected explicitly. */
static inline int slicks_track_records(const unsigned char *data,
    unsigned long size, struct SlicksTrackRecords *out)
{
    unsigned long at=8;
    unsigned short total=0;
    unsigned positive=0;
    if(size<6) return -1;
    if(data[5]!=2) {
        for(unsigned i=0;i<11;++i) out->entries[i][20]=out->entries[i][21]=0;
        out->trailer=0;
        return 0;
    }
    if(size<8) return -1;
    for(unsigned i=0;i<11;++i) {
        unsigned short sum=0;
        if(size-at<31) return -1;
        for(unsigned j=0;j<29;++j) {
            unsigned value=data[at++];
            out->entries[i][j]=(unsigned char)value;
            sum=(unsigned short)(sum+(value^0x7b)+3);
        }
        out->entries[i][19]=0;
        unsigned time=out->entries[i][20]|out->entries[i][21]<<8;
        if((short)time<=2) out->entries[i][20]=out->entries[i][21]=0;
        else positive=1;
        unsigned stored=data[at]*256U+data[at+1]; at+=2;
        unsigned valid=stored==sum;
        if(valid) {
            if(at>=size) return -1;
            valid=data[at++]==(unsigned char)(sum+0x66);
        }
        /* Original condition is cumulative: a positive earlier record also
         * makes a later bad checksum fatal. A mismatched word skips marker. */
        if(!valid && positive) return -1;
        total=(unsigned short)(total+((sum|0x24)^0x64));
    }
    if(size-at<3) return -1;
    out->trailer=(unsigned short)(data[at]*256U+data[at+1]);
    if(data[at+2]!=(unsigned char)total && positive) return -1;
    return 1;
}
#endif
