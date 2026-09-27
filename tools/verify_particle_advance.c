/* Native update/compaction against original DOS motion and lifetime code.
 * Pixel retirement and native bookkeeping are checked separately here;
 * original state retirement is also exercised by verify_retirement(). */
#define main dos_expiry_main
#include "verify_dos_particle_expiry.c"
#undef main
#include <unicorn/m68k.h>

static void be16(unsigned char *p,unsigned n){p[0]=n>>8;p[1]=n;}
static void be32(unsigned char *p,uint32_t n){be16(p,n>>16);be16(p+2,n);}
static unsigned get16(const unsigned char *p){return p[0]*256U+p[1];}
static uint64_t dos_stop;
/* Explicit fragment stops remain reliable when a reused Unicorn engine has
 * already translated the neighbouring block for a different stop address. */
static void stop_fragment(uc_engine *u,uint64_t address,uint32_t size,void *context)
{ (void)size;(void)context;if(address==dos_stop)check(uc_emu_stop(u)); }
static void original_step(uc_engine *u,unsigned char *p,unsigned page)
{
    uint16_t cs=0x1000,ds=0x2000,es=0x3000,ss=0x4000,bp=0x800,sp=0x700,bx=64;
    const int regs[]={UC_X86_REG_CS,UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,
        UC_X86_REG_BP,UC_X86_REG_SP,UC_X86_REG_BX};
    uint16_t values[]={cs,ds,es,ss,bp,sp,bx};
    for(unsigned i=0;i<7;++i)check(uc_reg_write(u,regs[i],&values[i]));
    unsigned char record[64]={0};record[0x1a]=p[23];record[0x3e]=p[17];
    check(uc_mem_write(u,0x30040,record,sizeof record));
    word(u,0x30040,get16(p+2));word(u,0x30042,get16(p+6));
    word(u,0x3005b,get16(p+8));word(u,0x3005d,get16(p+10));
    word(u,0x407fe,1);word(u,0x216ce,0);word(u,0x216d0,0x3000);
    unsigned char value=page;check(uc_mem_write(u,0x216c6,&value,1));
    dos_stop=0x1394f;check(uc_emu_start(u,0x13918,0x18000,0,100));
    check(uc_mem_read(u,0x3005a,&value,1));
    p[17]=(unsigned char)readword(u,0x3007e);
    if((int8_t)value<0) {
        /* The following original retirement block maps -1/-5 to -2/-6;
         * verify_retirement below checks all 256 input state bytes. */
        p[23]=(unsigned char)(value-1);return;
    }
    dos_stop=0x139d2;check(uc_emu_start(u,0x1394f,0x18000,0,200));
    be32(p,(uint32_t)(int32_t)(int16_t)readword(u,0x30040));
    be32(p+4,(uint32_t)(int32_t)(int16_t)readword(u,0x30042));
}

int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    static unsigned char runtime[300000],code[4096];
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f);fclose(f);
    const unsigned char signature[]={0x26,0x83,0x7f,0x3e,0,0x74,0x30,0x26,0xff,0x4f,0x3e};
    size_t site=0,matches=0;
    for(size_t i=0;i+sizeof signature<size;++i)if(!memcmp(runtime+i,signature,sizeof signature)){site=i;++matches;}
    if(matches!=1 || site<0x3918 || site-0x3918+65536>size)return 2;
    const unsigned char *segment=runtime+site-0x3918;
    verify_retirement(segment);
    f=fopen(argv[2],"rb");if(!f)return 2;size=fread(code,1,sizeof code,f);fclose(f);
    uc_engine *dos,*native;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&dos));check(uc_mem_map(dos,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(dos,0x10000,segment,65536));
    uc_hook hook;check(uc_hook_add(dos,&hook,UC_HOOK_CODE,stop_fragment,0,1,0));
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&native));
    check(uc_ctl_set_cpu_model(native,UC_CPU_M68K_M68020));
    check(uc_mem_map(native,0,0x100000,UC_PROT_ALL));check(uc_mem_write(native,0x10000,code,size));
    unsigned char rows[1024];for(unsigned y=0;y<256;++y)be32(rows+4*y,320*y);
    check(uc_mem_write(native,0x80000,rows,sizeof rows));
    static unsigned char initial[24*256+32],expected[sizeof initial],actual[sizeof initial];
    static unsigned char pixels[64000],got_pixels[64000],indices[1024],got_indices[1024];
    static unsigned char dirty[2048],got_dirty[2048];
    const int preserved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned trial=0;trial<1028;++trial) {
        unsigned count=trial%257,page=(trial%514)/257,out=0,ncounts[4]={0},collect=trial<514;
        unsigned dirty_count=trial%4==0?512:trial%4==1?511:trial%4==2?500:0;
        unsigned original_dirty_count=dirty_count;
        memset(initial,0xcc,sizeof initial);memset(indices,0xcc,sizeof indices);memset(dirty,0xcc,sizeof dirty);
        for(unsigned i=0;i<64000;++i)pixels[i]=(unsigned char)(i*13+trial);
        check(uc_mem_write(native,0x30000,pixels,sizeof pixels));
        for(unsigned i=0;i<count;++i) {
            unsigned char *p=initial+i*24;
            be32(p,(uint32_t)(int32_t)(int16_t)(i*1777+trial));
            be32(p+4,(uint32_t)(int32_t)(int16_t)(i*971-trial));
            be16(p+8,i*311+0x8000);be16(p+10,0x7fff-i*173);
            be16(p+12,(i*3)%320);be16(p+14,(i*7)%200);
            p[16]=i;p[17]=(i+trial)%6;p[18]=111+i;
            const unsigned priorities[]={0,3,5,6,127,255};p[19]=priorities[i%6];
            p[20]=(i+trial)%4;p[21]=(i+trial)%2;p[22]=15;
            p[23]=i%11==0?(p[21]?-6:-2):(p[21]?5:1);
        }
        memcpy(expected,initial,sizeof initial);
        for(unsigned i=0;i<count;++i) {
            unsigned char p[24];memcpy(p,initial+24*i,24);
            if((int8_t)p[23]<0)continue;
            original_step(dos,p,page);
            if((int8_t)p[23]<0) {
                if(p[20]&2) {
                    unsigned x=get16(p+12),y=get16(p+14);
                    if(p[21])pixels[y*320+x]=p[18];
                    if(dirty_count<512) {be16(dirty+4*dirty_count,x);dirty[4*dirty_count+2]=y;dirty[4*dirty_count+3]=0;++dirty_count;}
                }
                p[20]=0;
            } else {
                unsigned bucket=p[19]==0?0:p[19]==3?1:p[19]==5?2:3;
                if(collect)indices[bucket*256+ncounts[bucket]++]=out;
            }
            /* In-place updates also change source slots before compaction. */
            memcpy(expected+i*24,p,24);memcpy(expected+out*24,p,24);++out;
        }
        unsigned char stack[36],counts[8]={0},got_counts[8],dc[2];
        const unsigned args[]={0x18000,0x20000,count,collect?0x50000:0,collect?0x51000:0,0x60000,0x61000,0x30000,page};
        for(unsigned i=0;i<9;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(native,0x20000,initial,sizeof initial));
        memset(got_indices,0xcc,sizeof got_indices);memset(got_dirty,0xcc,sizeof got_dirty);
        check(uc_mem_write(native,0x50000,got_indices,sizeof got_indices));
        check(uc_mem_write(native,0x51000,counts,sizeof counts));
        check(uc_mem_write(native,0x60000,got_dirty,sizeof got_dirty));
        be16(dc,original_dirty_count);check(uc_mem_write(native,0x61000,dc,2));
        check(uc_mem_write(native,0x90000,stack,sizeof stack));uint32_t sp=0x90000,result;
        check(uc_reg_write(native,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){uint32_t v=0x98760000+i;check(uc_reg_write(native,preserved[i],&v));}
        check(uc_emu_start(native,0x10000,0x18000,0,100000));
        check(uc_reg_read(native,UC_M68K_REG_D0,&result));
        check(uc_mem_read(native,0x20000,actual,sizeof actual));
        check(uc_mem_read(native,0x30000,got_pixels,sizeof got_pixels));
        check(uc_mem_read(native,0x50000,got_indices,sizeof got_indices));
        check(uc_mem_read(native,0x51000,got_counts,sizeof got_counts));
        check(uc_mem_read(native,0x60000,got_dirty,sizeof got_dirty));check(uc_mem_read(native,0x61000,dc,2));
        for(unsigned i=0;i<4;++i)be16(counts+2*i,ncounts[i]);
        if((result&65535)!=out || memcmp(actual,expected,sizeof actual) ||
           memcmp(pixels,got_pixels,sizeof pixels) || memcmp(indices,got_indices,sizeof indices) ||
           memcmp(counts,got_counts,8) || memcmp(dirty,got_dirty,sizeof dirty) || get16(dc)!=dirty_count) {
            fprintf(stderr,"Native particle advance mismatch trial=%u count=%u/%u dirty=%u/%u pixels=%d indices=%d counts=%d dirtybytes=%d\n",trial,result&65535,out,get16(dc),dirty_count,memcmp(pixels,got_pixels,sizeof pixels),memcmp(indices,got_indices,sizeof indices),memcmp(counts,got_counts,8),memcmp(dirty,got_dirty,sizeof dirty));
            for(unsigned i=0;i<sizeof actual;++i)if(actual[i]!=expected[i]){fprintf(stderr,"particle byte %u actual=%u expected=%u\n",i,actual[i],expected[i]);break;}
            for(unsigned i=0;i<24;++i)fprintf(stderr,"%02x/%02x%s",actual[i],expected[i],i==23?"\n":" ");
            return 1;
        }
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){check(uc_reg_read(native,preserved[i],&result));if(result!=0x98760000+i)return 1;}
        check(uc_reg_read(native,UC_M68K_REG_A7,&sp));if(sp!=0x90004)return 1;
        check(uc_reg_read(native,UC_M68K_REG_PC,&result));if(result!=0x18000)return 1;
    }
    /* Shared actor pool: the same DOS-derived point results, plus the handle,
     * trail-index and slot-state bookkeeping previously done by C loops. */
    uint32_t shared_entry=0x10000+((uint32_t)code[size-4]<<24)+((uint32_t)code[size-3]<<16)+
        ((uint32_t)code[size-2]<<8)+code[size-1];
    for(unsigned trial=0;trial<800;++trial) {
        unsigned count=trial%200,page=(trial/200)&1,out=0;
        unsigned dirty_count=trial%4==0?512:trial%4==1?511:trial%4==2?500:0;
        unsigned original_dirty_count=dirty_count;
        static unsigned char handles[256],got_handles[256],index_bytes[400],got_index_bytes[400];
        static unsigned char states[200],got_states[200];
        memset(initial,0xcc,sizeof initial);memset(dirty,0xcc,sizeof dirty);
        for(unsigned i=0;i<64000;++i)pixels[i]=(unsigned char)(i*7+trial);
        check(uc_mem_write(native,0x30000,pixels,sizeof pixels));
        for(unsigned i=0;i<400;++i)index_bytes[i]=(unsigned char)(i*5+trial);
        for(unsigned i=0;i<200;++i)states[i]=(unsigned char)(i*3+trial);
        memset(handles,0xcc,sizeof handles);
        for(unsigned i=0;i<count;++i) {
            unsigned char *p=initial+i*24;
            be32(p,(uint32_t)(int32_t)(int16_t)(i*1777+trial*3));
            be32(p+4,(uint32_t)(int32_t)(int16_t)(i*971-trial*5));
            be16(p+8,i*311+0x8000+trial);be16(p+10,0x7fff-i*173);
            be16(p+12,(i*3+trial)%320);be16(p+14,(i*7)%200);
            p[16]=i;p[17]=(i+trial)%6;p[18]=111+i;p[19]=(i*5)%7;
            p[20]=(i+trial)%4;p[21]=(i+trial/3)%2;p[22]=(i*9)%31;
            p[23]=(i+trial)%9==0?(p[21]?-6:-2):(p[21]?5:1);
            handles[i]=(unsigned char)(1+(i*73+trial)%199);
        }
        memcpy(expected,initial,sizeof initial);
        unsigned char want_handles[256],want_index[400],want_states[200];
        memcpy(want_handles,handles,sizeof handles);memcpy(want_index,index_bytes,sizeof index_bytes);
        memcpy(want_states,states,sizeof states);
        for(unsigned i=0;i<count;++i) {
            unsigned char p[24];memcpy(p,initial+24*i,24);
            unsigned h=handles[i];
            if((int8_t)p[23]<0) { want_states[h]=0;be16(want_index+2*h,0xffff);continue; }
            original_step(dos,p,page);
            if((int8_t)p[23]<0) {
                if(p[20]&2) {
                    unsigned x=get16(p+12),y=get16(p+14);
                    if(p[21])pixels[y*320+x]=p[18];
                    if(dirty_count<512) {be16(dirty+4*dirty_count,x);dirty[4*dirty_count+2]=y;dirty[4*dirty_count+3]=0;++dirty_count;}
                }
                p[20]=0;
            }
            memcpy(expected+out*24,p,24);
            want_handles[out]=(unsigned char)h;be16(want_index+2*h,out);want_states[h]=p[23];
            ++out;
        }
        unsigned char stack[40],dc[2];
        const unsigned args[]={0x18000,0x20000,count,0x52000,0x53000,0x54000,0x60000,0x61000,0x30000,page};
        for(unsigned i=0;i<10;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(native,0x20000,initial,sizeof initial));
        check(uc_mem_write(native,0x52000,handles,sizeof handles));
        check(uc_mem_write(native,0x53000,index_bytes,sizeof index_bytes));
        check(uc_mem_write(native,0x54000,states,sizeof states));
        memset(got_dirty,0xcc,sizeof got_dirty);check(uc_mem_write(native,0x60000,got_dirty,sizeof got_dirty));
        be16(dc,original_dirty_count);check(uc_mem_write(native,0x61000,dc,2));
        check(uc_mem_write(native,0x90000,stack,sizeof stack));uint32_t sp=0x90000,result;
        check(uc_reg_write(native,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){uint32_t v=0x13570000+i;check(uc_reg_write(native,preserved[i],&v));}
        check(uc_emu_start(native,shared_entry,0x18000,0,200000));
        check(uc_reg_read(native,UC_M68K_REG_D0,&result));
        check(uc_mem_read(native,0x20000,actual,sizeof actual));
        check(uc_mem_read(native,0x30000,got_pixels,sizeof got_pixels));
        check(uc_mem_read(native,0x52000,got_handles,sizeof got_handles));
        check(uc_mem_read(native,0x53000,got_index_bytes,sizeof got_index_bytes));
        check(uc_mem_read(native,0x54000,got_states,sizeof got_states));
        check(uc_mem_read(native,0x60000,got_dirty,sizeof got_dirty));check(uc_mem_read(native,0x61000,dc,2));
        if((result&65535)!=out || memcmp(actual,expected,sizeof actual) ||
           memcmp(pixels,got_pixels,sizeof pixels) || memcmp(want_handles,got_handles,sizeof handles) ||
           memcmp(want_index,got_index_bytes,sizeof index_bytes) || memcmp(want_states,got_states,sizeof states) ||
           memcmp(dirty,got_dirty,sizeof dirty) || get16(dc)!=dirty_count) {
            fprintf(stderr,"Shared particle advance mismatch trial=%u count=%u/%u dirty=%u/%u pixels=%d handles=%d index=%d states=%d dirtybytes=%d\n",
                trial,result&65535,out,get16(dc),dirty_count,memcmp(pixels,got_pixels,sizeof pixels),
                memcmp(want_handles,got_handles,sizeof handles),memcmp(want_index,got_index_bytes,sizeof index_bytes),
                memcmp(want_states,got_states,sizeof states),memcmp(dirty,got_dirty,sizeof dirty));
            for(unsigned i=0;i<sizeof actual;++i)if(actual[i]!=expected[i]){fprintf(stderr,"particle byte %u actual=%u expected=%u\n",i,actual[i],expected[i]);break;}
            return 1;
        }
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){check(uc_reg_read(native,preserved[i],&result));if(result!=0x13570000+i)return 1;}
        check(uc_reg_read(native,UC_M68K_REG_A7,&sp));if(sp!=0x90004)return 1;
        check(uc_reg_read(native,UC_M68K_REG_PC,&result));if(result!=0x18000)return 1;
    }
    puts("Native particle update: 1028 legacy/shared batches match DOS lifetime/motion, retirement bookkeeping, compaction, permanent pixels, dirty saturation and ABI");
    puts("Native shared-pool particle update: 800 batches also match handle compaction, trail indices, slot states and untouched dead records");
    check(uc_close(native));check(uc_close(dos));return 0;
}
