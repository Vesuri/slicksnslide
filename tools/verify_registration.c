/* Reads a private key only at runtime. Never prints names, key bytes or hashes.
 * Execute the original loader, strlen, checksum reader and case conversion;
 * model only file services and the original fatal-error boundary. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/registration.h"
#include "../src/ui/registration_ui.h"
static const unsigned char *input;
static unsigned length,position;
static int invalid,ended;
static unsigned mode,branch;
static void ck(uc_err e) { if(e) { fprintf(stderr,"Oracle: %s\n",uc_strerror(e)); exit(1); } }
static uint16_t reg(uc_engine *u,int r) { uint16_t v; ck(uc_reg_read(u,r,&v)); return v; }
static void set(uc_engine *u,int r,uint16_t v) { ck(uc_reg_write(u,r,&v)); }
static void ret(uc_engine *u,unsigned ax,unsigned dx)
{
    unsigned sp=reg(u,UC_X86_REG_SP),ss=reg(u,UC_X86_REG_SS); unsigned char b[4];
    ck(uc_mem_read(u,16*ss+sp,b,4)); set(u,UC_X86_REG_AX,ax); set(u,UC_X86_REG_DX,dx);
    set(u,UC_X86_REG_SP,sp+4); set(u,UC_X86_REG_CS,b[2]|b[3]<<8); set(u,UC_X86_REG_IP,b[0]|b[1]<<8);
}
static void code(uc_engine *u,uint64_t a,uint32_t n,void *p)
{
    (void)n;(void)p;
    if(mode) {
        if((mode==1 && (a==0x25fcb || a==0x2611e)) || (mode==2 && a==0x2647d) ||
           (mode==3 && a==0x25d5d)) {
            branch=a==0x25fcb;ended=1;ck(uc_emu_stop(u));
        }
        return;
    }
    if(a==0x11eaf) ret(u,input?1:0,0); /* fopen */
    else if(a==0x12d8a) ret(u,position<length?input[position++]:0xffff,0); /* fgetc */
    else if(a==0x119c2) ret(u,0,0); /* fclose */
    else if(a==0x36243) { invalid=ended=1; ck(uc_emu_stop(u)); }
    else if(a==0x25d6b) { ended=1; ck(uc_emu_stop(u)); }
}
static void trial(uc_engine *u,const unsigned char *runtime,unsigned size,
    const unsigned char *bytes,unsigned count,unsigned id)
{
    ck(uc_mem_write(u,0x10100,runtime,size));
    input=bytes;length=count;position=0;invalid=ended=0;
    set(u,UC_X86_REG_CS,0x1987);set(u,UC_X86_REG_DS,0x3cbf);
    set(u,UC_X86_REG_SS,0x8000);set(u,UC_X86_REG_SP,0xe000);set(u,UC_X86_REG_BP,0xf000);
    ck(uc_emu_start(u,0x25c50,0x70000,0,200000));
    unsigned char name[61];ck(uc_mem_read(u,0x3cbf0+0x62f,name,sizeof name));
    struct SlicksRegistration r;int result=slicks_registration_decode(&r,bytes,count);
    int original=invalid?-1:name[0]?1:0;
    if(!ended || result!=original || (result==1 && memcmp(name,r.name,sizeof name))) {
        fprintf(stderr,"Registration mismatch case %u (no private data logged)\n",id);exit(1);
    }
    if(result!=1 && r.name[0]) exit(1);
}
int main(int argc,char **argv)
{
    if(argc!=2) { fprintf(stderr,"usage: verify_registration PRIVATE_KEY_PATH\n");return 2; }
    unsigned char runtime[300000],key[512],changed[512];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    unsigned size=(unsigned)fread(runtime,1,sizeof runtime,f);fclose(f);
    f=fopen(argv[1],"rb");if(!f)return 2;
    unsigned bytes=(unsigned)fread(key,1,sizeof key,f);int extra=fgetc(f);fclose(f);
    if(!bytes || extra!=EOF || bytes>sizeof key-8)return 2;
    struct SlicksRegistration r;
    if(slicks_registration_decode(&r,key,bytes)!=1) { fputs("Supplied key not accepted\n",stderr);return 1; }
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    uc_hook h;ck(uc_hook_add(u,&h,UC_HOOK_CODE,code,0,1,0));unsigned cases=0;
    trial(u,runtime,size,0,0,cases++);trial(u,runtime,size,key,bytes,cases++);
    for(unsigned n=0;n<bytes;++n)trial(u,runtime,size,key,n,cases++);
    for(unsigned n=0;n<bytes;++n)for(unsigned bit=0;bit<8;++bit) {
        memcpy(changed,key,bytes);changed[n]^=(unsigned char)(1<<bit);
        trial(u,runtime,size,changed,bytes,cases++);
    }
    for(unsigned c=0;c<256;++c) { memcpy(changed,key,bytes);changed[0]=(unsigned char)c;
        trial(u,runtime,size,changed,bytes,cases++); }
    memcpy(changed,key,bytes);memset(changed+bytes,0xa5,8);
    trial(u,runtime,size,changed,bytes+8,cases++);
    memset(changed,0,sizeof changed);trial(u,runtime,size,changed,5,cases++);
    changed[0]=0x9c;trial(u,runtime,size,changed,1,cases++);
    printf("Original registration loader: %u private-data-free comparisons pass\n",cases);
    unsigned gates=0;
    for(unsigned n=0;n<4096;++n) for(unsigned registered=0;registered<2;++registered) {
        unsigned short today=(unsigned short)(n*37),installed=(unsigned short)(n*101);
        unsigned char dates[6]={today,today>>8,0,0,0,0},saved[2]={installed,installed>>8},flag=registered;
        ck(uc_mem_write(u,0x3cbf0+0x4db4,dates,6));ck(uc_mem_write(u,0x3cbf0+0x3032,saved,2));
        ck(uc_mem_write(u,0x3cbf0+0x62f,&flag,1));
        set(u,UC_X86_REG_CS,0x1987);set(u,UC_X86_REG_DS,0x3cbf);
        mode=1;ended=0;ck(uc_emu_start(u,0x25f9f,0x70000,0,100));
        if(!ended || branch!=(unsigned)slicks_registration_trial_expired(today,(short)installed,flag)) return 1;
        ++gates;
        const unsigned char filename[]="/end1.bmp";
        ck(uc_mem_write(u,0x8f000-0x16,filename,sizeof filename));
        set(u,UC_X86_REG_SS,0x8000);set(u,UC_X86_REG_BP,0xf000);
        mode=2;ended=0;ck(uc_emu_start(u,0x26472,0x70000,0,20));
        unsigned char selected[10];ck(uc_mem_read(u,0x8f000-0x16,selected,10));
        if(!ended || strcmp((const char *)selected+1,slicks_registration_exit_image(flag))) return 1;
    }
    printf("Original trial/exit gates: %u comparisons pass\n",gates);
    for(unsigned c=0;c<256;++c) {
        unsigned char name[2]={(unsigned char)c,0};
        ck(uc_mem_write(u,0x3cbf0+0x62f,name,2));
        set(u,UC_X86_REG_CS,0x1987);set(u,UC_X86_REG_SP,0xe000);
        mode=3;ended=0;ck(uc_emu_start(u,0x25d0b,0x70000,0,10000));
        ck(uc_mem_read(u,0x3cbf0+0x62f,name,2));
        if(!ended || name[0]!=slicks_registration_uppercase((unsigned char)c) || name[1]) return 1;
    }
    puts("Original name case conversion: all 256 byte values match");
    ck(uc_close(u));
    return 0;
}
