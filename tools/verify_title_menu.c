/* Execute original x86 and production 68020 composition. Draw primitives
 * are boundaries here; their independent pixel oracles remain separate. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include <unicorn/m68k.h>
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static unsigned rw(uc_engine *u,unsigned a)
{ unsigned char b[2]; ck(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void ww(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; ck(uc_mem_write(u,a,b,2)); }
static unsigned be32(const unsigned char *p)
{ return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3]; }
static void wl(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[4]={v>>24,v>>16,v>>8,v}; ck(uc_mem_write(u,a,b,4)); }
static unsigned mr(uc_engine *u,int r) { unsigned v; ck(uc_reg_read(u,r,&v)); return v; }
static void mw(uc_engine *u,int r,unsigned v) { ck(uc_reg_write(u,r,&v)); }
struct Call { unsigned short kind,a[8]; };
struct Trace { struct Call calls[16]; unsigned count,colour; };
static struct Call *call(struct Trace *t,unsigned kind)
{ if(t->count==16) abort(); struct Call *c=&t->calls[t->count++]; c->kind=kind; return c; }
static void xhook(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Trace *t=p; uint16_t sp,ss,cs,ip;
    ck(uc_reg_read(u,UC_X86_REG_SP,&sp)); ck(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=16U*ss+sp;
    if(address==0x2fe63) t->colour=rw(u,stack+4)&255;
    else if(address==0x36227) {
        unsigned at=16*rw(u,stack+6)+rw(u,stack+4); unsigned char key;
        ck(uc_mem_read(u,at+4,&key,1)); if(key<'1' || key>'7') abort();
        uint16_t ax=(uint16_t)(key-'1'),dx=0x7000;
        ck(uc_reg_write(u,UC_X86_REG_AX,&ax)); ck(uc_reg_write(u,UC_X86_REG_DX,&dx));
    } else if(address==0x301ab) {
        struct Call *c=call(t,1); unsigned char id;
        ck(uc_mem_read(u,16*rw(u,stack+10)+rw(u,stack+8),&id,1));
        c->a[0]=id;c->a[1]=rw(u,stack+4);c->a[2]=rw(u,stack+6);
        c->a[3]=t->colour;c->a[4]=rw(u,stack+16);c->a[5]=rw(u,stack+18);
    } else if(address==0x309cf) {
        struct Call *c=call(t,2); for(unsigned i=0;i<8;++i) c->a[i]=rw(u,stack+4+2*i);
    } else abort();
    ip=rw(u,stack);cs=rw(u,stack+2);sp+=4;
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_IP,&ip));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
struct Native { struct Trace trace; unsigned text,bevel; };
static void mhook(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Native *n=p;
    if(address==n->text) {
        static const char *labels[]={"GO !!!","PLAYERS","TRACKS","OPTIONS","LOAD GAME","READ THIS","QUIT"};
        char text[16]={0}; ck(uc_mem_read(u,mr(u,UC_M68K_REG_A1),text,15));
        unsigned id=0;while(id<7 && strcmp(labels[id],text)) ++id;if(id==7) abort();
        struct Call *c=call(&n->trace,1);c->a[0]=id;
        c->a[1]=mr(u,UC_M68K_REG_D0);c->a[2]=mr(u,UC_M68K_REG_D1);
        c->a[3]=mr(u,UC_M68K_REG_D2)&255;c->a[4]=5;c->a[5]=mr(u,UC_M68K_REG_D3);
    } else if(address==n->bevel) {
        struct Call *c=call(&n->trace,2);for(unsigned i=0;i<8;++i) c->a[i]=mr(u,UC_M68K_REG_D0+i);
    }
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    unsigned char runtime[300000],code[16384]={0},elf[65536]; FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f);fclose(f);
    f=fopen(argv[1],"rb");if(!f)return 2;size_t elfsize=fread(elf,1,sizeof elf,f);fclose(f);
    if(elfsize<52 || memcmp(elf,"\177ELF\1\2",6)) return 2;
    unsigned sections=be32(elf+32),stride=(unsigned)elf[46]*256+elf[47],count=(unsigned)elf[48]*256+elf[49];
    if(stride<40 || sections>elfsize || count>(elfsize-sections)/stride) return 2;
    for(unsigned i=0;i<count;++i) {
        const unsigned char *s=elf+sections+i*stride;
        if(be32(s+4)!=1 || !(be32(s+8)&2)) continue;
        unsigned address=be32(s+12),offset=be32(s+16),bytes=be32(s+20);
        if(address>sizeof code || bytes>sizeof code-address || offset>elfsize || bytes>elfsize-offset) return 2;
        memcpy(code+address,elf+offset,bytes);
    }
    uc_engine *x,*m;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&x));ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&m));
    ck(uc_ctl_set_cpu_model(m,UC_CPU_M68K_M68020));
    ck(uc_mem_map(x,0,0x100000,UC_PROT_ALL));ck(uc_mem_write(x,0x10100,runtime,size));
    ck(uc_mem_map(m,0,0x100000,UC_PROT_ALL));ck(uc_mem_write(m,0,code,sizeof code));
    const unsigned char ids[]={0,1,2,3,4,5,6};ck(uc_mem_write(x,0x70000,ids,sizeof ids));
    struct Trace original={0};struct Native native={.text=be32(code+4),.bevel=be32(code+8)};uc_hook hook;
    const unsigned addresses[]={0x2fe63,0x36227,0x301ab,0x309cf};
    for(unsigned i=0;i<4;++i) ck(uc_hook_add(x,&hook,UC_HOOK_CODE,xhook,&original,addresses[i],addresses[i]));
    ck(uc_hook_add(m,&hook,UC_HOOK_CODE,mhook,&native,1,0));
    unsigned cases=0;
    for(unsigned row=0;row<7;++row) for(unsigned colour=0;colour<256;colour+=17)
    for(unsigned page=0;page<2;++page) {
        original=(struct Trace){0};native.trace=(struct Trace){0};
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        ck(uc_reg_write(x,UC_X86_REG_CS,&cs));ck(uc_reg_write(x,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(x,UC_X86_REG_SS,&ss));ck(uc_reg_write(x,UC_X86_REG_SP,&sp));ck(uc_reg_write(x,UC_X86_REG_BP,&bp));
        ww(x,0x8f006,row);ww(x,0x8efff,colour);ww(x,0x8effb,255-colour);ww(x,0x3cbf0+0x1d89,page?0x7fbc:0);
        ck(uc_emu_start(x,0x29852,0x29928,0,5000));
        mw(m,UC_M68K_REG_SR,0);mw(m,UC_M68K_REG_A7,0x90000);wl(m,0x90000,0x80000);
        mw(m,UC_M68K_REG_D0,colour);mw(m,UC_M68K_REG_D1,255-colour);mw(m,UC_M68K_REG_D2,row);
        mw(m,UC_M68K_REG_D7,page?0x7fbc:0);mw(m,UC_M68K_REG_A0,0x40000);mw(m,UC_M68K_REG_A1,0x30000);
        ck(uc_emu_start(m,be32(code),0x80000,0,5000));
        if(mr(m,UC_M68K_REG_PC)!=0x80000 || original.count!=native.trace.count || memcmp(original.calls,native.trace.calls,sizeof original.calls)) {
            fprintf(stderr,"Title draw mismatch row=%u colour=%u page=%u calls=%u/%u\n",row,colour,page,original.count,native.trace.count);return 1;
        }
        ++cases;
    }
    uc_close(m);uc_close(x);printf("Original/68020 title menu: %u complete draw-command comparisons pass\n",cases);return 0;
}
