#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned seed=141;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static unsigned big(const unsigned char *p){return (unsigned)p[0]<<24|p[1]<<16|p[2]<<8|p[3];}
static void word(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void stop(uc_engine *u,uint64_t at,uint32_t n,void *p)
{(void)at;(void)n;(void)p;ck(uc_emu_stop(u));}
struct Reads{unsigned count;int invalid;};
static void read_map(uc_engine *u,uc_mem_type type,uint64_t at,int n,int64_t v,void *p)
{
    struct Reads *r=p;(void)type;(void)v;++r->count;
    if(at<0x20000 || at+n>0x20000+60800 || n!=1){r->invalid=1;ck(uc_emu_stop(u));}
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[16384],map[65536];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(n<22||n==sizeof code)return 2;
    enum{CODE=0x10000,MAP=0x20000,CAR=0x40000,RACE=0x50000,STACK=0x80000};
    unsigned entry=CODE+big(code+n-22),exits[]={CODE+big(code+n-18),CODE+big(code+n-14),CODE+big(code+n-10)};
    unsigned layer_offset=big(code+n-6);
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));ck(uc_mem_write(u,CODE,code,n));
    uc_hook h;for(unsigned i=0;i<3;++i)ck(uc_hook_add(u,&h,UC_HOOK_CODE,stop,0,exits[i],exits[i]));
    struct Reads reads={0};ck(uc_hook_add(u,&h,UC_HOOK_MEM_READ,read_map,&reads,MAP,MAP+65535));
    unsigned counts[3]={0},fallback=0;
    for(unsigned t=0;t<24000;++t){
        int ox=rnd()%320,oy=rnd()%190,dx=(int)(rnd()%363)-181,dy=(int)(rnd()%363)-181;
        if(t%4==0){dx=(int)(rnd()%9)-4;dy=(int)(rnd()%9)-4;}
        if(t%11==0){dx=(int)(rnd()%65535)-32767;dy=(int)(rnd()%65535)-32767;}
        if(!dx&&!dy)dx=1;
        unsigned layer=t&1;
        if(t%26==0){const int origins[]={-32768,-1,320,32767};ox=origins[(t/26)%4];oy=110;layer=0;}
        /* The 181/182 cutoff is a semantic guard, not just a speed choice:
         * a 182-square ray wraps its original minor product near the end. */
        if(t%101==0){ox=oy=0;dx=dy=181+(t/101)%2;layer=0;}
        unsigned ax=abs(dx),ay=abs(dy),count=ax>ay?ax:ay;
        int boundary=(int)(t%7)-1;
        if(ax>181||ay>181)++fallback;
        for(unsigned i=0;i<65536;++i)map[i]=(t%4==0||t%101==0)?0:(unsigned char)(rnd()%256);
        ck(uc_mem_write(u,MAP,map,sizeof map));
        int clearx=ox,cleary=oy;unsigned result=2,samples=0;
        for(unsigned step=1;step<=count;++step){
            int x,y;
            /* Independent original equation: signed word product, signed
             * truncating division, then signed word coordinate arithmetic. */
            if(ax>=ay){x=(int16_t)(ox+(dx<0?-(int)step:(int)step));y=(int16_t)(oy-(int16_t)(step*(unsigned)(-dy))/(int)count);}
            else {y=(int16_t)(oy+(dy<0?-(int)step:(int)step));x=(int16_t)(ox-(int16_t)(step*(unsigned)(-dx))/(int)count);}
            unsigned offset=(uint16_t)(y*320+x);
            if(layer?((unsigned)x>=320||(unsigned)y>=190):offset>=60800){result=1;break;}
            ++samples;unsigned material=map[offset];
            if(material==2 || (material>=22&&material<=26&&(int)(material-22)<=boundary)){result=0;break;}
            clearx=x;cleary=y;
        }
        unsigned char frame[10]={0},lb=(unsigned char)layer;word(frame,ox);word(frame+2,oy);
        ck(uc_mem_write(u,STACK,frame,sizeof frame));ck(uc_mem_write(u,CAR+layer_offset,&lb,1));
        const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_A0,
            UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6,UC_M68K_REG_A7};
        unsigned vals[]={(uint16_t)-dx,(uint16_t)-dy,ax,ay,MAP+32768,(unsigned)ox,(unsigned)oy,RACE,CAR,(unsigned)boundary,STACK};
        for(unsigned i=0;i<11;++i)ck(uc_reg_write(u,regs[i],&vals[i]));reads=(struct Reads){0};
        ck(uc_emu_start(u,entry,0,0,3000000));
        unsigned pc,x,y,sp;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_A2,&x));ck(uc_reg_read(u,UC_M68K_REG_A3,&y));ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        if(pc!=exits[result] || (int32_t)x!=clearx || (int32_t)y!=cleary || reads.invalid || reads.count!=samples || sp!=STACK){
            fprintf(stderr,"probe %u origin=%d,%d delta=%d,%d layer=%u boundary=%d exit=%x/%x clear=%d,%d/%d,%d reads=%u/%u invalid=%d\n",t,ox,oy,dx,dy,layer,boundary,pc,exits[result],(int)x,(int)y,clearx,cleary,reads.count,samples,reads.invalid);return 1;}
        ++counts[result];
    }
    uc_close(u);printf("Native car rays: 24000 signed-word oracle cases (%u hits, %u bounds errors, %u clear; %u large rays), exact reads/clear outputs/stack pass\n",counts[0],counts[1],counts[2],fallback);return 0;
}
