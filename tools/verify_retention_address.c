/* Execute the actual address instructions/table from sprite_retention.s.
 * Remaining decisions are verified by the full target retention comparison. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"Unicorn: %s\n",uc_strerror(e));exit(2);}}
static unsigned be32(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned offset(const char *path,const char *name){
    FILE *f=fopen(path,"r");char line[256];if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,name,strlen(name)) && line[strlen(name)]==' '){
        unsigned v=(unsigned)strtoul(strstr(line,"equ")+3,0,0);fclose(f);return v;
    }
    fprintf(stderr,"Missing offset %s\n",name);exit(2);
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(code,1,sizeof code,f);fclose(f);if(size<12 || size==sizeof code)return 2;
    unsigned entry[2]={be32(code+size-12),be32(code+size-8)},table=be32(code+size-4);
    unsigned stride=offset(argv[2],"ACTOR_SIZE"),prev=offset(argv[2],"PREV_SIZE");
    unsigned capacity=offset(argv[2],"ACTOR_CAPACITY"),handle=offset(argv[2],"ENTRY_HANDLE");
    unsigned previous=offset(argv[2],"RACE_SPRITE_DIRTY_PREVIOUS");
    if(prev!=12 || capacity!=200 || (capacity-1)*stride>32767 || table+capacity*2>size-12)return 2;
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10000,code,size));
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,
        UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6,UC_M68K_REG_A7};
    for(unsigned h=0;h<capacity;++h){
        if(code[table+h*2]*256U+code[table+h*2+1]!=h*stride)return 1;
        for(unsigned variant=0;variant<8;++variant)for(unsigned part=0;part<2;++part){
            uint32_t before[16];for(unsigned k=0;k<16;++k)before[k]=0xa5000000U+k*0x12345+variant;
            before[1]=h;before[11]=0x180000;before[12]=0x100000+variant*2;
            before[14]=0x110000+variant*2;before[15]=0x1f0000;
            for(unsigned k=0;k<16;++k)ck(uc_reg_write(u,regs[k],&before[k]));
            unsigned char byte=(unsigned char)h;ck(uc_mem_write(u,before[11]+handle,&byte,1));
            ck(uc_emu_start(u,0x10000+entry[part],0x200000,0,part?5:4));
            uint32_t expected=part?before[12]+previous+h*prev:before[14]+h*stride;
            for(unsigned k=0;k<16;++k){
                uint32_t got;ck(uc_reg_read(u,regs[k],&got));
                if(k==0)continue; /* documented arithmetic scratch */
                uint32_t want=k==(part?8U:10U)?expected:before[k];
                if(got!=want){fprintf(stderr,"handle=%u part=%u reg=%u got=%x want=%x\n",h,part,k,got,want);return 1;}
            }
        }
    }
    ck(uc_close(u));puts("Retention address arithmetic: all 200 handles, 8 base alignments, both native paths and table pass");return 0;
}
