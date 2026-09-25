#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/moving_probe.h"
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v) { unsigned char b[2]={v,v>>8}; ck(uc_mem_write(u,at,b,2)); }
static unsigned rd(uc_engine *u,unsigned at) { unsigned char b[2]; ck(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void byte(uc_engine *u,unsigned at,unsigned v) { unsigned char b=v; ck(uc_mem_write(u,at,&b,1)); }
struct Trace { unsigned count,pattern; short entries[4096][5]; };
static struct Trace original,native;
static int record(struct Trace *t,int kind,short x,short y,signed char a,signed char b)
{
    if(t->count>=4096) abort();
    short *e=t->entries[t->count++]; e[0]=kind;e[1]=x;e[2]=y;e[3]=a;e[4]=b;
    unsigned v=(unsigned short)(x*7+y*11+a*3+b*5);
    if(kind==0) return t->pattern==1 || (t->pattern==2 && v%7==0) || (t->pattern==3 && t->count>9);
    if(kind==1) return t->pattern>=4 && v%(3+t->pattern)==0?1+v%4:0;
    return 0;
}
static int track(void *p,short x,short y,signed char a,signed char b) { return record(p,0,x,y,a,b); }
static signed char car(void *p,short x,short y,signed char a,signed char b) { return (signed char)record(p,1,x,y,a,b); }
static void wall(void *p,short x,short y,signed char a) { (void)record(p,2,x,y,a,0); }
static void service(uc_engine *u,uint64_t at,uint32_t size,void *unused)
{
    (void)size;(void)unused; uint16_t ss,sp,ax,cs,ip;
    ck(uc_reg_read(u,UC_X86_REG_SS,&ss));ck(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16+sp;
    int kind=at==0x1c5a0?0:at==0x1ca5e?1:2;
    int result=record(&original,kind,(short)rd(u,stack+4),(short)rd(u,stack+6),
        (signed char)rd(u,stack+8),kind==2?0:(signed char)rd(u,stack+10));
    ck(uc_reg_read(u,UC_X86_REG_AX,&ax)); ax=(ax&0xff00)|(unsigned char)result;
    ip=rd(u,stack);cs=rd(u,stack+2);sp+=4;
    ck(uc_reg_write(u,UC_X86_REG_AX,&ax));ck(uc_reg_write(u,UC_X86_REG_IP,&ip));
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void start(uc_engine *u,unsigned address)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
    word(u,0x8f000,0);word(u,0x8f002,0x7000);
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
    ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
    ck(uc_emu_start(u,address,0x70000,0,1000000));
}
int main(void)
{
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    unsigned char bytes[300000];size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,bytes,n));
    uc_hook hooks[3];unsigned addresses[]={0x1c5a0,0x1ca5e,0x1c63e};
    for(unsigned i=0;i<3;++i) ck(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,service,0,addresses[i],addresses[i]));
    const struct SlicksMovingProbeOps ops={track,car,wall,&native};
    const short deltas[]={0,1,-1,4,-4,51,-51,181,-181,300,-300};
    unsigned cases=0;
    for(unsigned t=0;t<16384;++t) {
        short x=(short)(t*107),y=(short)(t*37),nx=(short)(x+deltas[t%11]),ny=(short)(y+deltas[(t/11)%11]);
        signed char scale=(t&1)?16:-3,exclude=(signed char)((t/121)%6),sampling=(signed char)((t/726)%5-1),layer=(t/3630)&1;
        short out_x=12345,out_y=-4567;
        memset(&original,0,sizeof original);memset(&native,0,sizeof native);
        original.pattern=native.pattern=(t/7)%8;
        word(u,0x8f004,x);word(u,0x8f006,y);word(u,0x8f008,nx);word(u,0x8f00a,ny);
        word(u,0x8f00c,0xee00);word(u,0x8f00e,0x8000);word(u,0x8f010,0xee02);word(u,0x8f012,0x8000);
        word(u,0x8f014,scale);word(u,0x8f016,exclude);word(u,0x8f018,sampling);word(u,0x8f01a,layer);
        word(u,0x8ee00,out_x);word(u,0x8ee02,out_y);
        start(u,0x1cb02);uint16_t ax;ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        int result=slicks_moving_probe(x,y,nx,ny,&out_x,&out_y,scale,exclude,sampling,layer,&ops);
        if(result!=(signed char)ax || out_x!=(short)rd(u,0x8ee00) || out_y!=(short)rd(u,0x8ee02) ||
           memcmp(&original,&native,sizeof native)) {
            fprintf(stderr,"Moving probe mismatch trial=%u result=%d/%d calls=%u/%u\n",t,result,(signed char)ax,native.count,original.count);return 1;
        }
        ++cases;
    }
    printf("Original moving probe: %u returns, destinations and ordered call traces match\n",cases);
    for(unsigned i=0;i<3;++i) ck(uc_hook_del(u,hooks[i]));
    cases=0;
    for(unsigned t=0;t<32768;++t) {
        int x[4],y[4]; signed char roles[4];unsigned char layers[4];
        short px=(short)(t*27),py=(short)(t*13),special=(t%7==0)?1:0;
        signed char exclude=(signed char)(t%6-1),layer=(t/6)%3;
        for(unsigned d=0;d<4;++d) {
            x[d]=((int)px+(int)((t>>(d*2))%13)-6)*100+37;
            y[d]=((int)py+(int)((t>>(d*3))%13)-6)*100+71;
            roles[d]=(signed char)((t>>(d*2))%3)-1;layers[d]=(t>>d)&1;
            word(u,0x3cbf0+0x538c+4*d,x[d]);word(u,0x3cbf0+0x538e +4*d,(unsigned)x[d]>>16);
            word(u,0x3cbf0+0x539c+4*d,y[d]);word(u,0x3cbf0+0x539e +4*d,(unsigned)y[d]>>16);
            byte(u,0x3cbf0+0x4bc6+d,roles[d]);byte(u,0x3cbf0+0x5388+d,layers[d]);
        }
        word(u,0x3cbf0+0x3058+54*exclude,special);
        word(u,0x8f004,px);word(u,0x8f006,py);word(u,0x8f008,exclude);word(u,0x8f00a,layer);
        start(u,0x1ca5e);uint16_t ax;ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if((signed char)ax!=slicks_probe_cars(px,py,exclude,layer,x,y,roles,layers,special)) {
            fprintf(stderr,"Car probe mismatch trial=%u\n",t);return 1;
        }
        ++cases;
    }
    printf("Original car probe: %u owner/layer/state/position cases match\n",cases);
    ck(uc_close(u));return 0;
}
