/* Execute the original 286 expiry block, not a transcription of its result.
 * Scope: expiry, retirement decisions, and real point-actor slot release.
 * The redraw service is replaced with RETF; rendered pixels are not tested. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

static void check(uc_err error)
{
    if (error) { fprintf(stderr, "%s\n", uc_strerror(error)); exit(1); }
}
static void word(uc_engine *uc, unsigned address, unsigned value)
{
    uint8_t bytes[2] = {value, value >> 8};
    check(uc_mem_write(uc, address, bytes, 2));
}
struct Calls { unsigned draw, release; };
static void allocation_stop(uc_engine *uc, uint64_t address, uint32_t size, void *opaque)
{
    (void)size; (void)opaque;
    if (address == 0x13114 || address == 0x132a6) check(uc_emu_stop(uc));
}
static void verify_allocation(const unsigned char *segment)
{
    unsigned cases = 0;
    /* Five slots can independently be free, active, or retiring. Four
     * persistent car slots precede them; capacity is either full or grows. */
    for (unsigned pattern = 0; pattern < 243; ++pattern)
        for (unsigned capacity = 10; capacity <= 11; ++capacity) {
            uc_engine *uc;
            uc_hook hook;
            uint16_t cs=0x1000, ds=0x2000, ss=0x4000, bp=0x800, sp=0x700, ip;
            unsigned char actors[640] = {0}, actual[640], selected[2], high[2];
            unsigned digits=pattern, expected=0;
            for (unsigned slot=1; slot<10; ++slot) {
                unsigned kind = slot < 5 ? 1 : digits % 3;
                if (slot >= 5) digits /= 3;
                actors[slot*64+0x1a] = kind == 0 ? 0 : kind == 1 ? 2 : 251;
                if (!kind && !expected) expected=slot;
            }
            unsigned high_expected=10;
            if (!expected && capacity>10) { expected=10; high_expected=11; }
            check(uc_open(UC_ARCH_X86, UC_MODE_16, &uc));
            check(uc_mem_map(uc, 0, 0x100000, UC_PROT_ALL));
            check(uc_mem_write(uc, 0x10000, segment, 0x10000));
            check(uc_mem_write(uc, 0x30000, actors, sizeof actors));
            check(uc_reg_write(uc, UC_X86_REG_CS, &cs));
            check(uc_reg_write(uc, UC_X86_REG_DS, &ds));
            check(uc_reg_write(uc, UC_X86_REG_SS, &ss));
            check(uc_reg_write(uc, UC_X86_REG_BP, &bp));
            check(uc_reg_write(uc, UC_X86_REG_SP, &sp));
            word(uc, 0x40806, 1); /* Non-null source, before actor setup. */
            word(uc, 0x216ce, 0); word(uc, 0x216d0, 0x3000);
            word(uc, 0x216be, capacity); word(uc, 0x216c0, 10);
            check(uc_hook_add(uc, &hook, UC_HOOK_CODE, allocation_stop, NULL, 1, 0));
            check(uc_emu_start(uc, 0x130c4, 0, 0, 500));
            check(uc_reg_read(uc, UC_X86_REG_IP, &ip));
            check(uc_mem_read(uc, 0x407fe, selected, 2));
            check(uc_mem_read(uc, 0x216c0, high, 2));
            check(uc_mem_read(uc, 0x30000, actual, sizeof actual));
            if (ip != (expected ? 0x3114 : 0x32a6) ||
                (unsigned)(selected[0] | selected[1]<<8) != expected ||
                (unsigned)(high[0] | high[1]<<8) != high_expected ||
                memcmp(actors, actual, sizeof actors)) {
                fprintf(stderr, "allocation mismatch pattern=%u capacity=%u\n", pattern, capacity);
                exit(1);
            }
            check(uc_close(uc));
            ++cases;
        }
    printf("DOS 286 allocation: %u cases passed; lowest zero-state slot reused, retiring slots retained, full/growing capacity checked\n", cases);
}
static void calls(uc_engine *uc, uint64_t address, uint32_t size, void *opaque)
{
    struct Calls *counts = opaque;
    (void)uc; (void)size;
    counts->draw += address == 0x13673;
    counts->release += address == 0x135a5;
}

static unsigned readword(uc_engine *uc, unsigned address)
{
    unsigned char bytes[2];
    check(uc_mem_read(uc,address,bytes,2));
    return bytes[0] | bytes[1]<<8;
}

/* Execute both real far-call boundaries, including the allocator. The pool is
 * preallocated, as in a running race; no allocator/constructor/update stubs. */
static void verify_point_creation(const unsigned char *segment)
{
    unsigned cases=0;
    for(unsigned colour=0;colour<256;++colour)
        for(unsigned layer=0;layer<2;++layer)
            for(unsigned variant=0;variant<2;++variant)
                for(unsigned full=0;full<2;++full) {
                    uc_engine *uc;
                    uint16_t cs=0x1000,ds=0x2000,ss=0x4000,sp=0x800,ax,ip;
                    unsigned char actors[192]={0},actual[192];
                    unsigned life=variant?35:50;
                    unsigned args[13]={1,319,189,0x8000,0x7fff,0,0,0,0,life,layer*15,1,3};
                    actors[64+0x3a]=1;
                    actors[64+0x1a]=full?3:0;
                    actors[128+0x1a]=3;
                    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
                    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
                    check(uc_mem_write(uc,0x10000,segment,0x10000));
                    check(uc_mem_write(uc,0x30000,actors,sizeof actors));
                    check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
                    check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
                    check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
                    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
                    word(uc,0x216ce,0); word(uc,0x216d0,0x3000);
                    word(uc,0x216be,3); word(uc,0x216c0,3);
                    word(uc,0x40800,0); word(uc,0x40802,0x7000);
                    word(uc,0x40804,(unsigned)(int)(int8_t)colour);
                    check(uc_emu_start(uc,0x132ad,0x70000,0,2000));
                    check(uc_reg_read(uc,UC_X86_REG_AX,&ax));
                    check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
                    check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
                    if(ax!=(full?0:1) || ip || sp!=0x804) {
                        fprintf(stderr,"point constructor return mismatch colour=%u full=%u\n",colour,full);
                        exit(1);
                    }
                    if(!full) {
                        check(uc_mem_read(uc,0x30000,actual,sizeof actual));
                        if(actual[64+0x26]!=colour || actual[64+0x1a]!=1 ||
                           readword(uc,0x30044)!=320 || readword(uc,0x30046)!=320 ||
                           readword(uc,0x30050)!=256 || readword(uc,0x30052)!=0 ||
                           readword(uc,0x30054)!=256 || readword(uc,0x30056)!=0 ||
                           readword(uc,0x30058)!=0) {
                            fprintf(stderr,"point constructor record mismatch colour=%u\n",colour);
                            exit(1);
                        }
                        sp=0x800;
                        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
                        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
                        for(unsigned i=0;i<13;++i) word(uc,0x40804+2*i,args[i]);
                        check(uc_emu_start(uc,0x12ef2,0x70000,0,1000));
                        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
                        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
                        check(uc_mem_read(uc,0x30000,actual,sizeof actual));
                        if(ip || sp!=0x804 || readword(uc,0x30040)!=319*64 ||
                           readword(uc,0x30042)!=189*64 ||
                           readword(uc,0x3005b)!=0x8000 || readword(uc,0x3005d)!=0x7fff ||
                           readword(uc,0x3007e)!=life || actual[64+0x3b]!=layer*15 ||
                           actual[64+0x1a]!=1 || actual[64+0x25]!=3 ||
                           actual[64+0x26]!=colour || actual[64+0x24]!=0) {
                            fprintf(stderr,"point update mismatch colour=%u layer=%u variant=%u\n",colour,layer,variant);
                            exit(1);
                        }
                        /* First movement pass: real signed-word ADD and SAR,
                         * with both extreme velocities and both page slots.
                         * Stop before animation and rendering. */
                        {
                            uint16_t bp=0x900;
                            int x=(int16_t)(319*64+0x8000);
                            int y=(int16_t)(189*64+0x7fff);
                            unsigned char page=layer;
                            check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
                            check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
                            word(uc,0x408fe,1);
                            check(uc_mem_write(uc,0x216c6,&page,1));
                            check(uc_emu_start(uc,0x1394f,0x139d2,0,200));
                            check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
                            check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
                            if(ip!=0x39d2 || sp!=0x804 ||
                               readword(uc,0x30040)!=(uint16_t)x ||
                               readword(uc,0x30042)!=(uint16_t)y ||
                               readword(uc,0x30044+2*page)!=(uint16_t)(x<0?-((63-x)/64):x/64) ||
                               readword(uc,0x30048+2*page)!=(uint16_t)(y<0?-((63-y)/64):y/64)) {
                                fprintf(stderr,"point movement mismatch colour=%u page=%u\n",colour,page);
                                exit(1);
                            }
                            check(uc_mem_read(uc,0x30000,actual,sizeof actual));
                        }
                    } else check(uc_mem_read(uc,0x30000,actual,sizeof actual));
                    if(memcmp(actors,actual,64) || memcmp(actors+128,actual+128,64) ||
                       (full && memcmp(actors,actual,sizeof actors))) {
                        fprintf(stderr,"point creation altered another/full slot\n"); exit(1);
                    }
                    check(uc_close(uc)); ++cases;
                }
    printf("DOS point creation: %u real constructor/allocator/update cases passed; all colours, both burst variants/layers, full pool, balanced far returns; 1024 first-movement/page-coordinate cases (pixels not tested)\n",cases);
}

static void coordinate_stop(uc_engine *uc,uint64_t address,uint32_t size,void *opaque)
{
    (void)address;(void)size;(void)opaque;check(uc_emu_stop(uc));
}

static void verify_point_coordinate_words(const unsigned char *segment)
{
    uc_engine *uc;uc_hook hook;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10000,segment,0x10000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,coordinate_stop,0,0x139d2,0x139d2));
    word(uc,0x216ce,0);word(uc,0x216d0,0x3000);
    word(uc,0x216be,3);word(uc,0x216c0,3);
    unsigned char before[192],after[192];memset(before,0xa5,sizeof before);
    memset(before+64,0,64);before[64+0x3a]=1;before[64+0x1a]=1;
    for(unsigned x=0;x<65536;++x) for(unsigned page=0;page<2;++page) {
        /* Odd multipliers enumerate every word on both axes and velocities. */
        unsigned y=(x*40503U+79U)&65535U;
        unsigned vx=(x*73U+0x8000U)&65535U,vy=(x*151U+0x7fffU)&65535U;
        unsigned args[13]={1,x,y,vx,vy,0,0,0,0,3,0,1,3};
        uint16_t cs=0x1000,ds=0x2000,ss=0x4000,sp=0x800,bp=0x900,ip;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_mem_write(uc,0x30000,before,sizeof before));
        word(uc,0x40800,0);word(uc,0x40802,0x7000);
        for(unsigned i=0;i<13;++i)word(uc,0x40804+2*i,args[i]);
        check(uc_emu_start(uc,0x12ef2,0x70000,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs));check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        if(cs!=0x7000 || ip || sp!=0x804 ||
           readword(uc,0x30040)!=((x<<6)&65535U) ||
           readword(uc,0x30042)!=((y<<6)&65535U)) {
            fprintf(stderr,"Coordinate constructor mismatch x=%u y=%u page=%u cs:ip=%x:%x sp=%x actual=%u,%u\n",x,y,page,cs,ip,sp,readword(uc,0x30040),readword(uc,0x30042));exit(1);
        }
        cs=0x1000;check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        word(uc,0x408fe,1);unsigned char page_byte=(unsigned char)page;
        check(uc_mem_write(uc,0x216c6,&page_byte,1));
        check(uc_emu_start(uc,0x1394f,0x139d2,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        int xx=(int16_t)((x<<6)+vx),yy=(int16_t)((y<<6)+vy);
        int px=xx<0?-((63-xx)/64):xx/64,py=yy<0?-((63-yy)/64):yy/64;
        check(uc_mem_read(uc,0x30000,after,sizeof after));
        if(ip!=0x39d2 || sp!=0x804 || readword(uc,0x30040)!=(uint16_t)xx ||
           readword(uc,0x30042)!=(uint16_t)yy ||
           readword(uc,0x30044+2*page)!=(uint16_t)px ||
           readword(uc,0x30048+2*page)!=(uint16_t)py ||
           memcmp(before,after,64) || memcmp(before+128,after+128,64)) {
            fprintf(stderr,"Coordinate first-move mismatch x=%u y=%u page=%u\n",x,y,page);exit(1);
        }
    }
    check(uc_close(uc));
    puts("DOS coordinate boundary: 131072 real constructor/first-move cases; all input words on both axes, both pages, wrapped velocities, arithmetic pixel rounding and adjacent-slot guards pass");
}

static void active_draw_stop(uc_engine *uc,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;(void)opaque;
    if(address==0x13c2a || address==0x13dad)check(uc_emu_stop(uc));
}

static void verify_active_draw(const unsigned char *segment)
{
    static const unsigned char gate[]={0x26,0x8a,0x47,0x1a,0x98,0x0b,0xc0,0x7f,0x03,0xe9,0x83,0x01};
    if(memcmp(segment+0x3c1e,gate,sizeof gate)) {
        fprintf(stderr,"Original active draw gate signature mismatch\n");exit(1);
    }
    uc_engine *uc;uc_hook hook;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10000,segment,0x10000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,active_draw_stop,0,1,0));
    for(unsigned raw=0;raw<256;++raw) {
        uint16_t cs=0x1000,es=0x3000,bx=64,ip;
        unsigned char state=(unsigned char)raw;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));check(uc_reg_write(uc,UC_X86_REG_ES,&es));
        check(uc_reg_write(uc,UC_X86_REG_BX,&bx));check(uc_mem_write(uc,0x3005a,&state,1));
        check(uc_emu_start(uc,0x13c1e,0x15000,0,20));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        if(ip!=((int8_t)state>0?0x3c2a:0x3dad)) {
            fprintf(stderr,"Original active draw state=%d ip=%x\n",(int8_t)state,ip);exit(1);
        }
    }
    check(uc_close(uc));puts("DOS active drawing: all 256 state bytes; only signed-positive slots enter the drawing path");
}

static void verify_retirement(const unsigned char *segment)
{
    for (unsigned raw = 0; raw < 256; ++raw) {
        uc_engine *uc;
        uc_hook hook;
        struct Calls counts = {0};
        uint16_t cs=0x1000, ds=0x2000, ss=0x4000;
        uint16_t bp=0x800, sp=0x700, ip, final_sp;
        unsigned char value=raw, result, retf=0xcb;
        int state = (int8_t)raw;
        unsigned expected_draw=0, expected_release=0;
        if (state < 0) {
            state = (int8_t)(state - 1);
            expected_draw = state == -6 || state == -7;
            expected_release = state == -3 || state == -7;
            if (expected_release) state = 0;
            else if (state == -4) state = -3;
        }
        check(uc_open(UC_ARCH_X86, UC_MODE_16, &uc));
        check(uc_mem_map(uc, 0, 0x100000, UC_PROT_ALL));
        check(uc_mem_write(uc, 0x10000, segment, 0x10000));
        check(uc_mem_write(uc, 0x13673, &retf, 1));
        check(uc_reg_write(uc, UC_X86_REG_CS, &cs));
        check(uc_reg_write(uc, UC_X86_REG_DS, &ds));
        check(uc_reg_write(uc, UC_X86_REG_SS, &ss));
        check(uc_reg_write(uc, UC_X86_REG_BP, &bp));
        check(uc_reg_write(uc, UC_X86_REG_SP, &sp));
        word(uc, 0x407fe, 1);
        word(uc, 0x216ce, 0); word(uc, 0x216d0, 0x3000);
        word(uc, 0x216be, 256);
        check(uc_mem_write(uc, 0x3005a, &value, 1));
        check(uc_hook_add(uc, &hook, UC_HOOK_CODE, calls, &counts, 1, 0));
        check(uc_emu_start(uc, 0x13fa7, 0x14021, 0, 300));
        check(uc_reg_read(uc, UC_X86_REG_IP, &ip));
        check(uc_reg_read(uc, UC_X86_REG_SP, &final_sp));
        check(uc_mem_read(uc, 0x3005a, &result, 1));
        if (ip != 0x4021 || final_sp != sp || result != (unsigned char)state ||
            counts.draw != expected_draw || counts.release != expected_release) {
            fprintf(stderr, "retirement mismatch initial=%d state=%d draw=%u free=%u ip=%x sp=%x\n",
                    (int8_t)raw, (int8_t)result, counts.draw, counts.release, ip, final_sp);
            exit(1);
        }
        check(uc_close(uc));
    }
    puts("DOS 286 retirement: all 256 state bytes matched; real point-slot release and balanced stack verified (redraw stubbed)");
}
static void verify_shadow_sequence(const unsigned char *segment)
{
    /* Run real expiry plus retirement on a reserved state-3 shadow slot.
     * Expiry is at 3918, BEFORE drawing/page toggle at 3e0a. */
    for(unsigned initial_page=0; initial_page<2; ++initial_page) {
        uc_engine *uc;
        uint16_t cs=0x1000,ds=0x2000,es=0x3000,ss=0x4000,bp=0x800,sp=0x700,bx=64;
        unsigned state=3,life=3,page=initial_page;
        check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
        check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(uc,0x10000,segment,0x10000));
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x407fe,1);
        word(uc,0x216ce,0); word(uc,0x216d0,0x3000);
        for(unsigned frame=0;frame<12;++frame) {
            /* Reactivate the SAME slot after several retired updates. */
            if(frame==7) { state=3; life=3; }
            unsigned char value=(unsigned char)state,result;
            unsigned char life_bytes[2];
            check(uc_mem_write(uc,0x3005a,&value,1));
            word(uc,0x3007e,life);
            value=page; check(uc_mem_write(uc,0x216c6,&value,1));
            unsigned before=state;
            if((int8_t)state>0) {
                check(uc_reg_write(uc,UC_X86_REG_ES,&es));
                check(uc_reg_write(uc,UC_X86_REG_BX,&bx));
                check(uc_emu_start(uc,0x13918,0x1394f,0,100));
                if(life && !--life) {
                    if(!page) life=1;
                    else state=(unsigned char)-3;
                }
            }
            /* Rendering/page switching lies between these two fragments.
             * No claim about pixel output is made here. */
            page^=1;
            check(uc_emu_start(uc,0x13fa7,0x14021,0,300));
            check(uc_mem_read(uc,0x3005a,&result,1));
            check(uc_mem_read(uc,0x3007e,life_bytes,2));
            if(result!=state || result==0 ||
               (unsigned)(life_bytes[0]|life_bytes[1]<<8)!=life ||
               ((int8_t)before<0 && result!=(unsigned char)-3)) {
                fprintf(stderr,"reserved shadow sequence mismatch page=%u frame=%u\n",initial_page,frame);
                exit(1);
            }
        }
        check(uc_close(uc));
    }
    puts("DOS shadow lifecycle: two 12-pass page-phase sequences retain and reactivate the reserved slot (rendering not exercised)");
}

int main(int argc, char **argv)
{
    static unsigned char runtime[300000];
    static const unsigned char signature[] = {0x26,0x83,0x7f,0x3e,0,0x74,0x30,
                                              0x26,0xff,0x4f,0x3e};
    FILE *file;
    size_t size, site = 0, matches = 0;
    unsigned cases = 0;
    if (argc != 2 || !(file = fopen(argv[1], "rb"))) return 1;
    size = fread(runtime, 1, sizeof runtime, file);
    if (ferror(file) || !feof(file)) return 1;
    fclose(file);
    for (size_t i = 0; i + 0x37 <= size; ++i)
        if (!memcmp(runtime + i, signature, sizeof signature)) {
            site = i; ++matches;
        }
    if (matches != 1) return fprintf(stderr, "expiry signature matches: %zu\n", matches), 1;
    for (unsigned state = 1; state <= 5; ++state)
        for (unsigned page = 0; page < 2; ++page)
            for (unsigned lifetime = 0; lifetime < 3; ++lifetime) {
                uc_engine *uc;
                uint16_t cs=0x1000, ds=0x2000, es=0x3000, ss=0x4000;
                uint16_t bx=64, bp=0x800, sp=0x700;
                unsigned char value = state, result, life[2];
                check(uc_open(UC_ARCH_X86, UC_MODE_16, &uc));
                check(uc_mem_map(uc, 0, 0x100000, UC_PROT_ALL));
                check(uc_mem_write(uc, 0x10000, runtime + site, 0x37));
                check(uc_reg_write(uc, UC_X86_REG_CS, &cs));
                check(uc_reg_write(uc, UC_X86_REG_DS, &ds));
                check(uc_reg_write(uc, UC_X86_REG_ES, &es));
                check(uc_reg_write(uc, UC_X86_REG_SS, &ss));
                check(uc_reg_write(uc, UC_X86_REG_BX, &bx));
                check(uc_reg_write(uc, UC_X86_REG_BP, &bp));
                check(uc_reg_write(uc, UC_X86_REG_SP, &sp));
                word(uc, 0x407fe, 1); /* BP-2: actor slot */
                word(uc, 0x216ce, 0); word(uc, 0x216d0, es);
                value = page; check(uc_mem_write(uc, 0x216c6, &value, 1));
                value = state; check(uc_mem_write(uc, 0x3005a, &value, 1));
                word(uc, 0x3007e, lifetime);
                check(uc_emu_start(uc, 0x10000, 0x10037, 0, 100));
                uint16_t ip;
                check(uc_reg_read(uc, UC_X86_REG_IP, &ip));
                check(uc_mem_read(uc, 0x3005a, &result, 1));
                check(uc_mem_read(uc, 0x3007e, life, 2));
                unsigned expected_life = lifetime ? lifetime - 1 : 0;
                unsigned expected_state = state;
                if (lifetime == 1) {
                    if (!page) expected_life = 1;
                    else expected_state = (unsigned char)-(int)state;
                }
                if (ip != 0x37 || result != expected_state ||
                    (unsigned)(life[0] | life[1] << 8) != expected_life)
                    return fprintf(stderr, "expiry mismatch state=%u page=%u lifetime=%u\n",
                                   state, page, lifetime), 1;
                check(uc_close(uc));
                ++cases;
            }
    printf("DOS 286 expiry: %u cases passed; zero lifetime is unlimited, page 0 defers expiry, page 1 negates actor state\n", cases);
    if (site < 0x3918 || site - 0x3918 + 0x10000 > size)
        return fprintf(stderr, "missing actor code segment\n"), 1;
    verify_retirement(runtime + site - 0x3918);
    verify_active_draw(runtime + site - 0x3918);
    verify_allocation(runtime + site - 0x3918);
    verify_point_creation(runtime + site - 0x3918);
    verify_point_coordinate_words(runtime + site - 0x3918);
    verify_shadow_sequence(runtime + site - 0x3918);
    return 0;
}
