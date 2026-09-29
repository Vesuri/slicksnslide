#ifndef SLICKS_LANGUAGE_TABLE_H
#define SLICKS_LANGUAGE_TABLE_H

/* Original 2b70a constructs /langN.txt from the persisted language byte.
 * The archive API uses names without the leading slash. Only the eight
 * supplied tables are accepted here; startup chooser/default policy is separate. */
static inline int slicks_language_resource(char out[10],unsigned language)
{
    if(!out || language<1 || language>8) return -1;
    const char name[]="lang1.txt";
    for(unsigned i=0;i<sizeof name;++i) out[i]=name[i];
    out[4]=(char)('0'+language);
    return 0;
}

/* Original 3601a text-table loader, with bounded caller-owned storage.
 * fgets reads at most 198 bytes. Lines containing '=' are concatenated
 * after CR truncation, then LF becomes NUL and byte AF becomes LF.
 * A line starting with '.' ends the resource; EOF also ends it. */
static inline int slicks_language_table_load(const unsigned char *source,
    unsigned size,unsigned char *table,unsigned capacity,unsigned *used)
{
    if(!source || !table || !used) return -1;
    for(unsigned pass=0;pass<2;++pass) {
        unsigned at=0,out=0;
        while(at<size) {
            unsigned start=at,end,has_equal=0;
            while(at<size && at-start<198) {
                unsigned char ch=source[at++];
                if(!ch) return -1;
                if(ch=='\n') break;
            }
            end=at;
            if(source[start]=='.') break;
            for(unsigned i=start;i<end;++i) if(source[i]=='=') has_equal=1;
            if(!has_equal) continue;
            for(unsigned i=start;i<end;++i) if(source[i]=='\r') { end=i; break; }
            if(out+end-start+2>capacity) return -1;
            for(unsigned i=start;i<end;++i) {
                unsigned char ch=source[i];
                if(pass) table[out]=ch=='\n'?0:ch==0xaf?'\n':ch;
                ++out;
            }
            if(pass) table[out]=0;
            ++out;
        }
        if(out>=capacity) return -1;
        if(pass) { table[out]=0; *used=out+1; }
    }
    return 0;
}

/* Original 36182/361e9: first case-sensitive PREFIX match, not exact key
 * matching. The value starts one byte beyond the supplied key length.
 * Preserve this behavior even for ambiguous prefix keys. Missing keys use
 * the caller's fallback (36227 uses the key itself as fallback). */
static inline const unsigned char *slicks_language_lookup(
    const unsigned char *table,unsigned size,const unsigned char *key,
    const unsigned char *fallback)
{
    if(!table || !key) return fallback;
    unsigned length=0; while(key[length]) ++length;
    for(unsigned at=0;at<size && table[at];) {
        unsigned end=at; while(end<size && table[end]) ++end;
        if(end==size) return fallback;
        unsigned matched=0;
        while(matched<length && at+matched<end && table[at+matched]==key[matched]) ++matched;
        if(matched==length) return at+length+1<size?table+at+length+1:fallback;
        at=end+1;
    }
    return fallback;
}
#endif
