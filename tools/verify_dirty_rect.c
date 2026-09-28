/* Native mark_dirty_rect (src/game/dirty_rect.s) against the C reference.
 * Randomized call chains start from arbitrary 0..16-entry lists, including
 * containment, touching, repeated merges and the full-list fallback. The
 * real 68020 code runs in Unicorn on a big-endian image of the list; every
 * list entry, the count and all callee-saved registers must agree. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>

static void check(uc_err e) { if(e) { fprintf(stderr,"unicorn: %s\n",uc_strerror(e)); exit(2); } }
static unsigned long offset_of(const char *path,const char *name)
{
    FILE *f=fopen(path,"r"); char line[256]; size_t n=strlen(name);
    if(!f) exit(2);
    while(fgets(line,sizeof line,f))
        if(!strncmp(line,name,n) && line[n]==' ') { fclose(f); return strtoul(strstr(line,"equ")+3,0,0); }
    fclose(f); fprintf(stderr,"missing %s\n",name); exit(2);
}
static uint32_t rng=0x12345678u;
static unsigned next(void) { rng=rng*1103515245u+12345u; return rng>>8; }
static short coordinate(void)
{
    unsigned r=next()%16;
    if(r<2) return (short)(-(int)(next()%64));          /* negative clip */
    if(r<4) return (short)(300+next()%80);              /* beyond width */
    if(r<6) return (short)((next()%11)*32);             /* column edges */
    return (short)(next()%321);
}

/* Independent coverage oracle: one bit per aligned 8-pixel column,
 * not the C/native merge algorithm. Overlap and duplicate conversion are
 * permitted, but every requested pixel must remain covered. */
static void coverage(uint64_t rows[200],int left,int top,int right,int bottom)
{
    if(left<0)left=0;
    if(top<0)top=0;
    if(right>320)right=320;
    if(bottom>200)bottom=200;
    if(left>=right || top>=bottom)return;
    unsigned first=(unsigned)left/8,last=((unsigned)right+7)/8;
    uint64_t mask=((UINT64_C(1)<<last)-1)^((UINT64_C(1)<<first)-1);
    for(int y=top;y<bottom;++y)rows[y]|=mask;
}

int main(int argc,char **argv)
{
    if(argc!=3) return 2;
    static unsigned char code[4096];
    FILE *f=fopen(argv[1],"rb"); if(!f) return 2;
    size_t size=fread(code,1,sizeof code,f); fclose(f);
    const unsigned long rows_at=offset_of(argv[2],"RACE_DIRTY_ROWS");
    const unsigned long count_at=offset_of(argv[2],"RACE_DIRTY_ROW_COUNT");
    const uint32_t CODE=0x10000,RACE=0x100000,STACK=0x80000,STOP=0x9000;
    uc_engine *u;
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,0x200000,UC_PROT_ALL));
    check(uc_mem_write(u,CODE,code,size));
    static struct SlicksRaceRuntime race;
    const int saved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
        UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned long calls=0,merges=0,fallbacks=0,contained=0;
    for(unsigned chain=0;chain<20000;++chain) {
        race.dirty_row_count=(unsigned char)(next()%(SLICKS_DIRTY_ROW_MAX+1));
        for(unsigned i=0;i<SLICKS_DIRTY_ROW_MAX;++i) {
            struct SlicksDirtyRows *r=&race.dirty_rows[i];
            r->left=(unsigned short)((next()%10)*32); r->right=(unsigned short)(r->left+32+(next()%3)*32);
            if(r->right>320) r->right=320;
            r->top=(unsigned short)(next()%200); r->bottom=(unsigned short)(r->top+1+next()%40);
            if(r->bottom>200) r->bottom=200;
        }
        if(chain<4){
            race.dirty_row_count=1;
            race.dirty_rows[0]=(struct SlicksDirtyRows){0,0,16,4};
        }
        uint64_t required[200]={0};
        for(unsigned i=0;i<race.dirty_row_count;++i){
            const struct SlicksDirtyRows *r=&race.dirty_rows[i];
            coverage(required,r->left,r->top,r->right,r->bottom);
        }
        unsigned char image[SLICKS_DIRTY_ROW_MAX*8];
        for(unsigned i=0;i<SLICKS_DIRTY_ROW_MAX;++i) {
            const unsigned short v[4]={race.dirty_rows[i].left,race.dirty_rows[i].top,
                race.dirty_rows[i].right,race.dirty_rows[i].bottom};
            for(unsigned k=0;k<4;++k) { image[i*8+k*2]=(unsigned char)(v[k]>>8); image[i*8+k*2+1]=(unsigned char)v[k]; }
        }
        check(uc_mem_write(u,RACE+rows_at,image,sizeof image));
        check(uc_mem_write(u,RACE+count_at,&race.dirty_row_count,1));
        for(unsigned step=0;step<12;++step,++calls) {
            short args[4]={coordinate(),coordinate(),coordinate(),coordinate()};
            if(next()%4==0) { args[2]=(short)(args[0]+1+next()%40); args[3]=(short)(args[1]+1+next()%20); }
            if(chain<4 && step==0){
                static const short edges[4][4]={
                    {16,2,32,4}, /* horizontal contact */
                    {0,4,16,6},  /* vertical contact */
                    {16,4,32,6}, /* corner contact */
                    {8,2,24,6}   /* genuine overlap */
                };
                memcpy(args,edges[chain],sizeof args);
            }
            coverage(required,args[0],args[1],args[2],args[3]);
            unsigned before=race.dirty_row_count;
            mark_dirty_rect(&race,args[0],args[1],args[2],args[3]);
            if(chain<4 && step==0 && race.dirty_row_count!=(chain==3?1:2)){
                fprintf(stderr,"strict overlap/contact policy failed\n");return 1;
            }
            if(race.dirty_row_count<before) ++merges;
            if(before==SLICKS_DIRTY_ROW_MAX && race.dirty_row_count==1) ++fallbacks;
            if(race.dirty_row_count==before) ++contained;
            unsigned char frame[24];
            const uint32_t words[6]={STOP,RACE,(uint32_t)(int32_t)args[0],(uint32_t)(int32_t)args[1],
                (uint32_t)(int32_t)args[2],(uint32_t)(int32_t)args[3]};
            for(unsigned k=0;k<6;++k) { frame[k*4]=words[k]>>24;frame[k*4+1]=words[k]>>16;frame[k*4+2]=words[k]>>8;frame[k*4+3]=words[k]; }
            uint32_t sp=STACK-24; check(uc_mem_write(u,sp,frame,24));
            check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
            uint32_t marks[11];
            for(unsigned k=0;k<11;++k) { marks[k]=0xa5000000u+k*0x10101u+calls; check(uc_reg_write(u,saved[k],&marks[k])); }
            check(uc_emu_start(u,CODE,STOP,0,100000));
            uint32_t pc,after_sp; check(uc_reg_read(u,UC_M68K_REG_PC,&pc)); check(uc_reg_read(u,UC_M68K_REG_A7,&after_sp));
            if(pc!=STOP || after_sp!=STACK-20) { fprintf(stderr,"stack/return mismatch call=%lu\n",calls); return 1; }
            for(unsigned k=0;k<11;++k) { uint32_t v; check(uc_reg_read(u,saved[k],&v));
                if(v!=marks[k]) { fprintf(stderr,"register %u clobbered call=%lu\n",k,calls); return 1; } }
            unsigned char got[SLICKS_DIRTY_ROW_MAX*8],count;
            check(uc_mem_read(u,RACE+rows_at,got,sizeof got)); check(uc_mem_read(u,RACE+count_at,&count,1));
            if(count!=race.dirty_row_count) { fprintf(stderr,"count mismatch call=%lu %u/%u\n",calls,count,race.dirty_row_count); return 1; }
            for(unsigned i=0;i<SLICKS_DIRTY_ROW_MAX;++i) {
                const unsigned short v[4]={race.dirty_rows[i].left,race.dirty_rows[i].top,
                    race.dirty_rows[i].right,race.dirty_rows[i].bottom};
                for(unsigned k=0;k<4;++k) if(got[i*8+k*2]*256U+got[i*8+k*2+1]!=v[k]) {
                    fprintf(stderr,"row mismatch call=%lu entry=%u field=%u\n",calls,i,k); return 1; }
            }
            uint64_t actual[200]={0};
            for(unsigned i=0;i<count;++i){
                unsigned v[4];for(unsigned k=0;k<4;++k)v[k]=got[i*8+k*2]*256U+got[i*8+k*2+1];
                if(v[0]>=v[2] || v[1]>=v[3] || v[2]>320 || v[3]>200 || ((v[0]|v[2])&15)){
                    fprintf(stderr,"invalid output rectangle call=%lu\n",calls);return 1;
                }
                coverage(actual,(int)v[0],(int)v[1],(int)v[2],(int)v[3]);
            }
            for(unsigned y=0;y<200;++y)if(required[y]&~actual[y]){
                fprintf(stderr,"lost dirty coverage call=%lu y=%u\n",calls,y);return 1;
            }
        }
    }
    printf("Native dirty rectangles: %lu calls match C and independent coverage (%lu merges, %lu full-list fallbacks, %lu unchanged counts)\n",
        calls,merges,fallbacks,contained);
    return 0;
}
