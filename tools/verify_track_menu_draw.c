#define SLICKS_MENU_TRACE_CAPACITY 128
#define main profile_setup_verifier_main
#include "verify_profile_setup.c"
#undef main
#include "../src/ui/track_menu_draw.h"

static void track_tint(void *p,short l,short t,short r,short b)
{ struct MenuDrawCall *c=menu_call(p,8); short a[]={l,t,r,b}; memcpy(c->args,a,sizeof a); }
static const unsigned char *track_name(void *p,unsigned track)
{
    (void)p; static unsigned char text[64];
    snprintf((char *)text,sizeof text,"TRACK%03u",track); return text;
}
static void track_draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address!=0x34599 && address!=0x35e63 && address!=0x3b09c) {
        editor_draw_boundary(u,address,size,p); return;
    }
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x34599) track_tint(p,(short)readword(u,stack+4),(short)readword(u,stack+6),
        (short)readword(u,stack+8),(short)readword(u,stack+10));
    else if(address==0x35e63) {
        const unsigned char *name=track_name(0,readword(u,stack+8));
        check(uc_mem_write(u,0x62000,name,strlen((const char *)name)+1));
        uint16_t ax=0,dx=0x6200;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    const unsigned char *data=runtime+0x3cbf0-0x10100;
    struct SlicksTrackMenuLabels labels={0};
    for(unsigned i=0;i<6;++i) {
        unsigned at=data[0x10b6+i*4]|data[0x10b7+i*4]<<8;
        labels.actions[i]=data+at;
    }
    labels.random_on=data+0x12e0; labels.random_off=data+0x12eb; labels.separator=data+0x1307;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x3b9de,0x309cf,0x301ab,0x2fe63,0x302b6,0x36fae,0x39ed8,0x34599,0x35e63,0x3b09c};
    struct MenuDrawTrace dos;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,track_draw_boundary,&dos,addresses[i],addresses[i]));
    }
    const short totals[]={0,1,21,22,23,195,256}; unsigned cases=0;
    for(unsigned c=0;c<7;++c) for(unsigned column=0;column<7;++column) for(unsigned v=0;v<8;++v) {
        short total=totals[c],cursor=(short)(total?((total-1)*v/7):-1),top=(short)(cursor>21?cursor-21:0);
        struct SlicksTrackMenu m={cursor,top,v==7?cursor:-2,(short)(v&1?total+10:1),(unsigned char)column,0,(unsigned char)(v&1)};
        short tracks[256]; unsigned count=v%4==0?0:v%4==1?(unsigned)total:(unsigned)total/2;
        for(unsigned i=0;i<count;++i) { tracks[i]=(short)(v%4==1?i:(i*17+v)%total); word(u,0x70000+2*i,tracks[i]); }
        struct SlicksTrackPlaylist selected={tracks,(unsigned short)count,256};
        word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x7000);
        word(u,0x3cbf0+0x90,count); word(u,0x3cbf0+0x4da8,total);
        word(u,0x3cbf0+0x626,m.random_count); word(u,0x3cbf0+0x624,m.random_order);
        word(u,0x3cbf0+0x10b2,cursor); word(u,0x3cbf0+0x10b4,top);
        word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
        word(u,0x8effa,m.previous); unsigned char small[3]={m.column,37,22};
        check(uc_mem_write(u,0x8effd,small,3));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x270b9,0x274d5,0,100000));
        struct MenuDrawTrace native={0};
        struct SlicksTrackMenuDrawOps ops={{{menu_restore,menu_bevel,menu_sprite,menu_text,&native},
            editor_colour,editor_number,editor_nearest,editor_rectangle},track_tint,track_name};
        if(slicks_draw_track_menu(&m,total,&selected,&labels,37,&ops) || memcmp(&dos,&native,sizeof dos) ||
           (unsigned short)m.previous!=readword(u,0x8effa) || (unsigned short)m.random_count!=readword(u,0x3cbf0+0x626)) {
            fprintf(stderr,"Tracks draw mismatch total=%d column=%u variant=%u calls=%u/%u\n",total,column,v,dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"First difference call=%u kinds=%u/%u text=%s/%s\n",i,dos.calls[i].kind,native.calls[i].kind,dos.calls[i].text,native.calls[i].text);
                for(unsigned j=0;j<8;++j) fprintf(stderr," %d/%d",dos.calls[i].args[j],native.calls[i].args[j]);
                fputc('\n',stderr); break;
            }
            return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original Tracks drawing: %u full command/text/order/state comparisons pass\n",cases);
    return 0;
}
