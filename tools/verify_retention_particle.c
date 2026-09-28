/* Execute the production point readers and stride, not a duplicate assembly
 * implementation. Full retention decisions still require target RETCHECK. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"Unicorn: %s\n",uc_strerror(e));exit(2);}}
static unsigned be32(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put16(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void put32(unsigned char *p,uint32_t v){put16(p,v>>16);put16(p+2,v);}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    unsigned stride=(unsigned)atoi(argv[2]);if(stride!=20 && stride!=24)return 2;
    unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(n<16 || n==sizeof code)return 2;
    unsigned entries[4];for(unsigned i=0;i<4;++i){entries[i]=be32(code+n-16+i*4);if(entries[i]>=n-16)return 2;}
    enum {CODE=0x10000,POOL=0x40000,END=0x100000};
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,END,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,
        UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6,UC_M68K_REG_A7};
    for(unsigned v=0;v<65536;++v){
        unsigned char record[32],got[32];memset(record,0xcd,sizeof record);
        unsigned y=(v*40503+79)&65535;
        int x_signed=v<32768?(int)v:(int)v-65536;
        int y_signed=y<32768?(int)y:(int)y-65536;
        if(stride==20){put16(record,v);put16(record+2,y);}
        else{put32(record,(uint32_t)x_signed);put32(record+4,(uint32_t)y_signed);}
        record[stride-5]=(unsigned char)v;
        unsigned address=POOL+(v&255)*stride;
        ck(uc_mem_write(u,address,record,sizeof record));
        for(unsigned part=0;part<4;++part){
            uint32_t before[16];for(unsigned k=0;k<16;++k)before[k]=0xa5f00000+k*0x101+v;
            before[8]=address;before[15]=0xf0000;
            for(unsigned k=0;k<16;++k)ck(uc_reg_write(u,regs[k],before+k));
            ck(uc_emu_start(u,CODE+entries[part],END,0,part<2?2:1));
            unsigned changed=part==0?1:part==1?0:part==2?8:4;
            int signed_value=part==0?y_signed:x_signed;
            /* Explicit floor division, independent of host signed shifts. */
            int pixel=signed_value>=0?signed_value/64:-((-signed_value+63)/64);
            uint32_t expected=part<2?(stride==20?(before[changed]&0xffff0000)|(uint16_t)pixel:(uint32_t)pixel):
                part==2?address+stride:(before[4]&0xffffff00)|(v&255);
            for(unsigned k=0;k<16;++k){uint32_t actual;ck(uc_reg_read(u,regs[k],&actual));
                if(actual!=(k==changed?expected:before[k])){
                    fprintf(stderr,"stride=%u word=%u path=%u reg=%u got=%x expected=%x\n",stride,v,part,k,actual,k==changed?expected:before[k]);return 1;}}
            ck(uc_mem_read(u,address,got,sizeof got));if(memcmp(record,got,sizeof got))return 1;
        }
    }
    ck(uc_close(u));printf("Retention particle readers (%u-byte): all 65536 coordinate words on both axes, all priorities, 256 pool offsets, stride, untouched registers and record guards pass\n",stride);return 0;
}
