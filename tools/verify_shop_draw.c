#define SLICKS_MENU_TRACE_CAPACITY 256
#define SLICKS_MENU_TEXT_CAPACITY 128
#define main profile_setup_verifier_main
#include "verify_profile_setup.c"
#undef main
#include "../src/ui/shop_draw.h"

static void shop_number(void *p,unsigned font,short number,short x,short y,unsigned char flags)
{ struct MenuDrawCall *c=menu_call(p,5); short a[]={(short)font,number,x,y,flags}; memcpy(c->args,a,sizeof a); }
static void shop_colour(void *p,unsigned font,unsigned char r,unsigned char g,unsigned char b)
{ struct MenuDrawCall *c=menu_call(p,9); short a[]={(short)font,r,g,b};memcpy(c->args,a,sizeof a); }
static void shop_tint(void *p,short l,short t,short r,short b)
{ struct MenuDrawCall *c=menu_call(p,8);short a[]={l,t,r,b};memcpy(c->args,a,sizeof a); }
static unsigned char pending_rgb[3];
static void shop_draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address!=0x302b6) { editor_draw_boundary(u,address,size,p); return; }
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned at=16U*ss+sp;
    shop_number(p,readword(u,at+10)==0x100?0:1,(short)readword(u,at+8),
        (short)readword(u,at+4),(short)readword(u,at+6),(unsigned char)readword(u,at+14));
    ip=readword(u,at);cs=readword(u,at+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void shop_static_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address==0x301ab || address==0x302b6) {shop_draw_boundary(u,address,size,p);return;}
    uint16_t ss,sp,cs,ip,ax=0,dx=0x6000;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned at=16U*ss+sp;unsigned a[8];for(unsigned i=0;i<8;++i)a[i]=readword(u,at+4+2*i);
    if(address==0x36fae) { for(unsigned i=0;i<3;++i)pending_rgb[i]=(unsigned char)a[i]; }
    else if(address==0x2fe63) shop_colour(p,a[1]==0x100?0:1,pending_rgb[0],pending_rgb[1],pending_rgb[2]);
    else if(address==0x34599) shop_tint(p,(short)a[0],(short)a[1],(short)a[2],(short)a[3]);
    else if(address==0x2e2d2) menu_sprite(p,(short)((a[2]-0x1000)/16),(short)a[0],(short)a[1]);
    else if(address==0x2e0f8) {
        unsigned char name[16];check(uc_mem_read(u,a[0]+16*a[1],name,sizeof name));
        unsigned id=100;
        for(unsigned i=0;i<14;++i) if(name[i]>='0' && name[i]<='9' && name[i+1]>='0' && name[i+1]<='9') {id=10*(name[i]-'0')+name[i+1]-'0';break;}
        if(id>=13) {fprintf(stderr,"Unexpected shop asset %s\n",name);exit(1);}
        ax=(uint16_t)(0x1000+16*id);
    }
    else if(address!=0x344c5 && address!=0x139fd) abort();
    check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    ip=readword(u,at);cs=readword(u,at+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned ds=0x3cbf0;
    struct SlicksShopRules rules;
    check(uc_mem_read(u,ds+0x106f,rules.flags,13));check(uc_mem_read(u,ds+0x10a4,rules.capacity,13));
    check(uc_mem_read(u,ds+0x107c,rules.batch,13));check(uc_mem_read(u,ds+0x1aa,rules.vehicle_capacity,10));
    check(uc_mem_read(u,ds+0x13e,rules.weapon_weight,8));
    for(unsigned i=0;i<13;++i) { rules.base_price[i]=(short)readword(u,ds+0x108a+2*i);rules.ammunition_price[i]=(short)readword(u,ds+0x17c+2*i); }
    struct MenuDrawTrace dos;
    const unsigned addresses[]={0x3b9de,0x309cf,0x301ab,0x302b6,0x39ed8};
    uc_hook dynamic_hooks[5];
    for(unsigned i=0;i<5;++i) check(uc_hook_add(u,&dynamic_hooks[i],UC_HOOK_CODE,shop_draw_boundary,&dos,addresses[i],addresses[i]));
    unsigned char exit_label[64];check(uc_mem_read(u,ds+0x1523,exit_label,64));
    word(u,ds+0x680,0x100);word(u,ds+0x682,0x6000);word(u,ds+0x684,0x200);word(u,ds+0x686,0x6000);
    unsigned cases=0;
    /* The original input loop skips the painter when neither refresh
       selector is set, including ignored keys and a restored Help view. */
    {
        uint16_t cs=0x266c,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8eff3,0);
        memset(&dos,0,sizeof dos);
        check(uc_emu_start(u,0x2cf6d,0x2cfcc,0,100));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x690c || dos.count) {
            fprintf(stderr,"Shop zero-refresh branch failed ip=%04x calls=%u\n",ip,dos.count);
            return 1;
        }
        puts("Original shop zero-refresh branch skips painting");
    }
    for(unsigned trial=0;trial<4096;++trial) {
        struct SlicksSetupSession s={0};
        unsigned mask=1+trial%15,flags=(trial/15)%16;
        s.options=(struct SlicksRaceOptions){.weapons_enabled=flags&1,.inventory_mode=flags&2,.fuel=flags&4,.damage=flags&8};
        word(u,ds+0x3020,s.options.weapons_enabled);word(u,ds+0x3022,s.options.inventory_mode);
        word(u,ds+0x3024,s.options.fuel);word(u,ds+0x3026,s.options.damage);
        unsigned char extra=(trial/240)&1;check(uc_mem_write(u,ds+0x62f,&extra,1));
        for(unsigned d=0;d<4;++d) {
            s.players.selected[d]=(mask&(1<<d))?(short)(d+1):0;
            s.players.participation[d]=s.players.selected[d]?((trial+d)&1?1:-1):0;
            s.players.vehicle[d]=(trial+d)%10;
            s.players.count+=s.players.selected[d]!=0;
            word(u,ds+0x44c+2*d,s.players.selected[d]);
            check(uc_mem_write(u,ds+0x4bc6+d,&s.players.participation[d],1));
            check(uc_mem_write(u,ds+0x4bc2+d,&s.players.vehicle[d],1));
            s.cash[d]=(short)(trial*13+d*517);word(u,ds+0x4bf6+2*d,s.cash[d]);
            for(unsigned i=0;i<13;++i) { s.inventory[d][i]=(trial+d+i)%(rules.capacity[i]+1); word(u,ds+0x6a7a+26*d+2*i,s.inventory[d][i]); }
        }
        check(uc_mem_write(u,ds+0x4c16,&s.players.count,1));
        signed char rows=0;while(rows<13 && slicks_shop_item(&rules,&s.options,s.inventory[0],s.players.participation[0],s.players.vehicle[0],extra,rows)>=0) ++rows;
        signed char column=trial%s.players.count,row=trial%(rows+1),refresh_driver=(signed char)(trial%6)-1,refresh_row=(signed char)(trial%15)-1;
        uint16_t cs=0x266c,dsreg=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&dsreg));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8e000,0);word(u,0x8e002,0x7000);
        const short args[]={column,row,refresh_driver,refresh_row,rows,137,211};
        for(unsigned i=0;i<7;++i) word(u,0x8e004+2*i,args[i]);
        memset(&dos,0,sizeof dos);check(uc_emu_start(u,0x2c574,0x70000,0,1000000));check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        struct MenuDrawTrace native={0};struct SlicksShopDrawOps ops={{menu_restore,menu_bevel,menu_sprite,menu_text,&native},shop_number,editor_rectangle};
        signed char result=slicks_draw_shop_values(&s,&rules,extra,column,row,refresh_driver,refresh_row,rows,137,211,exit_label,&ops);
        if(result!=(signed char)ax || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Shop draw mismatch trial=%u driver=%d/%d calls=%u/%u\n",trial,result,(signed char)ax,dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"call %u kind %u/%u\n",i,dos.calls[i].kind,native.calls[i].kind);
                for(unsigned j=0;j<8;++j) fprintf(stderr," %d/%d",dos.calls[i].args[j],native.calls[i].args[j]);fputc('\n',stderr);break;
            }
            return 1;
        }
        ++cases;
    }
    printf("Original shop draw: %u complete refresh/selection/command/font comparisons pass\n",cases);
    for(unsigned i=0;i<5;++i)check(uc_hook_del(u,dynamic_hooks[i]));
    const unsigned statics[]={0x301ab,0x302b6,0x36fae,0x2fe63,0x34599,0x344c5,0x2e0f8,0x2e2d2,0x139fd};
    for(unsigned i=0;i<sizeof statics/sizeof *statics;++i) {uc_hook h;check(uc_hook_add(u,&h,UC_HOOK_CODE,shop_static_boundary,&dos,statics[i],statics[i]));}
    const unsigned char *data=runtime+0x3cbf0-0x10100;
    const unsigned char names[4][21]={"Driver one","Second driver","COMPUTER","Fourth"};
    for(unsigned d=0;d<4;++d)check(uc_mem_write(u,ds+0x36aa+21*(d+1),names[d],21));
    cases=0;
    for(unsigned trial=0;trial<512;++trial) {
        struct SlicksSetupSession s={0};unsigned flags=trial%16,mask=1+trial/16%15;
        s.options=(struct SlicksRaceOptions){.weapons_enabled=flags&1,.inventory_mode=flags&2,.fuel=flags&4,.damage=flags&8};
        word(u,ds+0x3020,s.options.weapons_enabled);word(u,ds+0x3022,s.options.inventory_mode);
        word(u,ds+0x3024,s.options.fuel);word(u,ds+0x3026,s.options.damage);
        struct SlicksShopContent content={.session=&s,.rules=&rules,.items=(const unsigned char (*)[15])(data+0xfac),
            .footer=data+0x1531,.exit_label=data+0x1523,.register_label=data+0x1575,.separator=data+0x1307,
            .extra=trial&1,.track=(short)(trial+1),.total=512};
        check(uc_mem_write(u,ds+0x62f,&content.extra,1));word(u,ds+0x628,trial);word(u,ds+0x90,512);
        for(unsigned d=0;d<4;++d) {
            s.players.selected[d]=(mask&(1<<d))?(short)(d+1):0;
            s.players.participation[d]=s.players.selected[d]?1:0;s.players.vehicle[d]=(trial+d)%10;
            s.players.count+=s.players.selected[d]!=0;content.names[d]=names[d];
            word(u,ds+0x44c+2*d,s.players.selected[d]);check(uc_mem_write(u,ds+0x4bc6+d,&s.players.participation[d],1));
            check(uc_mem_write(u,ds+0x4bc2+d,&s.players.vehicle[d],1));
            for(unsigned i=0;i<13;++i)word(u,ds+0x6a7a+26*d+2*i,0);
        }
        for(unsigned i=0;i<10;++i) {word(u,ds+0x4e44+4*i,0x1000+16*(13+i));word(u,ds+0x4e46+4*i,0x6000);}
        check(uc_mem_write(u,ds+0x4c16,&s.players.count,1));
        signed char rows=0;while(rows<13 && slicks_shop_item(&rules,&s.options,s.inventory[0],s.players.participation[0],s.players.vehicle[0],content.extra,rows)>=0)++rows;
        uint16_t cs=0x266c,dsreg=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&dsreg));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos);check(uc_emu_start(u,0x2ca23,0x2cf00,0,1000000));
        struct MenuDrawTrace native={0};struct SlicksShopStaticOps ops={{{menu_restore,menu_bevel,menu_sprite,menu_text,&native},shop_number,editor_rectangle},shop_colour,shop_tint};
        if(slicks_draw_shop_background(&content,rows,&ops) || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Shop static mismatch trial=%u calls=%u/%u\n",trial,dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i)if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"call %u kind %u/%u text=%s/%s\n",i,dos.calls[i].kind,native.calls[i].kind,dos.calls[i].text,native.calls[i].text);
                for(unsigned j=0;j<8;++j)fprintf(stderr," %d/%d",dos.calls[i].args[j],native.calls[i].args[j]);fputc('\n',stderr);break;
            }
            return 1;
        }++cases;
    }
    check(uc_close(u));printf("Original shop background: %u complete command/font/icon/tint comparisons pass\n",cases);return 0;
}
