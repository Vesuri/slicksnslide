/* Reuse the independent x86 VGA hooks and 68020 engine setup, not a C oracle. */
#define main bounded_graphics_main
#include "verify_native_graphics.c"
#undef main

int main(int argc, char **argv)
{
    if(argc!=3) return 2;
    size_t runtime_bytes, code_bytes;
    uint8_t *runtime=read_file(argv[1],&runtime_bytes);
    uint8_t *code=read_file(argv[2],&code_bytes);
    if(runtime_bytes!=runtime_size || code_bytes<4) return 2;
    uc_engine *x86=open_x86(runtime);
    uc_engine *native=open_m68k(code,code_bytes);
    uint8_t segment[65536], expected[m68k_plane_size], actual[m68k_plane_size];
    for(unsigned trial=0;trial<128;++trial) {
        /* First two reproduce 185ff's arguments and both observed pages.
         * Others vary source offset, phase, height subtraction and SI wrap. */
        uint16_t offset=trial<2?4:(uint16_t)(next_random()&0xfffe);
        uint16_t dx=trial<2?0:(uint16_t)(trial&3);
        uint16_t sx=trial<2?35:(uint16_t)((next_random()%15)*8);
        uint16_t sy=trial<2?105:(uint16_t)(next_random()%600);
        uint16_t height=trial<2?150:(uint16_t)(next_random()%255+1);
        uint16_t page=(trial&1)?32700:0;
        /* Isolate source wrapping; destination word-boundary faults are a
         * separate VGA contract, not part of this compatibility entry. */
        if(trial>=2) page=(uint16_t)(4096-sy*100-(sx>>2));
        for(unsigned i=0;i<65536;++i) segment[i]=(uint8_t)(next_random()>>16);
        segment[offset]=80; segment[offset+1]=200;
        for(unsigned i=0;i<m68k_plane_size;++i) expected[i]=(uint8_t)(i*31+trial);
        check_uc("seed x86 segment",uc_mem_write(x86,x86_source_base,segment,sizeof segment));
        check_uc("seed native segment",uc_mem_write(native,m68k_source_base,segment,sizeof segment));
        check_uc("seed native planes",uc_mem_write(native,m68k_plane_base,expected,sizeof expected));
        uint16_t arguments[]={dx,0,sx,sy,100,(uint16_t)(height|(trial<2?0:0x3400)),
                              offset,x86_source_segment,page};
        prepare_x86_stack(x86,arguments,9,0x3b8d);
        VgaPortState ports={-1,-1,-1,0,expected,0};
        uc_hook output=0,memory=0;
        reset_x86_hook(x86,&ports,&output);
        check_uc("hook VGA writes",uc_hook_add(x86,&memory,UC_HOOK_MEM_WRITE,
            (void *)hook_x86_vga_write,&ports,x86_vga_base,x86_vga_base+x86_vga_size-1));
        uc_err error=uc_emu_start(x86,x86_subrect_start,x86_subrect_stop,0,0);
        if(error) fprintf(stderr,"original trial=%u offset=%u source=%u,%u height=%u page=%u\n",
            trial,offset,sx,sy,height,page);
        check_uc("original far crop",error);
        check_uc("remove output hook",uc_hook_del(x86,output));
        check_uc("remove memory hook",uc_hook_del(x86,memory));
        prepare_m68k_subrect(native,dx,0,sx,sy,100,(uint8_t)height,page);
        uint32_t address=m68k_source_base+offset;
        check_uc("far crop pointer",uc_reg_write(native,UC_M68K_REG_A1,&address));
        address=m68k_source_base;
        check_uc("far crop segment",uc_reg_write(native,UC_M68K_REG_A6,&address));
        address=m68k_stack_base+m68k_stack_size-16;
        check_uc("far crop stack",uc_reg_write(native,UC_M68K_REG_A7,&address));
        check_uc("native far crop",uc_emu_start(native,m68k_code_base,m68k_code_base+code_bytes-2,0,0));
        check_m68k_blit_preserved(native,"far crop",trial);
        check_uc("read far crop stack",uc_reg_read(native,UC_M68K_REG_A7,&address));
        if(address!=m68k_stack_base+m68k_stack_size-16) return 1;
        check_uc("read far crop",uc_mem_read(native,m68k_plane_base,actual,sizeof actual));
        if(ports.bad_port_value || memcmp(expected,actual,sizeof actual)) {
            fprintf(stderr,"far crop mismatch trial=%u offset=%u source=%u,%u height=%u\n",
                trial,offset,sx,sy,height);
            return 1;
        }
    }
    uc_close(x86); uc_close(native); free(runtime); free(code);
    puts("far subrectangle: 128 original/68020 full-plane comparisons passed");
    return 0;
}
