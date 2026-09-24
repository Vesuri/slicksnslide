#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/track_menu.h"

struct TrackBoundary { enum SlicksTrackMenuAction action; };
static void track_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct TrackBoundary *b=context;
    unsigned target=0;
    switch(address) {
    case 0x27583: b->action=SLICKS_TRACK_MENU_TOGGLE; target=0x278f1; break;
    case 0x276bb: b->action=SLICKS_TRACK_MENU_ALL; target=0x278f1; break;
    case 0x27705: b->action=SLICKS_TRACK_MENU_CLEAR; target=0x278f1; break;
    case 0x2771c: b->action=SLICKS_TRACK_MENU_RANDOM; target=0x278f1; break;
    case 0x27812: b->action=SLICKS_TRACK_MENU_RECORDS; target=0x2787d; break;
    case 0x327dc: b->action=SLICKS_TRACK_MENU_HELP; break;
    case 0x2d752: b->action=SLICKS_TRACK_MENU_LISTS; break;
    case 0x26d34: b->action=SLICKS_TRACK_MENU_SHUFFLE; break;
    case 0x19dc2: break; /* key acknowledgement, no menu state */
    default: abort();
    }
    if(target) {
        uint16_t ip=(uint16_t)(target-0x266c0);
        check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    } else {
        uint16_t sp,ss; check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
        uint16_t ip=readword(u,ss*16U+sp),cs=readword(u,ss*16U+sp+2); sp+=4;
        check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    }
}
static void byte(uc_engine *u,unsigned address,unsigned char value)
{ check(uc_mem_write(u,address,&value,1)); }
static unsigned readbyte(uc_engine *u,unsigned address)
{ unsigned char value; check(uc_mem_read(u,address,&value,1)); return value; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    struct TrackBoundary boundary;
    const unsigned hooks[]={0x27583,0x276bb,0x27705,0x2771c,0x27812,0x327dc,0x2d752,0x26d34,0x19dc2};
    for(unsigned i=0;i<sizeof hooks/sizeof *hooks;++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,track_boundary,&boundary,hooks[i],hooks[i]));
    }
    const short counts[]={0,1,21,22,23,195,256}; unsigned cases=0;
    for(unsigned column=0;column<7;++column) for(unsigned c=0;c<7;++c)
    for(unsigned position=0;position<5;++position) for(unsigned scan=0;scan<256;++scan) {
        short count=counts[c];
        const short cursors[]={-1,0,(short)(count/2),(short)(count-1),32767};
        const short randoms[]={1,3,32767,-1,256};
        struct SlicksTrackMenu m={cursors[position],(short)(position*11),123,randoms[position],
            (unsigned char)column,0,(unsigned char)(position&1)};
        regs(u,scan); word(u,0x3cbf0+0x10b2,(unsigned short)m.cursor);
        word(u,0x3cbf0+0x10b4,(unsigned short)m.top); word(u,0x3cbf0+0x4da8,count);
        word(u,0x3cbf0+0x626,(unsigned short)m.random_count); byte(u,0x3cbf0+0x624,m.random_order);
        byte(u,0x8efff,22); byte(u,0x8effd,m.column); word(u,0x8effa,(unsigned short)m.previous); byte(u,0x8eff9,0);
        boundary.action=SLICKS_TRACK_MENU_NONE;
        check(uc_emu_start(u,0x274e5,0x2794a,0,500));
        enum SlicksTrackMenuAction action=slicks_track_menu_key(&m,count,(unsigned char)scan);
        if(action!=boundary.action || (unsigned short)m.cursor!=readword(u,0x3cbf0+0x10b2) ||
           (unsigned short)m.top!=readword(u,0x3cbf0+0x10b4) ||
           (unsigned short)m.random_count!=readword(u,0x3cbf0+0x626) || m.random_order!=readbyte(u,0x3cbf0+0x624) ||
           m.column!=readbyte(u,0x8effd) || m.done!=readbyte(u,0x8eff9) || (unsigned short)m.previous!=readword(u,0x8effa)) {
            fprintf(stderr,"Track menu mismatch column=%u count=%d position=%u scan=%x action=%u/%u\n",column,count,position,scan,action,boundary.action);
            return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Original track menu: %u navigation/state/action comparisons; playlist mutations and modal bodies remain separate\n",cases);
    return 0;
}
