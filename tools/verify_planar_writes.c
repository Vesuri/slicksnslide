/* Execute the production 68020 writers with a write hook: final checksums
 * alone cannot detect an out-of-bounds write subsequently repaired. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#include <unicorn/x86.h>

enum { CODE=0x1000, STOP=0x80000, STACK=0x90000, CHUNKY=0x100000,
       BITMAP=0x120000, PLANES=0x140000, SCRATCH=0x160000, PIXELS=0x180000,
       SCREEN=64000 };
static uint8_t allowed[SCREEN], initial[SCREEN], expected[SCREEN], actual[SCREEN];
static uint8_t chunky[SCREEN];
static unsigned case_number, violations;
static unsigned check_final_store;
static unsigned packed_size, maximum_lookahead;
static unsigned maximum_chunky_lookahead;
static void chunky_read_hook(uc_engine *uc,uc_mem_type type,uint64_t address,
    int size,int64_t value,void *user) {
    (void)type;(void)value;(void)user;
    uint64_t end=address+size;
    if(address<CHUNKY || end>CHUNKY+SCREEN+32) {
        fprintf(stderr,"Chunky source read outside allocation: %llx + %d\n",
            (unsigned long long)address,size);
        ++violations;uc_emu_stop(uc);
    } else if(end>CHUNKY+SCREEN && end-CHUNKY-SCREEN>maximum_chunky_lookahead)
        maximum_chunky_lookahead=(unsigned)(end-CHUNKY-SCREEN);
}
static void read_hook(uc_engine *uc, uc_mem_type type, uint64_t address,
                      int size, int64_t value, void *user) {
    (void)type; (void)value; (void)user;
    unsigned end=(unsigned)(address-SCRATCH)+size;
    if(end>packed_size) {
        unsigned extra=end-packed_size;
        if(extra>maximum_lookahead) maximum_lookahead=extra;
        if(extra>32) {
            fprintf(stderr,"Case %u excessive source lookahead: %u\n",case_number,extra);
            violations++;
            uc_emu_stop(uc);
        }
    }
}
static void check(uc_err err) {
    if (err) { fprintf(stderr,"Unicorn: %s\n",uc_strerror(err)); exit(1); }
}
static void put32(uc_engine *uc, uint32_t address, uint32_t value) {
    uint8_t b[4]={value>>24,value>>16,value>>8,value};
    check(uc_mem_write(uc,address,b,4));
}
static void put16(uc_engine *uc, uint32_t address, uint16_t value) {
    uint8_t b[2]={value>>8,value};
    check(uc_mem_write(uc,address,b,2));
}
static void write_hook(uc_engine *uc, uc_mem_type type, uint64_t address,
                       int size, int64_t value, void *user) {
    (void)type; (void)value; (void)user;
    for (int i=0;i<size;i++) {
        uint64_t at=address+i;
        if (at<PLANES || at>=PLANES+SCREEN || !allowed[at-PLANES]) {
            fprintf(stderr,"Case %u unauthorized write %08llx (%d bytes)\n",
                    case_number,(unsigned long long)address,size);
            violations++;
            uc_emu_stop(uc);
            return;
        }
        /* Rectangle conversion should store only finished plane words, not
         * use the visible bitmap as temporary transpose storage. */
        if (check_final_store &&
            (uint8_t)((uint64_t)value >> (8*(size-1-i))) != expected[at-PLANES]) {
            fprintf(stderr,"Case %u intermediate C2P value at %08llx\n",
                    case_number,(unsigned long long)at);
            violations++;
            uc_emu_stop(uc);
            return;
        }
    }
}
static void expect_pixel(unsigned x,unsigned y) {
    unsigned bit=1U<<(7-(x&7));
    for(unsigned p=0;p<8;p++) {
        unsigned at=y*320+p*40+x/8;
        allowed[at]=1;
        expected[at]=(expected[at]&~bit)|((chunky[y*320+x]&(1U<<p))?bit:0);
    }
}
static void reset(uc_engine *uc) {
    memset(allowed,0,sizeof allowed);
    memcpy(expected,initial,sizeof expected);
    check(uc_mem_write(uc,PLANES,initial,sizeof initial));
    uint32_t sp=STACK, sr=0;
    check(uc_reg_write(uc,UC_M68K_REG_SR,&sr));
    check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
    put32(uc,STACK,STOP);
    put32(uc,STACK+4,CHUNKY);
    put32(uc,STACK+8,BITMAP);
}
static void run(uc_engine *uc,uint32_t entry) {
    check(uc_emu_start(uc,entry,STOP,0,20000000));
    uint32_t pc=0;
    check(uc_reg_read(uc,UC_M68K_REG_PC,&pc));
    check(uc_mem_read(uc,PLANES,actual,sizeof actual));
    if(violations || pc!=STOP || memcmp(expected,actual,sizeof actual)) {
        fprintf(stderr,"Case %u failed, PC=%08x\n",case_number,pc);
        for(unsigned i=0;i<SCREEN;i++) if(expected[i]!=actual[i]) {
            fprintf(stderr,"First mismatch row=%u plane=%u byte=%u expected=%02x actual=%02x\n",
                    i/320,(i%320)/40,i%40,expected[i],actual[i]); break;
        }
        exit(1);
    }
    case_number++;
}
static void verify_retirement(uc_engine *uc, uint32_t entry) {
    enum { ACTORS=0x1a0000, INDICES=0x1a4000, COUNTS=0x1a5000,
           DIRTY=0x1a6000, COUNT=0x1a7000 };
    unsigned cases=0;
    for(unsigned life=0;life<=2;life++)
    for(unsigned page=0;page<2;page++)
    for(unsigned valid=0;valid<=2;valid+=2)
    for(unsigned permanent=0;permanent<=1;permanent++)
    for(unsigned full=0;full<=1;full++) {
        uint8_t actors[48]={0}, result[48], count[2];
        actors[17]=life; actors[18]=71; actors[20]=valid;
        actors[21]=permanent; actors[13]=100; actors[15]=100;
        actors[23]=permanent?5:1;
        actors[24+17]=5; actors[24+18]=218; actors[24+19]=6;
        actors[24+21]=0; actors[24+23]=1;
        check(uc_mem_write(uc,ACTORS,actors,sizeof actors));
        memset(expected,40,SCREEN);
        check(uc_mem_write(uc,CHUNKY,expected,SCREEN));
        if(life==1 && page && valid && permanent) expected[32100]=71;
        uint32_t sp=STACK, sr=0, d0=0, pc=0;
        check(uc_reg_write(uc,UC_M68K_REG_SR,&sr));
        check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
        put32(uc,STACK,STOP); put32(uc,STACK+4,ACTORS);
        put32(uc,STACK+8,2); put32(uc,STACK+12,INDICES);
        put32(uc,STACK+16,COUNTS); put32(uc,STACK+20,DIRTY);
        put32(uc,STACK+24,COUNT); put32(uc,STACK+28,CHUNKY);
        put32(uc,STACK+32,page);
        put16(uc,COUNT,full?512:0);
        check(uc_emu_start(uc,entry,STOP,0,10000));
        check(uc_reg_read(uc,UC_M68K_REG_PC,&pc));
        check(uc_reg_read(uc,UC_M68K_REG_D0,&d0));
        check(uc_mem_read(uc,CHUNKY,actual,SCREEN));
        check(uc_mem_read(uc,ACTORS,result,sizeof result));
        check(uc_mem_read(uc,COUNT,count,2));
        unsigned expires=life==1 && page;
        unsigned wanted=full?512:(expires && valid?1:0);
        unsigned remaining=2; /* Expired slot remains occupied this pass. */
        if(pc!=STOP || (d0&65535)!=remaining ||
           memcmp(expected,actual,SCREEN) ||
           ((unsigned)count[0]*256+count[1])!=wanted ||
           result[(remaining-1)*24+17]!=4 ||
           result[(remaining-1)*24+18]!=218 ||
           result[23]!=(unsigned char)(expires?(permanent?-6:-2):(permanent?5:1)) ||
           (expires && result[20]) ||
           (!expires && (result[21]!=permanent || result[17]!=(life?1:0)))) {
            fprintf(stderr,"Particle retirement failed: life=%u page=%u valid=%u permanent=%u full=%u\n",
                    life,page,valid,permanent,full);
            exit(1);
        }
        if(expires) {
            /* Next emission phase still sees the retained entry. The next
             * actor pass releases it and compacts the survivor exactly once. */
            check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
            check(uc_emu_start(uc,entry,STOP,0,10000));
            check(uc_reg_read(uc,UC_M68K_REG_D0,&d0));
            check(uc_mem_read(uc,ACTORS,result,sizeof result));
            check(uc_mem_read(uc,COUNT,count,2));
            if((d0&65535)!=1 || result[17]!=3 || result[18]!=218 ||
               result[23]!=1 || ((unsigned)count[0]*256+count[1])!=wanted) {
                fputs("Retired particle did not release on following pass\n",stderr);
                exit(1);
            }
        }
        cases++;
    }
    printf("68020 particle retirement passed %u expiry/compaction/capacity cases.\n",cases);
    const int positions[]={-32768,-32767,-65,-64,-1,0,63,64,32766,32767};
    const int velocities[]={-32768,-65,-1,0,1,65,32767};
    unsigned motion_cases=0;
    for(unsigned i=0;i<sizeof positions/sizeof *positions;i++)
    for(unsigned j=0;j<sizeof velocities/sizeof *velocities;j++) {
        uint8_t actor[24]={0}, result[8];
        uint32_t sp=STACK, sr=0;
        actor[17]=2;
        actor[23]=1;
        check(uc_mem_write(uc,ACTORS,actor,sizeof actor));
        put32(uc,ACTORS,(uint32_t)positions[i]);
        put32(uc,ACTORS+4,(uint32_t)-positions[i]);
        put16(uc,ACTORS+8,(uint16_t)velocities[j]);
        put16(uc,ACTORS+10,(uint16_t)-velocities[j]);
        check(uc_reg_write(uc,UC_M68K_REG_SR,&sr));
        check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
        put32(uc,STACK+8,1);
        check(uc_emu_start(uc,entry,STOP,0,10000));
        check(uc_mem_read(uc,ACTORS,result,sizeof result));
        for(unsigned axis=0;axis<2;axis++) {
            unsigned at=axis*4;
            uint32_t got=((uint32_t)result[at]<<24)|((uint32_t)result[at+1]<<16)|
                         ((uint32_t)result[at+2]<<8)|result[at+3];
            int sum=positions[i]+velocities[j];
            if(axis) sum=-sum;
            uint32_t low=(uint32_t)sum&65535U;
            uint32_t wanted=(low&32768U)?(low|0xffff0000U):low;
            if(got!=wanted) {
                fprintf(stderr,"Particle signed-word motion mismatch: %d + %d axis %u\n",
                        positions[i],velocities[j],axis);
                exit(1);
            }
        }
        motion_cases++;
    }
    printf("68020 particle motion passed %u signed-word boundary cases.\n",motion_cases);
}
static void xword(uc_engine *uc,unsigned at,unsigned value) {
    uint8_t bytes[2]={value,value>>8}; check(uc_mem_write(uc,at,bytes,2));
}
static void verify_dos_lifecycle(uc_engine *native,uint32_t entry,const char *path) {
    enum { ACTORS=0x1a0000, INDICES=0x1a4000, COUNTS=0x1a5000,
           DIRTY=0x1a6000, COUNT=0x1a7000 };
    const unsigned lives[]={0,1,2,3,35,50};
    uint8_t runtime[300000];
    FILE *file=fopen(path,"rb");
    if(!file) { perror(path); exit(1); }
    size_t length=fread(runtime,1,sizeof runtime,file);
    if(ferror(file) || !feof(file) || length<0x30000-0x10100+65536) abort();
    fclose(file);
    unsigned passes=0;
    for(unsigned kind=1;kind<=5;kind+=4)
    for(unsigned phase=0;phase<2;++phase)
    for(unsigned l=0;l<sizeof lives/sizeof *lives;++l) {
        uc_engine *dos;
        uint16_t cs=0x1000,ds=0x2000,ss=0x4000,bp=0x800,xsp=0x700;
        uint8_t actor[24]={0},original[64]={0},retf=0xcb;
        unsigned count=1,page=phase;
        check(uc_open(UC_ARCH_X86,UC_MODE_16,&dos));
        check(uc_mem_map(dos,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(dos,0x10000,runtime+0x30000-0x10100,65536));
        /* Existing listing segment uses physical 30000 as its zero. */
        check(uc_mem_write(dos,0x13673,&retf,1)); /* retirement redraw only */
        check(uc_reg_write(dos,UC_X86_REG_CS,&cs));
        check(uc_reg_write(dos,UC_X86_REG_DS,&ds));
        check(uc_reg_write(dos,UC_X86_REG_SS,&ss));
        check(uc_reg_write(dos,UC_X86_REG_BP,&bp));
        check(uc_reg_write(dos,UC_X86_REG_SP,&xsp));
        xword(dos,0x407fe,1); xword(dos,0x216ce,0); xword(dos,0x216d0,0x3000);
        xword(dos,0x216be,200);
        original[0x1a]=kind; original[0x3a]=1;
        check(uc_mem_write(dos,0x30040,original,sizeof original));
        xword(dos,0x30040,20000); xword(dos,0x30042,10000);
        xword(dos,0x3005b,7); xword(dos,0x3005d,(uint16_t)-5);
        xword(dos,0x3007e,lives[l]);
        actor[17]=lives[l]; actor[21]=kind==5; actor[23]=kind;
        check(uc_mem_write(native,ACTORS,actor,sizeof actor));
        put32(native,ACTORS,20000); put32(native,ACTORS+4,10000);
        put16(native,ACTORS+8,7); put16(native,ACTORS+10,(uint16_t)-5);
        for(unsigned frame=0;frame<80;++frame) {
            uint16_t es=0x3000,bx=64,ip;
            uint32_t sp=STACK,sr=0,d0,pc;
            uint8_t result[24],counts[2],state;
            check(uc_mem_read(dos,0x3005a,&state,1));
            check(uc_mem_write(dos,0x216c6,&page,1));
            check(uc_reg_write(dos,UC_X86_REG_ES,&es));
            check(uc_reg_write(dos,UC_X86_REG_BX,&bx));
            if((int8_t)state>0) {
                /* Adjacent slices have different stop PCs. Unicorn can reuse
                 * a translated block spanning an earlier stop otherwise. */
                check(uc_ctl_remove_cache(dos,0,0x100000));
                check(uc_emu_start(dos,0x13918,0x1394f,0,100));
                check(uc_reg_read(dos,UC_X86_REG_IP,&ip));
                if(ip!=0x394f) abort();
                check(uc_mem_read(dos,0x3005a,&state,1));
                if((int8_t)state>0) {
                    check(uc_ctl_remove_cache(dos,0,0x100000));
                    check(uc_emu_start(dos,0x1394f,0x139d2,0,200));
                    check(uc_reg_read(dos,UC_X86_REG_IP,&ip));
                    if(ip!=0x39d2) abort();
                }
            }
            check(uc_ctl_remove_cache(dos,0,0x100000));
            check(uc_emu_start(dos,0x13fa7,0x14021,0,300));
            check(uc_reg_read(dos,UC_X86_REG_IP,&ip));
            check(uc_mem_read(dos,0x30040,original,sizeof original));
            check(uc_reg_write(native,UC_M68K_REG_SR,&sr));
            check(uc_reg_write(native,UC_M68K_REG_A7,&sp));
            put32(native,STACK,STOP); put32(native,STACK+4,ACTORS);
            put32(native,STACK+8,count); put32(native,STACK+12,INDICES);
            put32(native,STACK+16,COUNTS); put32(native,STACK+20,DIRTY);
            put32(native,STACK+24,COUNT); put32(native,STACK+28,CHUNKY);
            put32(native,STACK+32,page); put16(native,COUNT,0);
            check(uc_emu_start(native,entry,STOP,0,10000));
            check(uc_reg_read(native,UC_M68K_REG_D0,&d0));
            check(uc_reg_read(native,UC_M68K_REG_PC,&pc));
            check(uc_mem_read(native,ACTORS,result,sizeof result));
            check(uc_mem_read(native,COUNTS,counts,2));
            count=d0&65535;
            if(ip!=0x4021 || pc!=STOP || count!=(original[0x1a]!=0) ||
               (count && (result[23]!=original[0x1a] || result[17]!=original[0x3e] ||
                result[2]!=original[1] || result[3]!=original[0] ||
                result[6]!=original[3] || result[7]!=original[2])) ||
               ((unsigned)counts[0]*256+counts[1])!=((int8_t)original[0x1a]>0)) {
                fprintf(stderr,"DOS/68020 lifecycle mismatch kind=%u phase=%u life=%u frame=%u state=%d/%d count=%u\n",
                    kind,phase,lives[l],frame,(int8_t)original[0x1a],(int8_t)result[23],count);
                fprintf(stderr,"ip=%x pc=%x life=%u/%u xy=%u,%u/%u,%u bucket=%u\n",
                    ip,pc,original[0x3e],result[17],original[0]|original[1]<<8,
                    original[2]|original[3]<<8,result[3]|result[2]<<8,
                    result[7]|result[6]<<8,(unsigned)counts[0]*256+counts[1]);
                exit(1);
            }
            page^=1; ++passes;
        }
        check(uc_close(dos));
    }
    printf("DOS/68020 point lifecycle: %u consecutive update comparisons passed (redraw pixels excluded).\n",passes);
}
int main(int argc,char **argv) {
    if(argc!=7) { fprintf(stderr,"usage: %s code.bin rect-address pixels-address particles-address runtime.bin rows-address\n",argv[0]); return 2; }
    FILE *f=fopen(argv[1],"rb"); if(!f) { perror(argv[1]); return 1; }
    uint8_t code[32768]; size_t length=fread(code,1,sizeof code,f); fclose(f);
    uc_engine *uc; uc_hook hook, reads,chunky_reads;
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&uc));
    check(uc_ctl_set_cpu_model(uc,UC_CPU_M68K_M68020));
    check(uc_mem_map(uc,0,0x200000,UC_PROT_ALL));
    check(uc_mem_write(uc,CODE,code,length));
    check(uc_hook_add(uc,&hook,UC_HOOK_MEM_WRITE,write_hook,NULL,PLANES-4096,PLANES+SCREEN+4095));
    check(uc_hook_add(uc,&reads,UC_HOOK_MEM_READ,read_hook,NULL,SCRATCH,SCRATCH+SCREEN+4095));
    check(uc_hook_add(uc,&chunky_reads,UC_HOOK_MEM_READ,chunky_read_hook,NULL,CHUNKY-4096,CHUNKY+SCREEN+4095));
    for(unsigned i=0;i<SCREEN;i++) {
        chunky[i]=(uint8_t)((i*73U)^(i>>3)^(i>>9));
        initial[i]=(uint8_t)(i*29U+17U);
    }
    check(uc_mem_write(uc,CHUNKY,chunky,sizeof chunky));
    put16(uc,BITMAP,320); put16(uc,BITMAP+2,200);
    uint8_t depth=8; check(uc_mem_write(uc,BITMAP+5,&depth,1));
    for(unsigned p=0;p<8;p++) put32(uc,BITMAP+8+p*4,PLANES+p*40);
    uint32_t rect=strtoul(argv[2],NULL,16), pixels=strtoul(argv[3],NULL,16);
    const unsigned tops[]={0,1,63,100,178,190,199};
    const unsigned heights[]={1,2,7,20,200};
    check_final_store=1;
    for(unsigned left=0;left<320;left+=32)
    for(unsigned right=left+32;right<=320;right+=32)
    for(unsigned t=0;t<sizeof tops/sizeof tops[0];t++)
    for(unsigned h=0;h<sizeof heights/sizeof heights[0];h++) {
        unsigned top=tops[t], bottom=top+heights[h]; if(bottom>200) continue;
        reset(uc);
        packed_size=(right-left)*(bottom-top);
        for(unsigned y=top;y<bottom;y++) for(unsigned x=left;x<right;x++) expect_pixel(x,y);
        put32(uc,STACK+12,left); put32(uc,STACK+16,top);
        put32(uc,STACK+20,right); put32(uc,STACK+24,bottom);
        put32(uc,STACK+28,SCRATCH);
        run(uc,rect);
    }
    for(unsigned t=0;t<sizeof tops/sizeof tops[0];++t)
    for(unsigned h=0;h<sizeof heights/sizeof heights[0];++h) {
        unsigned top=tops[t],bottom=top+heights[h];if(bottom>200)continue;
        reset(uc);
        for(unsigned y=top;y<bottom;++y)for(unsigned x=0;x<320;++x)expect_pixel(x,y);
        put32(uc,STACK+12,top);put32(uc,STACK+16,bottom);
        run(uc,(uint32_t)strtoul(argv[6],NULL,16));
    }
    check_final_store=0; /* Sparse writes may update one byte in several steps. */
    for(unsigned count=0;count<=512;count=count<8?count+1:count+63) {
        reset(uc);
        for(unsigned i=0;i<count;i++) {
            unsigned x=(i*73U+319U)%320, y=(i*37U+199U)%200;
            put16(uc,PIXELS+i*4,x);
            uint8_t row=y; check(uc_mem_write(uc,PIXELS+i*4+2,&row,1));
            expect_pixel(x,y);
        }
        put32(uc,STACK+12,PIXELS); put32(uc,STACK+16,count);
        run(uc,pixels);
    }
    /* Composition regression: sparse pixels are not a bitplane-only overlay.
     * Re-converting an enclosing rectangle from the same chunky source must
     * preserve every sparse pixel, including those sharing plane bytes. */
    reset(uc);
    for(unsigned i=0;i<16;i++) {
        unsigned x=96+i, y=100;
        put16(uc,PIXELS+i*4,x);
        uint8_t row=y; check(uc_mem_write(uc,PIXELS+i*4+2,&row,1));
        expect_pixel(x,y);
    }
    put32(uc,STACK+12,PIXELS); put32(uc,STACK+16,16);
    run(uc,pixels);
    memset(allowed,0,sizeof allowed);
    for(unsigned y=99;y<102;y++)
        for(unsigned x=64;x<160;x++) expect_pixel(x,y);
    uint32_t sp=STACK;
    check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
    put32(uc,STACK+12,64); put32(uc,STACK+16,99);
    put32(uc,STACK+20,160); put32(uc,STACK+24,102);
    put32(uc,STACK+28,SCRATCH);
    packed_size=96*3;
    check_final_store=1;
    run(uc,rect);
    printf("Sparse pixels survive a subsequent overlapping rectangle conversion.\n");
    printf("Planar writers passed %u cases: every destination write in bounds and every final byte correct.\n",case_number);
    printf("Maximum packed-source lookahead: %u bytes.\n",maximum_lookahead);
    printf("Maximum chunky-source lookahead: %u bytes (allocation padding32).\n",maximum_chunky_lookahead);
    printf("Every rectangle destination store already contains its final plane value.\n");
    verify_retirement(uc,(uint32_t)strtoul(argv[4],NULL,16));
    verify_dos_lifecycle(uc,(uint32_t)strtoul(argv[4],NULL,16),argv[5]);
    uc_close(uc); return 0;
}
