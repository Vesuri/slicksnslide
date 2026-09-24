#ifndef SLICKS_PLAYER_PROFILES_H
#define SLICKS_PLAYER_PROFILES_H
#include "profile_setup.h"

#define SLICKS_PROFILE_MAX 100
struct SlicksPlayerProfiles {
    struct SlicksSetupProfile setup[SLICKS_PROFILE_MAX];
    unsigned char names[SLICKS_PROFILE_MAX][21];
    short statistics[SLICKS_PROFILE_MAX][9];
    unsigned char setting[SLICKS_PROFILE_MAX];
    signed char coefficients[SLICKS_PROFILE_MAX][9];
    short extra_statistic[SLICKS_PROFILE_MAX]; /* tenth in-memory word, not serialized */
    unsigned char field_4b5e[SLICKS_PROFILE_MAX];
    short count;
};

/* Before PLR loading, C startup has cleared all profile storage except the
 * initialized six-byte RGB endpoint records at DS:01db. Supply those as typed
 * original data; never initialize profiles from unpacked BSS contents. */
static inline void slicks_player_profile_defaults(struct SlicksPlayerProfiles *out,
    const unsigned char colours[SLICKS_PROFILE_MAX][6])
{
    for(unsigned i=0;i<SLICKS_PROFILE_MAX;++i) {
        out->setup[i].flags=out->setup[i].vehicle=out->setting[i]=0;
        out->extra_statistic[i]=0; out->field_4b5e[i]=0;
        for(unsigned j=0;j<6;++j) out->setup[i].colours[j]=colours[i][j];
        for(unsigned j=0;j<21;++j) out->names[i][j]=0;
        for(unsigned j=0;j<9;++j) {
            out->statistics[i][j]=0;
            out->coefficients[i][j]=0;
        }
    }
    out->count=0;
}

/* 2ac53..2acea, called after the profile/config file readers. Only these
 * fields are overwritten; preserve other built-in fields and name tails.
 * Labels come from the caller's original/localized UI resources. */
static inline int slicks_set_builtin_profiles(struct SlicksPlayerProfiles *out,
    short vehicle_count,const char *const names[3])
{
    unsigned lengths[3];
    for(unsigned i=0;i<3;++i) {
        if(!names[i]) return -1;
        lengths[i]=0;
        while(lengths[i]<21 && names[i][lengths[i]]) ++lengths[i];
        if(lengths[i]==21) return -1;
    }
    for(unsigned i=0;i<3;++i)
        for(unsigned j=0;j<=lengths[i];++j) out->names[i][j]=(unsigned char)names[i][j];
    out->setup[0].flags=7; out->setup[1].flags=7; out->setup[2].flags=6;
    out->setup[1].vehicle=(unsigned char)((unsigned short)vehicle_count+1U);
    out->setup[2].vehicle=(unsigned char)vehicle_count;
    out->setting[1]=out->setting[2]=100;
    return 0;
}

/* 2b997..2bb6f: PLR records extend the three built-in profiles supplied by
 * the caller. Names are 21 raw bytes, not necessarily terminated. Returns
 * 1 for loaded records, 0 for absent/unrecognized data, -1 for truncation or
 * an out-of-capacity count. Invalid-size input leaves state unchanged;
 * this is a native safety guard, not the original's unchecked EOF behavior. */
static inline int slicks_load_player_profiles(struct SlicksPlayerProfiles *out,
    const unsigned char *data, unsigned long size)
{
    unsigned count=3;
    int recognized=data && size && data[0]==0x97;
    if(recognized) {
        if(size<3) return -1;
        unsigned extra=((unsigned)data[1]<<8)|data[2];
        /* DOS adds the serialized word to three with 16-bit wrapping.
         * Deleting built-ins can legitimately save -2/-1 => counts 1/2. */
        count=(unsigned short)(extra+3);
        unsigned records=count>3?count-3:0;
        if(count<1 || count>SLICKS_PROFILE_MAX || size<3UL+58UL*records) return -1;
        unsigned long at=3;
        for(unsigned i=3;i<count;++i) {
            for(unsigned j=0;j<21;++j) out->names[i][j]=data[at++];
            for(unsigned j=0;j<9;++j) {
                out->statistics[i][j]=(short)(((unsigned)data[at]<<8)|data[at+1]);
                at+=2;
            }
            ++at; /* Serialized byte consumed and discarded by ba62. */
            out->setting[i]=data[at++];
            for(unsigned j=0;j<9;++j)
                out->coefficients[i][j]=(signed char)(unsigned char)(data[at++]-40);
            out->setup[i].vehicle=data[at++]; out->setup[i].flags=data[at++];
            for(unsigned j=0;j<6;++j) out->setup[i].colours[j]=data[at++];
        }
    }
    out->count=(short)count;
    /* Includes built-in profiles, even when no PLR file was recognized. */
    for(unsigned i=0;i<count;++i)
        if(out->setting[i]<10) out->setting[i]=100;
    return recognized;
}

/* PLR portion of 2b097, at 2b2d8..2b474. Built-in profiles are not saved.
 * The reserved byte is always zero (the reader discards it). File I/O and
 * the caller's dirty flag remain outside this encoder. Reject invalid counts
 * and short output buffers before writing anything. */
static inline int slicks_save_player_profiles(const struct SlicksPlayerProfiles *p,
    unsigned char *data,unsigned long capacity)
{
    if(!p || !data || p->count<1 || p->count>SLICKS_PROFILE_MAX) return -1;
    unsigned extra=(unsigned short)(p->count-3);
    unsigned records=p->count>3?(unsigned)p->count-3:0;
    if(capacity<3UL+58UL*records) return -1;
    unsigned at=0;
    data[at++]=0x97; data[at++]=(unsigned char)(extra>>8); data[at++]=(unsigned char)extra;
    for(unsigned i=3;i<(unsigned)p->count;++i) {
        for(unsigned j=0;j<21;++j) data[at++]=p->names[i][j];
        for(unsigned j=0;j<9;++j) {
            unsigned value=(unsigned short)p->statistics[i][j];
            data[at++]=(unsigned char)(value>>8); data[at++]=(unsigned char)value;
        }
        data[at++]=0; data[at++]=p->setting[i];
        for(unsigned j=0;j<9;++j) data[at++]=(unsigned char)(p->coefficients[i][j]+40);
        data[at++]=p->setup[i].vehicle; data[at++]=p->setup[i].flags;
        for(unsigned j=0;j<6;++j) data[at++]=p->setup[i].colours[j];
    }
    return (int)at;
}

/* New-profile edit initialization 27a6d..27b05. The name is edited in a local
 * dialog buffer; this stage neither commits it nor increments profile count.
 * Preserve vehicle and all other fields not touched by the original. */
static inline int slicks_begin_new_profile(struct SlicksPlayerProfiles *p,
    unsigned index,unsigned long *random_state)
{
    if(!p || !random_state || index<1 || index>=SLICKS_PROFILE_MAX) return -1;
    for(unsigned i=0;i<9;++i) p->statistics[index][i]=0;
    p->extra_statistic[index]=0;
    for(unsigned i=0;i<6;++i) {
        *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
        p->setup[index].colours[i]=(unsigned char)((*random_state>>25)&63);
    }
    p->setup[index].flags=0;
    p->coefficients[index][0]=0;
    p->setting[index]=100;
    return 0;
}

/* Editor tail 28145..28195: 0 accepted, 1 cancelled/empty, -1 unsafe input.
 * Only the local name is deferred; cancellation does not roll back property
 * edits or new-profile initialization. The caller increments count on success.
 * Preserve bytes after the copied terminator, exactly like original strcpy. */
static inline int slicks_finish_profile_edit(struct SlicksPlayerProfiles *p,
    unsigned index,unsigned char is_new,signed char result,
    const unsigned char *name,unsigned long size)
{
    if(!p || index>=SLICKS_PROFILE_MAX) return -1;
    if(result<=0) return 1;
    if(!name || !size) return -1;
    if(!name[0]) return 1;
    unsigned length=0;
    while(length<21 && length<size && name[length]) ++length;
    if(length==21 || length==size) return -1;
    for(unsigned i=0;i<=length;++i) p->names[index][i]=name[i];
    if(is_new) {
        p->field_4b5e[index]=1;
        p->coefficients[index][0]=0;
    }
    return 0;
}

/* 2821e uses strcpy for both byte arrays. Do not reinterpret the nine-byte
 * field as a fixed-size arithmetic record here. Native bounds guards reject
 * malformed unterminated records before mutation. Setting is NOT copied. */
static inline int slicks_profile_copy_lengths(const struct SlicksPlayerProfiles *p,
    unsigned source,unsigned *name_length,unsigned *field_length)
{
    *name_length=*field_length=0;
    while(*name_length<21 && p->names[source][*name_length]) ++*name_length;
    while(*field_length<9 && p->coefficients[source][*field_length]) ++*field_length;
    return *name_length<21 && *field_length<9 ? 0 : -1;
}
static inline int slicks_copy_player_profile(struct SlicksPlayerProfiles *p,
    unsigned destination,unsigned source)
{
    unsigned name_length,field_length;
    if(!p || destination>=100 || source>=100 ||
        slicks_profile_copy_lengths(p,source,&name_length,&field_length)) return -1;
    for(unsigned i=0;i<=name_length;++i) p->names[destination][i]=p->names[source][i];
    for(unsigned i=0;i<=field_length;++i) p->coefficients[destination][i]=p->coefficients[source][i];
    for(unsigned i=0;i<9;++i) p->statistics[destination][i]=p->statistics[source][i];
    p->extra_statistic[destination]=p->extra_statistic[source];
    p->field_4b5e[destination]=p->field_4b5e[source];
    p->setup[destination]=p->setup[source];
    return 0;
}

/* Confirmed-delete stage 28c3d..28cbd. Confirmation and dirty state belong
 * to the menu caller. Original dispatch permits deleting slots 1 and 2;
 * slot zero cannot be selected by its delete-action result range.
 * The now-unused final record and each slot's setting remain untouched. */
static inline int slicks_delete_player_profile(struct SlicksPlayerProfiles *p,
    short selected[4],unsigned index)
{
    if(!p || !selected || p->count<2 || p->count>100 || index<1 || index>=(unsigned)p->count) return -1;
    for(unsigned i=index+1;i<(unsigned)p->count;++i) {
        unsigned a,b;
        if(slicks_profile_copy_lengths(p,i,&a,&b)) return -1;
    }
    --p->count;
    for(unsigned i=0;i<4;++i) {
        if(selected[i]==(short)index) selected[i]=0;
        else if(selected[i]>(short)index) --selected[i];
    }
    for(unsigned i=index;i<(unsigned)p->count;++i)
        (void)slicks_copy_player_profile(p,i,i+1);
    return 0;
}
#endif
