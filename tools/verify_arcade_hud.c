#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/arcade_hud.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void longword(uc_engine *u,unsigned at,unsigned v)
{ word(u,at,v); word(u,at+2,v>>16); }
static void capture(uc_engine *u,uint64_t address,uint32_t size,void *user)
{
    (void)size; struct SlicksArcadeHud *out=user;
    uint16_t ss,sp,ip,cs,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned s=ss*16U+sp;
    if(address==0x36fae) {
        /* Palette boundary: identify the requested grey; arithmetic and
         * command orchestration execute unmodified original instructions. */
        ax=get(u,s+4);
        if(ax!=get(u,s+6) || ax!=get(u,s+8)) abort();
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else if(address==0x39ed8) {
        if(out->count>=2) abort();
        out->rects[out->count++]=(struct SlicksArcadeRect){
            get(u,s+4),get(u,s+6),get(u,s+8),get(u,s+10),get(u,s+12)};
    } else if(address==0x301ab) {
        char text[5]={0};
        unsigned ptr=get(u,s+8)+16U*get(u,s+10);
        check(uc_mem_read(u,ptr,text,4));
        if(get(u,s+4)!=70 || get(u,s+6)!=189 || get(u,s+16)!=1) abort();
        if(!strcmp(text,"LAST")) out->text=1;
        else if(!strcmp(text,"LAP")) out->text=2;
        else abort();
    } else if(address==0x2fe63 && get(u,s+4)!=60) abort();
    ip=get(u,s); cs=get(u,s+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(!n || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    struct SlicksArcadeHud actual;
    unsigned hooks[]={0x36fae,0x39ed8,0x301ab,0x2fe63};
    for(unsigned i=0;i<4;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,capture,&actual,hooks[i],hooks[i])); }
    word(u,0x3cbf0+0x459,0); /* No demo overlay. */
    const short times[]={5,10,30,99,300};
    const unsigned ticks[]={0,1,89,90,449,450,451,719,720,721,900,990,27000,40000};
    const unsigned deadlines[]={0,1,1800,2700,0xffffffff};
    unsigned cases=0;
    for(unsigned mode=0;mode<6;++mode)
    for(unsigned t=0;t<sizeof times/sizeof times[0];++t)
    for(unsigned k=0;k<sizeof ticks/sizeof ticks[0];++k)
    for(unsigned d=0;d<sizeof deadlines/sizeof deadlines[0];++d) {
        memset(&actual,0,sizeof actual);
        word(u,0x3cbf0+0x92,mode); word(u,0x3cbf0+0xfa,times[t]);
        longword(u,0x3cbf0+0x74bc,ticks[k]);
        longword(u,0x3cbf0+0x685e,ticks[k]); longword(u,0x3cbf0+0x6862,deadlines[d]);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1f84d,0x90000,0,100000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        struct SlicksArcadeHud expected;
        if(ip || sp!=0xf004 || slicks_arcade_hud(mode,times[t],ticks[k],ticks[k],deadlines[d],&expected) ||
            actual.count!=expected.count || actual.text!=expected.text) {
            fprintf(stderr,"Arcade HUD state mismatch %u %d %u %u\n",mode,times[t],ticks[k],deadlines[d]); return 1;
        }
        for(unsigned i=0;i<actual.count;++i) {
            const struct SlicksArcadeRect *a=&actual.rects[i],*b=&expected.rects[i];
            if(a->left!=b->left || a->top!=b->top || a->right!=b->right || a->bottom!=b->bottom || a->grey!=b->grey) {
                fprintf(stderr,"Arcade HUD rectangle mismatch %u: %d,%d,%d,%d vs %d,%d,%d,%d\n",i,
                    a->left,a->top,a->right,a->bottom,b->left,b->top,b->right,b->bottom); return 1;
            }
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original Arcade HUD: %u command sequences pass\n",cases); return 0;
}
