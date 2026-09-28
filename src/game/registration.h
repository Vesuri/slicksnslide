#ifndef SLICKS_REGISTRATION_H
#define SLICKS_REGISTRATION_H

/* Original startup 1987:c3e0..c4fb. A key is never a build input. The caller
 * supplies binary file bytes; missing files retain the shareware state.
 * Invalid files must report an error, not silently unlock or continue.
 * Bound the original 60-byte name read: do not reproduce its out-of-bounds
 * strlen on malformed unterminated input. No key generation is provided. */
enum SlicksRegistrationResult {
    SLICKS_REGISTRATION_INVALID=-1,
    SLICKS_REGISTRATION_SHAREWARE=0,
    SLICKS_REGISTRATION_VALID=1
};
struct SlicksRegistration { unsigned char name[61]; };
static inline unsigned char slicks_registration_uppercase(unsigned char c)
{
    if(c>='a' && c<='z') c=(unsigned char)(c-32);
    if(c==0x84) c=0x8e;
    if(c==0x94) c=0x99;
    if(c==0x86) c=0x8f;
    return c;
}
static inline int slicks_registration_decode(struct SlicksRegistration *r,
    const unsigned char *bytes,unsigned long size)
{
    for(unsigned i=0;i<sizeof r->name;++i) r->name[i]=0;
    if(!bytes) return SLICKS_REGISTRATION_SHAREWARE;
    if(size && bytes[0]==0x9c) return SLICKS_REGISTRATION_SHAREWARE;
    unsigned n=0; unsigned short sum=0,complement=0;
    while(n<size && n<60 && bytes[n]) {
        short value=(short)(signed char)bytes[n++];
        sum=(unsigned short)(sum+value);
        complement=(unsigned short)(complement+300-value);
    }
    if(n==60 || n>=size || size-n<5 ||
       sum!=(unsigned short)((bytes[n+1]<<8)|bytes[n+2]) ||
       complement!=(unsigned short)((bytes[n+3]<<8)|bytes[n+4]))
        return SLICKS_REGISTRATION_INVALID;
    for(unsigned i=0;i<n;++i) {
        r->name[i]=slicks_registration_uppercase(bytes[i]);
    }
    return n?SLICKS_REGISTRATION_VALID:SLICKS_REGISTRATION_SHAREWARE;
}
#endif
