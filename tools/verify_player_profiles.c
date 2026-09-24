#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/player_profiles.h"
#include "../src/ui/profile_editor.h"
#include "../src/gen/setup_defaults.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Stream { unsigned char data[6000]; unsigned size,at; };
static void io(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Stream *s=p; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x11eaf) dx=0x9100;
    else if(address==0x12d8a) ax=s->at<s->size?s->data[s->at++]:65535;
    else if(address==0x123ce) {
        if(s->at>=sizeof s->data) abort();
        s->data[s->at++]=(unsigned char)readword(u,stack+4);
    }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static int same(uc_engine *u,unsigned at,const void *native,unsigned size)
{ unsigned char actual[2100]; if(size>sizeof actual) abort(); check(uc_mem_read(u,0x3cbf0+at,actual,size)); return !memcmp(actual,native,size); }
static int defaults(uc_engine *u)
{
    struct SlicksPlayerProfiles p;
    memset(&p,0xa5,sizeof p);
    unsigned char poison[0x7822-0x2fa4]; memset(poison,0xa5,sizeof poison);
    check(uc_mem_write(u,0x3cbf0+0x2fa4,poison,sizeof poison));
    uint16_t cs=0x1010;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_emu_start(u,0x10100,0x10108,0,100));
    check(uc_emu_start(u,0x101ae,0x101c0,0,100000));
    slicks_player_profile_defaults(&p,slicks_original_profile_colours);
    if(!same(u,0x36aa,p.names,sizeof p.names) || !same(u,0x3f42,p.setting,100) ||
        !same(u,0x47da,p.coefficients,sizeof p.coefficients) ||
        readword(u,0x3cbf0+0x36a8)!=(unsigned short)p.count) return 1;
    for(unsigned i=0;i<100;++i) {
        if(!same(u,0x3ede +i,&p.setup[i].flags,1) ||
            !same(u,0x3fa6+i,&p.setup[i].vehicle,1) ||
            !same(u,0x1db+6*i,p.setup[i].colours,6) ||
            !same(u,0x4b5e +i,&p.field_4b5e[i],1) ||
            readword(u,0x3cbf0+0x401c+20*i)!=(unsigned short)p.extra_statistic[i]) return 1;
        for(unsigned j=0;j<9;++j)
            if(readword(u,0x3cbf0+0x400a+20*i+2*j)!=(unsigned short)p.statistics[i][j]) return 1;
    }
    const unsigned labels[]={0x143a,0x1441,0x144a};
    for(unsigned i=0;i<3;++i)
        if(!same(u,labels[i],slicks_original_profile_labels[i],
            (unsigned)strlen(slicks_original_profile_labels[i])+1)) return 1;
    if(!same(u,0x433,slicks_original_fallback_colours,24) ||
        readword(u,0x3cbf0+0xf1a)!=(unsigned short)slicks_original_override_count) return 1;
    if(!same(u,0x1b4,slicks_original_vehicle_weights,10) ||
        !same(u,0x106f,slicks_original_item_flags,13)) return 1;
    puts("DOS profile defaults: all 100 typed records match original startup clear and initialized colours; generated labels match");
    return 0;
}
static int builtins(uc_engine *u)
{
    unsigned cases=0;
    static unsigned char initial[65536]; memset(initial,0xa5,sizeof initial);
    const unsigned source_offsets[]={0x143a,0x1441,0x144a};
    /* Put labels in a separate DS: original destination ES is fixed. */
    for(unsigned value=0;value<256;++value) for(unsigned variant=0;variant<3;++variant) {
        char labels[3][21]; const char *names[3]={labels[0],labels[1],labels[2]};
        struct SlicksPlayerProfiles p; memset(&p,0xa5,sizeof p);
        check(uc_mem_write(u,0x3cbf0,initial,sizeof initial));
        short vehicle_count=(short)(value+(variant==1?0x7f00:variant==2?0xff00:0));
        word(u,0x3cbf0+0x4c6e,(unsigned short)vehicle_count);
        for(unsigned i=0;i<3;++i) {
            /* Distinct non-overlapping source segments preserve the original
             * tightly packed pointers: use strings at most six bytes here. */
            unsigned length=variant*3;
            for(unsigned j=0;j<length;++j) labels[i][j]=(char)('A'+i+j);
            labels[i][length]=0;
            check(uc_mem_write(u,0x60000+source_offsets[i],labels[i],length+1));
        }
        if(slicks_set_builtin_profiles(&p,vehicle_count,names)) return 1;
        uint16_t cs=0x266c,ds=0x6000,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2ac53,0x70000,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || !same(u,0x36aa,p.names,sizeof p.names) ||
            !same(u,0x3f42,p.setting,100) || readword(u,0x3cbf0+0x36a8)!=(unsigned short)p.count) return 1;
        for(unsigned i=0;i<100;++i)
            if(!same(u,0x3ede +i,&p.setup[i].flags,1) ||
                !same(u,0x3fa6+i,&p.setup[i].vehicle,1) ||
                !same(u,0x1db+6*i,p.setup[i].colours,6)) return 1;
        ++cases;
    }
    printf("DOS built-in profiles: %u initializer comparisons pass, including preserved fields and byte wrapping\n",cases);
    return 0;
}
static void verify_save(uc_engine *u,struct Stream *stream)
{
    unsigned cases=0;
    for(int extra=-2;extra<=97;++extra) for(unsigned pattern=0;pattern<256;++pattern) {
        struct SlicksPlayerProfiles p;
        memset(&p,0,sizeof p); p.count=(short)(extra+3);
        for(unsigned i=0;i<100;++i) {
            for(unsigned j=0;j<21;++j) p.names[i][j]=(unsigned char)(pattern+37*i+11*j);
            for(unsigned j=0;j<9;++j) {
                p.statistics[i][j]=(short)(pattern*257U+31*i+71*j);
                p.coefficients[i][j]=(signed char)(pattern+i+37*j);
                word(u,0x3cbf0+0x400a+20*i+2*j,(unsigned short)p.statistics[i][j]);
            }
            p.setting[i]=(unsigned char)(pattern+i);
            p.setup[i].vehicle=(unsigned char)(pattern+3*i);
            p.setup[i].flags=(unsigned char)(pattern+7*i);
            for(unsigned j=0;j<6;++j) p.setup[i].colours[j]=(unsigned char)(pattern+i+17*j);
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&p.setup[i].vehicle,1));
            check(uc_mem_write(u,0x3cbf0+0x3ede +i,&p.setup[i].flags,1));
            check(uc_mem_write(u,0x3cbf0+0x1db+6*i,p.setup[i].colours,6));
        }
        check(uc_mem_write(u,0x3cbf0+0x36aa,p.names,sizeof p.names));
        check(uc_mem_write(u,0x3cbf0+0x3f42,p.setting,sizeof p.setting));
        check(uc_mem_write(u,0x3cbf0+0x47da,p.coefficients,sizeof p.coefficients));
        word(u,0x3cbf0+0x36a8,p.count);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xefe0,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        stream->at=0;
        check(uc_emu_start(u,0x2b2d8,0x2b474,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char encoded[6000]; memset(encoded,0xa5,sizeof encoded);
        unsigned size=3+58*(unsigned)(extra>0?extra:0);
        if(ip!=0x2b474-0x266c0 || sp!=0xefe0 || stream->at!=size ||
            slicks_save_player_profiles(&p,encoded,size)!=(int)size ||
            memcmp(encoded,stream->data,size)) {
            fprintf(stderr,"PLR save mismatch extra=%d pattern=%u bytes=%u\n",extra,pattern,stream->at);
            exit(1);
        }
        for(unsigned i=size;i<sizeof encoded;++i) if(encoded[i]!=0xa5) abort();
        if(!pattern) for(unsigned capacity=0;capacity<size;++capacity) {
            memset(encoded,0xa5,sizeof encoded);
            if(slicks_save_player_profiles(&p,encoded,capacity)!=-1) abort();
            for(unsigned i=0;i<sizeof encoded;++i) if(encoded[i]!=0xa5) abort();
        }
        ++cases;
    }
    struct SlicksPlayerProfiles p; memset(&p,0,sizeof p);
    unsigned char encoded[6000],expected[6000]; memset(expected,0xa5,sizeof expected);
    const short invalid[]={-32768,-1,0,101,32767};
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];++i) {
        p.count=invalid[i]; memcpy(encoded,expected,sizeof encoded);
        if(slicks_save_player_profiles(&p,encoded,sizeof encoded)!=-1 ||
            memcmp(encoded,expected,sizeof encoded)) abort();
    }
    printf("DOS profile save: %u complete byte-stream comparisons pass; all counts, byte patterns and short capacities covered\n",cases);
}

static void project_profiles(unsigned char *ds,const struct SlicksPlayerProfiles *p,const short selected[4])
{
    ds[0x36a8]=(unsigned char)p->count; ds[0x36a9]=(unsigned char)((unsigned short)p->count>>8);
    for(unsigned i=0;i<4;++i) {
        ds[0x44c+2*i]=(unsigned char)selected[i]; ds[0x44d+2*i]=(unsigned char)((unsigned short)selected[i]>>8);
    }
    memcpy(ds+0x36aa,p->names,sizeof p->names);
    memcpy(ds+0x47da,p->coefficients,sizeof p->coefficients);
    memcpy(ds+0x3f42,p->setting,100); memcpy(ds+0x4b5e,p->field_4b5e,100);
    for(unsigned i=0;i<100;++i) {
        ds[0x3ede +i]=p->setup[i].flags; ds[0x3fa6+i]=p->setup[i].vehicle;
        memcpy(ds+0x1db+6*i,p->setup[i].colours,6);
        for(unsigned j=0;j<10;++j) {
            unsigned value=(unsigned short)(j<9?p->statistics[i][j]:p->extra_statistic[i]);
            ds[0x400a+20*i+2*j]=(unsigned char)value;
            ds[0x400b+20*i+2*j]=(unsigned char)(value>>8);
        }
    }
}
static void verify_delete(uc_engine *u)
{
    static unsigned char expected[65536],actual[65536]; unsigned cases=0;
    for(unsigned count=2;count<=100;++count) for(unsigned choice=0;choice<5;++choice)
    for(unsigned pattern=0;pattern<9;++pattern) {
        struct SlicksPlayerProfiles p; memset(&p,0xa5,sizeof p); p.count=(short)count;
        unsigned index=choice==0?1:choice==1?2:choice==2?3:choice==3?(count+2)/2:count-1;
        if(index>=count) continue;
        short selected[4]={(short)index,(short)(index+1),-1,(short)(index-1)};
        for(unsigned i=0;i<100;++i) {
            for(unsigned j=0;j<21;++j) p.names[i][j]=(unsigned char)(1+(i+j)%254);
            p.names[i][(i+pattern)%21]=0;
            for(unsigned j=0;j<9;++j) {
                p.coefficients[i][j]=(signed char)(1+(i*3+j)%254);
                p.statistics[i][j]=(short)(i*257+j+pattern);
            }
            p.coefficients[i][(i+pattern)%9]=0;
            p.extra_statistic[i]=(short)(i*131+pattern);
            p.field_4b5e[i]=(unsigned char)(i+pattern);
            p.setting[i]=(unsigned char)(i*7+pattern);
            p.setup[i].flags=(unsigned char)(i+pattern);
            p.setup[i].vehicle=(unsigned char)(i*3+pattern);
            for(unsigned j=0;j<6;++j) p.setup[i].colours[j]=(unsigned char)(i+j+pattern);
        }
        memset(expected,0xa5,sizeof expected); project_profiles(expected,&p,selected);
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        word(u,0x8eff0,index);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x28c3d,0x28cbd,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(slicks_delete_player_profile(&p,selected,index)) abort();
        project_profiles(expected,&p,selected);
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip!=0x28cbd-0x266c0 || sp!=0xeb00 || memcmp(actual,expected,sizeof actual)) {
            fprintf(stderr,"Profile delete mismatch count=%u index=%u pattern=%u\n",count,index,pattern);
            for(unsigned i=0;i<65536;++i) if(actual[i]!=expected[i]) {
                fprintf(stderr,"DS:%04x DOS=%02x native=%02x\n",i,actual[i],expected[i]); break;
            }
            exit(1);
        }
        ++cases;
    }
    struct SlicksPlayerProfiles p,before; short selected[4]={3,4,5,6},saved[4];
    memset(&p,0,sizeof p); p.count=7; memset(p.coefficients[6],1,9);
    before=p; memcpy(saved,selected,sizeof saved);
    if(slicks_delete_player_profile(&p,selected,3)!=-1 || memcmp(&p,&before,sizeof p) ||
        memcmp(selected,saved,sizeof saved)) abort();
    puts("Malformed profile deletion rejected atomically before shifting earlier records");
    printf("DOS profile deletion: %u full-DS comparisons pass, including original string copies, preserved tails and slot settings\n",cases);
}

static void verify_new(uc_engine *u)
{
    static unsigned char expected[65536],actual[65536]; unsigned cases=0;
    const unsigned indices[]={1,2,3,50,99};
    for(unsigned pattern=0;pattern<256;++pattern) for(unsigned n=0;n<5;++n) {
        struct SlicksPlayerProfiles p; memset(&p,(unsigned char)pattern,sizeof p);
        p.count=3; short selected[4]={0,1,2,1};
        unsigned long seed=(pattern*0x9e3779b9UL+n)&0xffffffffUL;
        memset(expected,0xa5,sizeof expected); project_profiles(expected,&p,selected);
        for(unsigned i=0;i<4;++i) expected[0x2aaa+i]=(unsigned char)(seed>>(8*i));
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        word(u,0x8f006,indices[n]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x27a6d,0x27b05,0,100000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(slicks_begin_new_profile(&p,indices[n],&seed)) abort();
        project_profiles(expected,&p,selected);
        for(unsigned i=0;i<4;++i) expected[0x2aaa+i]=(unsigned char)(seed>>(8*i));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip!=0x27b05-0x266c0 || sp!=0xeb00 || memcmp(expected,actual,sizeof actual)) {
            fprintf(stderr,"New profile mismatch pattern=%u index=%u\n",pattern,indices[n]);
            for(unsigned i=0;i<65536;++i) if(actual[i]!=expected[i]) {
                fprintf(stderr,"DS:%04x DOS=%02x native=%02x\n",i,actual[i],expected[i]); break;
            }
            exit(1);
        }
        ++cases;
    }
    printf("DOS new profile: %u full-DS/RNG comparisons pass, including preserved vehicle/name and string tails\n",cases);
}

static void verify_finish_edit(uc_engine *u)
{
    static unsigned char expected[65536],actual[65536]; unsigned cases=0;
    const signed char results[]={-128,-1,0,1,127};
    const unsigned indices[]={3,50,99};
    for(unsigned length=0;length<=20;++length) for(unsigned n=0;n<3;++n)
    for(unsigned r=0;r<5;++r) for(unsigned create=0;create<2;++create) {
        struct SlicksPlayerProfiles p; memset(&p,0xa5,sizeof p); p.count=7;
        short selected[4]={0,1,2,3}; unsigned char name[21];
        for(unsigned i=0;i<21;++i) name[i]=(unsigned char)(i+1+length);
        name[length]=0;
        memset(expected,0xa5,sizeof expected); project_profiles(expected,&p,selected);
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        check(uc_mem_write(u,0x8efd8,name,sizeof name));
        check(uc_mem_write(u,0x8effa,&results[r],1));
        word(u,0x8f006,indices[n]); word(u,0x8f008,create?255:0);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x28145,0x28195,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        int result=slicks_finish_profile_edit(&p,indices[n],create?255:0,results[r],name,sizeof name);
        project_profiles(expected,&p,selected);
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip!=0x28195-0x266c0 || sp!=0xeb00 || (ax&255)!=(unsigned)result || memcmp(expected,actual,sizeof actual)) {
            fprintf(stderr,"Editor commit mismatch length=%u index=%u result=%d new=%u\n",length,indices[n],results[r],create); exit(1);
        }
        ++cases;
    }
    struct SlicksPlayerProfiles p,before; unsigned char name[21];
    memset(&p,0xa5,sizeof p); before=p; memset(name,'X',sizeof name);
    for(unsigned size=0;size<=21;++size)
        if(slicks_finish_profile_edit(&p,3,1,1,name,size)!=-1 || memcmp(&p,&before,sizeof p)) abort();
    printf("DOS profile editor tail: %u full-DS/return comparisons pass; unterminated names rejected atomically\n",cases);
}

static void verify_editor_controls(uc_engine *u)
{
    static unsigned char expected[65536],actual[65536]; unsigned cases=0;
    const short counts[]={10,0,127,32767};
    for(unsigned pattern=0;pattern<4;++pattern) for(unsigned row=0;row<6;++row)
    for(unsigned scan=0;scan<256;++scan) {
        if((scan==0x1c || scan==0x1d || scan==0x39) && (row==0 || row==3 || row==4)) continue;
        struct SlicksPlayerProfiles p; memset(&p,0xa5,sizeof p); p.count=7;
        short selected[4]={0,1,2,3}; unsigned index=3+pattern*31;
        struct SlicksProfileEditor e={(unsigned char)row,0,0};
        p.setting[index]=(unsigned char)(pattern==0?0:pattern==1?255:pattern==2?50:150);
        p.setup[index].vehicle=(unsigned char)(pattern==0?0:pattern==1?255:pattern==2?127:11);
        memset(expected,0xa5,sizeof expected); project_profiles(expected,&p,selected);
        expected[0x4c6e]=(unsigned char)counts[pattern]; expected[0x4c6f]=(unsigned char)((unsigned short)counts[pattern]>>8);
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        check(uc_mem_write(u,0x8effc,&e.row,1)); check(uc_mem_write(u,0x8effb,&e.redraw,1));
        check(uc_mem_write(u,0x8effa,&e.result,1)); word(u,0x8f006,index);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip,ax=scan;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        check(uc_emu_start(u,0x27f5b,0x28120,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(slicks_profile_editor_key(&e,&p,index,counts[pattern],(unsigned char)scan)!=SLICKS_PROFILE_EDIT_NONE) abort();
        project_profiles(expected,&p,selected);
        unsigned char state[3];
        check(uc_mem_read(u,0x8effc,state,1)); check(uc_mem_read(u,0x8effb,state+1,1));
        check(uc_mem_read(u,0x8effa,state+2,1)); check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip!=0x28120-0x266c0 || sp!=0xeb00 || memcmp(state,&e,3) || memcmp(expected,actual,sizeof actual)) {
            fprintf(stderr,"Editor key mismatch pattern=%u row=%u scan=%u\n",pattern,row,scan); exit(1);
        }
        ++cases;
    }
    printf("DOS profile editor keys: %u non-modal full-DS/state comparisons pass\n",cases);
    cases=0;
    for(unsigned value=0;value<256;++value) for(unsigned flags=0;flags<2;++flags)
    for(unsigned global=0;global<2;++global) {
        struct SlicksPlayerProfiles p; memset(&p,0xa5,sizeof p); p.count=7;
        short selected[4]={0,1,2,3}; unsigned index=50;
        p.setting[index]=(unsigned char)value; p.setup[index].flags=(unsigned char)(0xfe|flags);
        memset(expected,0xa5,sizeof expected); project_profiles(expected,&p,selected);
        expected[0x1a6]=global?255:0;
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected)); word(u,0x8f006,index);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x27b92,0x27bea,0,1000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        slicks_profile_editor_limits(&p,index,global?255:0); project_profiles(expected,&p,selected);
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip!=0x27bea-0x266c0 || memcmp(expected,actual,sizeof actual)) abort();
        ++cases;
    }
    printf("DOS profile editor limits: %u full-DS comparisons pass\n",cases);
}

int main(int argc,char **argv)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    if(defaults(u)) return 1;
    if(argc==2 && !strcmp(argv[1],"--lifecycle")) {
        verify_delete(u); verify_new(u); verify_finish_edit(u); verify_editor_controls(u); check(uc_close(u)); return 0;
    }
    struct Stream stream; const unsigned hooks[]={0x11eaf,0x12d8a,0x119c2,0x123ce};
    for(unsigned i=0;i<4;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,io,&stream,hooks[i],hooks[i])); }
    static unsigned char initial[65536]; memset(initial,0xa5,sizeof initial);
    unsigned cases=0;
    for(int extra=-2;extra<=97;++extra)
    for(unsigned variant=0;variant<8;++variant) {
        stream.size=3+58*(unsigned)(extra>0?extra:0); stream.at=0;
        for(unsigned at=0;at<stream.size;++at) stream.data[at]=(unsigned char)(at*37+variant*31);
        stream.data[0]=variant==7?0:0x97; stream.data[1]=(unsigned char)((unsigned short)extra>>8); stream.data[2]=(unsigned char)extra;
        struct SlicksPlayerProfiles profiles; memset(&profiles,0xa5,sizeof profiles);
        for(unsigned i=0;i<3;++i) profiles.setting[i]=(unsigned char)(variant+i*5);
        check(uc_mem_write(u,0x3cbf0,initial,sizeof initial));
        check(uc_mem_write(u,0x3cbf0+0x3f42,profiles.setting,100));
        int result=slicks_load_player_profiles(&profiles,stream.data,stream.size);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2b997,0x70000,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || result!=(variant!=7) ||
            readword(u,0x3cbf0+0x36a8)!=(unsigned)profiles.count ||
            stream.at!=(variant==7?1:stream.size) ||
            !same(u,0x36aa,profiles.names,sizeof profiles.names) ||
            !same(u,0x3f42,profiles.setting,100) ||
            !same(u,0x47da,profiles.coefficients,sizeof profiles.coefficients)) goto mismatch;
        for(unsigned i=0;i<100;++i) {
            if(!same(u,0x3ede +i,&profiles.setup[i].flags,1) ||
                !same(u,0x3fa6+i,&profiles.setup[i].vehicle,1) ||
                !same(u,0x1db+6*i,profiles.setup[i].colours,6)) goto mismatch;
            for(unsigned j=0;j<9;++j)
                if(readword(u,0x3cbf0+0x400a+20*i+2*j)!=(unsigned short)profiles.statistics[i][j]) goto mismatch;
        }
        ++cases; continue;
mismatch:
        fprintf(stderr,"PLR mismatch extra=%d variant=%u read=%u/%u\n",extra,variant,stream.at,stream.size); return 1;
    }
    struct SlicksPlayerProfiles before,after; memset(&before,0x55,sizeof before);
    stream.data[0]=0x97; stream.data[1]=0; stream.data[2]=97;
    for(unsigned size=1;size<3+58*97;++size) {
        after=before;
        if(slicks_load_player_profiles(&after,stream.data,size)!=-1 || memcmp(&after,&before,sizeof before)) return 1;
    }
    stream.data[2]=98; after=before;
    if(slicks_load_player_profiles(&after,stream.data,sizeof stream.data)!=-1 ||
        memcmp(&after,&before,sizeof before)) return 1;
    after=before; after.setting[0]=0; after.setting[1]=9; after.setting[2]=10;
    if(slicks_load_player_profiles(&after,0,0)!=0 || after.count!=3 ||
        after.setting[0]!=100 || after.setting[1]!=100 || after.setting[2]!=10) return 1;
    if(builtins(u)) return 1;
    verify_save(u,&stream);
    verify_delete(u);
    verify_new(u);
    verify_finish_edit(u);
    verify_editor_controls(u);
    check(uc_close(u));
    printf("DOS player profiles: %u full-loader comparisons pass; all nonempty truncated maximum-size prefixes rejected atomically\n",cases);
    return 0;
}
