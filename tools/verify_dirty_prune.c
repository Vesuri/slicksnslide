/* Exact native sparse-list pruning against the unchanged C reference. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>

static void check(uc_err e) { if(e) { fprintf(stderr,"unicorn: %s\n",uc_strerror(e)); exit(2); } }
static unsigned offset_of(const char *path,const char *name)
{
    FILE *f=fopen(path,"r"); char line[256]; size_t n=strlen(name);
    if(!f) exit(2);
    while(fgets(line,sizeof line,f))
        if(!strncmp(line,name,n) && line[n]==' ') { fclose(f); return (unsigned)strtoul(strstr(line,"equ")+3,0,0); }
    fclose(f); fprintf(stderr,"missing %s\n",name); exit(2);
}
static unsigned rng=0x963721u;
static unsigned next(void) { rng=rng*1664525u+1013904223u; return rng>>8; }
static void word(unsigned char *p,unsigned v) { p[0]=v>>8; p[1]=v; }
static void lng(unsigned char *p,unsigned v) { word(p,v>>16); word(p+2,v); }

int main(int argc,char **argv)
{
    if(argc!=3) return 2;
    unsigned char code[4096]; FILE *f=fopen(argv[1],"rb"); if(!f) return 2;
    size_t size=fread(code,1,sizeof code,f); fclose(f);
    const unsigned rows=offset_of(argv[2],"RACE_DIRTY_ROWS"),
        nr=offset_of(argv[2],"RACE_DIRTY_ROW_COUNT"),
        pixels=offset_of(argv[2],"RACE_DIRTY_PIXELS"),
        np=offset_of(argv[2],"RACE_DIRTY_PIXEL_COUNT");
    enum { CODE=0x10000,RACE=0x100000,STACK=0x80000,STOP=0x9000,IMAGE=65536 };
    static unsigned char before[IMAGE],expected[IMAGE],got[IMAGE];
    static struct SlicksRaceRuntime race;
    if(rows+SLICKS_DIRTY_ROW_MAX*8>IMAGE || pixels+SLICKS_DIRTY_PIXEL_MAX*4>IMAGE || nr>=IMAGE || np+2>IMAGE) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,0x200000,UC_PROT_ALL)); check(uc_mem_write(u,CODE,code,size));
    const int saved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
        UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned trial=0;trial<12000;++trial) {
        memset(before,0xa7,sizeof before);
        race.dirty_row_count=trial%17;
        race.dirty_pixel_count=trial%5==0?512:trial%5==1?0:trial%5==2?1:next()%513;
        before[nr]=race.dirty_row_count; word(before+np,race.dirty_pixel_count);
        for(unsigned i=0;i<SLICKS_DIRTY_ROW_MAX;++i) {
            struct SlicksDirtyRows *r=&race.dirty_rows[i];
            r->left=next()%320; r->right=r->left+next()%(321-r->left);
            r->top=next()%200; r->bottom=r->top+next()%(201-r->top);
            if(trial%11==0) { r->left=0;r->top=0;r->right=320;r->bottom=200; }
            word(before+rows+i*8,r->left); word(before+rows+i*8+2,r->top);
            word(before+rows+i*8+4,r->right); word(before+rows+i*8+6,r->bottom);
        }
        for(unsigned i=0;i<SLICKS_DIRTY_PIXEL_MAX;++i) {
            struct SlicksDirtyPixel *p=&race.dirty_pixels[i];
            p->x=next()%320;p->y=next()%200;p->unused=next();
            /* Exact half-open edges, duplicates, and unsigned outliers. */
            if(i%3==0) {
                const struct SlicksDirtyRows *r=&race.dirty_rows[(i/3)%16];
                p->x=i&1?r->left:r->right;p->y=i&2?r->top:r->bottom;
            }
            if(trial%13==0) { p->x=0xffff;p->y=255; }
            word(before+pixels+i*4,p->x);before[pixels+i*4+2]=p->y;before[pixels+i*4+3]=p->unused;
        }
        memcpy(expected,before,sizeof expected);
        slicks_race_prune_dirty_pixels(&race);
        word(expected+np,race.dirty_pixel_count);
        for(unsigned i=0;i<SLICKS_DIRTY_PIXEL_MAX;++i) {
            const struct SlicksDirtyPixel *p=&race.dirty_pixels[i];
            word(expected+pixels+i*4,p->x);expected[pixels+i*4+2]=p->y;expected[pixels+i*4+3]=p->unused;
        }
        check(uc_mem_write(u,RACE,before,sizeof before));
        unsigned char args[8];lng(args,STOP);lng(args+4,RACE);
        unsigned sp=STACK-8;check(uc_mem_write(u,sp,args,sizeof args));check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned marks[11];
        for(unsigned i=0;i<11;++i) { marks[i]=0xa5000000u+i*0x10101u+trial;check(uc_reg_write(u,saved[i],&marks[i])); }
        check(uc_emu_start(u,CODE,STOP,0,1000000));
        unsigned pc;check(uc_reg_read(u,UC_M68K_REG_PC,&pc));check(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        if(pc!=STOP || sp!=STACK-4) { fprintf(stderr,"return/stack trial=%u\n",trial);return 1; }
        for(unsigned i=0;i<11;++i) { unsigned v;check(uc_reg_read(u,saved[i],&v));
            if(v!=marks[i]) { fprintf(stderr,"register %u trial=%u\n",i,trial);return 1; } }
        check(uc_mem_read(u,RACE,got,sizeof got));
        if(memcmp(got,expected,sizeof got)) {
            for(unsigned i=0;i<IMAGE;++i) if(got[i]!=expected[i]) {
                fprintf(stderr,"trial=%u offset=%u got=%u expected=%u\n",trial,i,got[i],expected[i]);break;
            }
            return 1;
        }
    }
    uc_close(u);puts("Native dirty pruning: 12000 lists, half-open edges, full capacity, stable order, untouched tail and ABI match C");
    return 0;
}
