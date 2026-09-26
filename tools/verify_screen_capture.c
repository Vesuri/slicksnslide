#define main graphics_harness_main
#include "verify_native_graphics.c"
#undef main
#include "../src/ui/screen_capture.h"

static uint8_t capture[SLICKS_CAPTURE_SIZE], pixels[64000], palette[768];
static unsigned captured;
static uint16_t word_at(uc_engine *u,uint32_t at)
{ uint8_t b[2]; check_uc("read word",uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void ret_far(uc_engine *u,uint16_t sp,uint16_t ax)
{
    uint16_t ip=word_at(u,x86_stack_base+sp),cs=word_at(u,x86_stack_base+sp+2);
    sp+=4;
    check_uc("return SP",uc_reg_write(u,UC_X86_REG_SP,&sp));
    check_uc("return AX",uc_reg_write(u,UC_X86_REG_AX,&ax));
    check_uc("return CS",uc_reg_write(u,UC_X86_REG_CS,&cs));
    check_uc("return IP",uc_reg_write(u,UC_X86_REG_IP,&ip));
}
static void boundary(uc_engine *u,uint64_t pc,uint32_t size,void *context)
{
    (void)size; (void)context;
    if(pc!=0x11eaf && pc!=0x123ce && pc!=0x119c2 && pc!=0x37059 && pc!=0x3b58e) return;
    uint16_t sp; check_uc("read SP",uc_reg_read(u,UC_X86_REG_SP,&sp));
    uint32_t args=x86_stack_base+sp+4;
    uint16_t result=0;
    if(pc==0x11eaf) {
        uint16_t dx=0; check_uc("file handle high",uc_reg_write(u,UC_X86_REG_DX,&dx)); result=1;
    } else if(pc==0x123ce) {
        if(captured>=sizeof capture) { fputs("capture overflow\n",stderr); exit(1); }
        capture[captured++]=(uint8_t)word_at(u,args);
    } else if(pc==0x37059) {
        unsigned index=word_at(u,args);
        if(index>=256) exit(1);
        for(unsigned c=0;c<3;++c) {
            uint32_t ptr=word_at(u,args+2+4*c)+16UL*word_at(u,args+4+4*c);
            check_uc("palette output",uc_mem_write(u,ptr,palette+3*index+c,1));
        }
    } else if(pc==0x3b58e) {
        unsigned x=word_at(u,args),y=word_at(u,args+2);
        if(x>=320 || y>=200) exit(1);
        result=pixels[y*320+x];
    }
    ret_far(u,sp,result);
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    size_t bytes; uint8_t *runtime=read_file(argv[1],&bytes);
    if(bytes!=runtime_size) return 2;
    uc_engine *u=open_x86_relocated(runtime); uc_hook hook;
    check_uc("capture boundaries",uc_hook_add(u,&hook,UC_HOOK_CODE,(void *)boundary,0,1,0));
    uint8_t encoded[SLICKS_CAPTURE_SIZE];
    for(unsigned trial=0;trial<3;++trial) {
        for(unsigned i=0;i<64000;++i) pixels[i]=(uint8_t)(i*31+(i/320)*13+trial*71);
        for(unsigned i=0;i<768;++i) palette[i]=(uint8_t)((i*7+trial*19)&63);
        uint16_t arguments[]={0,x86_source_segment};
        prepare_x86_stack(u,arguments,2,0x2e0f); captured=0;
        uint16_t ds=0x3cbf; check_uc("relocated DS",uc_reg_write(u,UC_X86_REG_DS,&ds));
        uc_err error=uc_emu_start(u,0x305a0,0x306c6,0,10000000);
        if(error) {
            uint16_t cs,ip; uc_reg_read(u,UC_X86_REG_CS,&cs); uc_reg_read(u,UC_X86_REG_IP,&ip);
            fprintf(stderr,"capture at %04x:%04x bytes=%u\n",cs,ip,captured);
        }
        check_uc("original BMP writer",error);
        if(slicks_encode_capture(encoded,sizeof encoded,pixels,palette) ||
           captured!=sizeof encoded || memcmp(capture,encoded,sizeof encoded)) {
            fprintf(stderr,"BMP comparison failed trial=%u size=%u\n",trial,captured); return 1;
        }
    }
    if(!slicks_encode_capture(encoded,sizeof encoded-1,pixels,palette) ||
       !slicks_encode_capture(0,sizeof encoded,pixels,palette)) return 1;
    uc_close(u); free(runtime);
    puts("Screenshot BMP: three complete original-code streams and buffer guards pass");
    return 0;
}
