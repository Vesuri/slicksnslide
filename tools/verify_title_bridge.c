/* Verify production wrappers with actual GCC-sized, big-endian stack slots. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>

static void check(uc_err error)
{
    if (error) { fprintf(stderr,"%s\n",uc_strerror(error)); exit(1); }
}
static unsigned be32(const unsigned char *p)
{ return (unsigned)p[0]<<24 | (unsigned)p[1]<<16 | (unsigned)p[2]<<8 | p[3]; }
static void put32(uc_engine *uc,unsigned address,unsigned value)
{
    unsigned char b[]={value>>24,value>>16,value>>8,value};
    check(uc_mem_write(uc,address,b,4));
}
static unsigned reg(uc_engine *uc,int id)
{ unsigned value; check(uc_reg_read(uc,id,&value)); return value; }
static void run(uc_engine *uc,unsigned entry,unsigned stop)
{
    unsigned sp=0x90000,sr=0;
    check(uc_reg_write(uc,UC_M68K_REG_SR,&sr));
    check(uc_reg_write(uc,UC_M68K_REG_A7,&sp));
    put32(uc,sp,0x80000);
    check(uc_emu_start(uc,entry,stop,0,100));
    if(reg(uc,UC_M68K_REG_PC)!=stop) exit(1);
}
static unsigned action(unsigned scan)
{
    switch(scan) {
    case 1: case 0x44: return 1;
    case 0x1c: case 0x1d: case 0x39: return 2;
    case 0x3b: return 3;
    case 0x43: return 4;
    case 0x58: return 5;
    default: return 0;
    }
}
int main(int argc,char **argv)
{
    unsigned char code[4096];
    if(argc!=2) return 2;
    FILE *file=fopen(argv[1],"rb");
    if(!file) return 1;
    size_t size=fread(code,1,sizeof code,file);
    if(size<20 || size==sizeof code || ferror(file)) return 1;
    fclose(file);
    uc_engine *uc;
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&uc));
    check(uc_ctl_set_cpu_model(uc,UC_CPU_M68K_M68020));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0,code,size));
    for(unsigned scan=0;scan<65536;++scan) {
        put32(uc,0x90004,scan);
        run(uc,be32(code),0x80000);
        if(reg(uc,UC_M68K_REG_D0)!=action(scan) || reg(uc,UC_M68K_REG_A7)!=0x90004) {
            fprintf(stderr,"Title dispatch ABI mismatch scan=%x\n",scan); return 1;
        }
    }
    for(unsigned v=0;v<256;++v) {
        put32(uc,0x90004,0x50000); put32(uc,0x90008,0x60000);
        put32(uc,0x9000c,v); put32(uc,0x90010,255-v); put32(uc,0x90014,v);
        run(uc,be32(code+4),be32(code+12));
        if(reg(uc,UC_M68K_REG_A0)!=0x50000 || reg(uc,UC_M68K_REG_A1)!=0x60000 ||
           (reg(uc,UC_M68K_REG_D0)&65535)!=v || (reg(uc,UC_M68K_REG_D1)&65535)!=255-v ||
           reg(uc,UC_M68K_REG_D2)!=v) { fputs("Title text ABI mismatch\n",stderr); return 1; }
        run(uc,be32(code+8),be32(code+16));
        if((reg(uc,UC_M68K_REG_D2)&65535)!=v ||
           (reg(uc,UC_M68K_REG_D0)&65535)!=0x12 || (reg(uc,UC_M68K_REG_D1)&65535)!=0x34)
            { fputs("Title selection ABI mismatch\n",stderr); return 1; }
    }
    for(unsigned x=0;x<320;++x) {
        put32(uc,0x90004,0x50000); put32(uc,0x90008,0x60000);
        put32(uc,0x9000c,0x61000); put32(uc,0x90010,x); put32(uc,0x90014,191);
        run(uc,be32(code+20),be32(code+24));
        if(reg(uc,UC_M68K_REG_A0)!=0x50000 || reg(uc,UC_M68K_REG_A1)!=0x60000 ||
           reg(uc,UC_M68K_REG_A2)!=0x61000 || (reg(uc,UC_M68K_REG_D0)&65535)!=x ||
           (reg(uc,UC_M68K_REG_D1)&65535)!=191 || reg(uc,UC_M68K_REG_D2)!=0 ||
           reg(uc,UC_M68K_REG_D3)!=1 || reg(uc,UC_M68K_REG_D4)!=10 ||
           reg(uc,UC_M68K_REG_D5)!=0 || (reg(uc,UC_M68K_REG_D6)&65535)!=0x100) {
            fputs("Original font GCC bridge mismatch\n",stderr); return 1;
        }
    }
    puts("Original font GCC bridge: 320 coordinate/argument cases pass");
    for(unsigned flags=4;flags<=6;flags+=2) for(unsigned x=0;x<320;++x) {
        put32(uc,0x90004,0x50000);put32(uc,0x90008,0x61000);
        put32(uc,0x9000c,x);put32(uc,0x90010,114);put32(uc,0x90014,flags);
        run(uc,be32(code+28),be32(code+32));
        if(reg(uc,UC_M68K_REG_A0)!=0x50000 || reg(uc,UC_M68K_REG_A2)!=0x61000 ||
           (reg(uc,UC_M68K_REG_D0)&65535)!=x || (reg(uc,UC_M68K_REG_D1)&65535)!=114 ||
           (reg(uc,UC_M68K_REG_D2)&65535)!=flags || reg(uc,UC_M68K_REG_D3)!=1 ||
           reg(uc,UC_M68K_REG_D4)!=10 || reg(uc,UC_M68K_REG_D5)!=0x56 ||
           (reg(uc,UC_M68K_REG_D6)&65535)!=0x100) return 1;
    }
    puts("Title status font bridge: 640 alignment/coordinate/argument cases pass");
    for(unsigned colour=0;colour<256;++colour) {
        unsigned registers[]={UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_D0,
            UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3};
        unsigned values[]={0x40000,0x61000,160,85,colour,0};
        for(unsigned i=0;i<6;++i) check(uc_reg_write(uc,registers[i],&values[i]));
        run(uc,be32(code+36),be32(code+32));
        unsigned char actual;check(uc_mem_read(uc,0x50006,&actual,1));
        if(actual!=colour || reg(uc,UC_M68K_REG_D2)!=5 ||
           reg(uc,UC_M68K_REG_D5)!=0x56 || reg(uc,UC_M68K_REG_A1)!=0x50000 ||
           reg(uc,UC_M68K_REG_A2)!=0x61000) return 1;
    }
    puts("Title label font wrapper: 256 foreground colours preserve the original palette-selected shadow");
    for(unsigned flags=0;flags<8;++flags) for(unsigned colour=0;colour<256;++colour) {
        put32(uc,0x90004,0x50000);put32(uc,0x90008,0x52000);put32(uc,0x9000c,0x61000);
        put32(uc,0x90010,120);put32(uc,0x90014,126);put32(uc,0x90018,flags);put32(uc,0x9001c,colour);
        run(uc,be32(code+40),be32(code+32));
        if(reg(uc,UC_M68K_REG_A0)!=0x50000 || reg(uc,UC_M68K_REG_A1)!=0x52000 ||
           reg(uc,UC_M68K_REG_A2)!=0x61000 || (reg(uc,UC_M68K_REG_D0)&65535)!=120 ||
           (reg(uc,UC_M68K_REG_D1)&65535)!=126 || (reg(uc,UC_M68K_REG_D2)&65535)!=flags ||
           reg(uc,UC_M68K_REG_D3)!=1 || reg(uc,UC_M68K_REG_D4)!=10 ||
           (reg(uc,UC_M68K_REG_D5)&65535)!=colour || (reg(uc,UC_M68K_REG_D6)&65535)!=256) return 1;
    }
    puts("Arcade title font bridge: 2048 font/flags/shadow argument cases pass");
    uc_close(uc);
    puts("Title GCC bridge: 65536 dispatch and 512 text/selection argument cases pass");
    return 0;
}
