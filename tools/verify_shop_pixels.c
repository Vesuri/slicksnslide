#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/shop_draw.h"
#include "../src/ui/track_records_renderer.h"

struct ShopPixels {
    struct SlicksPlayerMenuRenderer surface;
    unsigned char tint[256];
};
static void shop_text(void *p,unsigned font,const unsigned char *s,short x,short y,unsigned char flags)
{ struct ShopPixels *r=p;native_text(r->surface.context,font,s,x,y,flags); }
static void shop_number(void *p,unsigned font,short value,short x,short y,unsigned char flags)
{ unsigned char s[7];slicks_records_decimal(value,s);shop_text(p,font,s,x,y,flags); }
static void shop_colour(void *p,unsigned font,unsigned char r,unsigned char g,unsigned char b)
{ struct ShopPixels *s=p;s->surface.fonts[font][6]=slicks_ui_nearest(&s->surface.ui,r,g,b); }
static void shop_tint(void *p,short l,short t,short r,short b)
{ struct ShopPixels *s=p;if(slicks_ui_remap(&s->surface.ui,l,t,r,b,s->tint))abort(); }
static void shop_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ slicks_player_renderer_restore(&((struct ShopPixels *)p)->surface,x,y,sx,sy,w,h); }
static void shop_bevel(void *p,short x,short y,short w,short h,unsigned char r,unsigned char g,unsigned char b)
{ slicks_ui_bevel(&((struct ShopPixels *)p)->surface.ui,x,y,w,h,r,g,b); }
static void shop_rectangle(void *p,short l,short t,short r,short b,unsigned char c)
{ slicks_ui_rectangle(&((struct ShopPixels *)p)->surface.ui,l,t,r,b,c); }
static void shop_sprite(void *p,short id,short left,short top)
{
    struct ShopPixels *s=p;const struct SlicksMenuIcon *i=&s->surface.icons[id];
    for(unsigned y=0;y<i->height;++y)for(unsigned x=0;x<i->width;++x)
        if(i->pixels[y*i->width+x])s->surface.ui.pixels[mult320[top+y]+left+x]=i->pixels[y*i->width+x];
}
static void shop_resource(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size;(void)p;uint16_t ss,sp,ip,cs;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));unsigned at=16U*ss+sp;
    if(address==0x2e0f8) {
        unsigned char name[16];check(uc_mem_read(u,getword(u,at+4)+16U*getword(u,at+6),name,sizeof name));
        unsigned id=99;
        for(unsigned i=0;i<14;++i)if(name[i]>='0'&&name[i]<='9'&&name[i+1]>='0'&&name[i+1]<='9') {id=10*(name[i]-'0')+name[i+1]-'0';break;}
        if(id>=13)abort();uint16_t ax=id*0x800,dx=0x6500;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    }
    ip=getword(u,at);cs=getword(u,at+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void registers(uc_engine *u,unsigned sp)
{
    uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,s=(uint16_t)sp,bp=0xf000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));
    check(uc_reg_write(u,UC_X86_REG_SP,&s));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
}
static void compare(struct ShopPixels *s,struct Vga *v,unsigned test,const char *stage)
{
    if(!memcmp(s->surface.ui.pixels,v->pixels,64000))return;
    for(unsigned i=0;i<64000;++i)if(s->surface.ui.pixels[i]!=v->pixels[i]) {
        fprintf(stderr,"Shop %s pixels test=%u at %u,%u native=%u DOS=%u\n",stage,test,i%320,i/320,s->surface.ui.pixels[i],v->pixels[i]);break;
    }
    exit(1);
}
int main(void)
{
    static unsigned char runtime[300000],source[70000],base[64000],palette[768],fonts[2][8192],native[64000],saved[64000],snapshot[64002];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    unsigned ds=0x3cbf0;const unsigned char *data=runtime+ds-0x10100;
    long loaded=host_archive_load("ref/SLICKS.000","tuning.@I",source,sizeof source);unsigned short width,height;
    if(loaded<0 || slicks_decode_indexed_menu_icon(source,(unsigned long)loaded,base,sizeof base,&width,&height)||width!=320||height!=200)abort();
    if(host_archive_load("ref/SLICKS.000","tuning.@p",palette,sizeof palette)!=sizeof palette)abort();
    struct NativeText n={0};const char *font_names[]={"kirj.@f","pieni.@f"};
    for(unsigned i=0;i<2;++i) {
        loaded=host_archive_load("ref/SLICKS.000",font_names[i],source,sizeof source);
        long size=loaded<0?-1:slicks_decode_font_resource(source,loaded,fonts[i],sizeof fonts[i]);if(size<0)abort();
        n.fonts[i]=fonts[i];n.sizes[i]=(unsigned)size;
        word(u,ds+0x680+4*i,0x2000*i);word(u,ds+0x682+4*i,0x6000);
    }
    f=fopen("build/font_string_test.bin","rb");if(!f)abort();bytes=fread(source,1,sizeof source,f);fclose(f);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));check(uc_mem_write(n.cpu,0,source,bytes));
    n.records_bridge=(unsigned)source[20]<<24|(unsigned)source[21]<<16|(unsigned)source[22]<<8|source[23];
    n.pixels=native;n.spacing=1;n.tab=10;n.shadow=0x100;
    struct Vga v={0};uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,shop_resource,0,0x2e0f8,0x2e0f8));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,shop_resource,0,0x139fd,0x139fd));
    check(uc_mem_write(u,ds+0x71bc,palette,sizeof palette));word(u,ds+0x71b8,0x71bc);word(u,ds+0x71ba,0x3cbf);
    word(u,ds+0x68ba,0x71bc);word(u,ds+0x68bc,0x3cbf);
    word(u,ds+0x1d7b,100);word(u,ds+0x1d87,0);word(u,ds+0x1d8d,0);word(u,ds+0x1d8f,200);word(u,ds+0x1d91,0);word(u,ds+0x1d93,79);
    word(u,ds+0x5b8,0);word(u,ds+0x5ba,0x9000);
    static unsigned char icon_pixels[23][256],raw_icons[23][1024];unsigned raw_sizes[23];struct SlicksMenuIcon icons[23];
    for(unsigned i=0;i<23;++i) {
        char name[32];if(i<13)snprintf(name,sizeof name,"vir%02u.@16",i);else if(i==13)strcpy(name,"carimage16");else snprintf(name,sizeof name,"auto%02u.@16",i-13);
        loaded=host_archive_load("ref/SLICKS.000",name,source,sizeof source);
        if(loaded<0||slicks_decode_menu_icon(source,loaded,palette,icon_pixels[i],256,&icons[i].width,&icons[i].height))abort();
        icons[i].pixels=icon_pixels[i];unsigned char *raw=raw_icons[i];raw[0]=0;raw[1]=0xb1;raw[2]=source[5];raw[3]=0;raw[4]=source[7];raw[5]=0;
        memcpy(raw+6,source+8,loaded-8);raw_sizes[i]=loaded-2;
        if(i>=13){word(u,ds+0x4e44+4*(i-13),i*0x800);word(u,ds+0x4e46+4*(i-13),0x6500);}
    }
    struct SlicksShopRules rules;
    memcpy(rules.flags,data+0x106f,13);memcpy(rules.capacity,data+0x10a4,13);memcpy(rules.batch,data+0x107c,13);
    memcpy(rules.vehicle_capacity,data+0x1aa,10);memcpy(rules.weapon_weight,data+0x13e,8);
    for(unsigned i=0;i<13;++i){rules.base_price[i]=(short)getword(u,ds+0x108a+2*i);rules.ammunition_price[i]=(short)getword(u,ds+0x17c+2*i);}
    const unsigned char names[4][21]={"Driver one","Computer two","Third","Fourth driver"};
    struct ShopPixels painter={0};painter.surface.ui=(struct SlicksChunkyUi){native,palette,0,0};painter.surface.saved=saved;painter.surface.context=&n;painter.surface.icons=icons;
    for(unsigned i=0;i<2;++i)painter.surface.fonts[i]=fonts[i];slicks_ui_tint_table(palette,painter.tint,10,10,30,50);
    struct SlicksShopStaticOps ops={{{shop_restore,shop_bevel,shop_sprite,shop_text,&painter},shop_number,shop_rectangle},shop_colour,shop_tint};
    unsigned cases=0;
    for(unsigned trial=0;trial<32;++trial) {
        struct SlicksSetupSession s={0};unsigned flags=trial%16,mask=1+trial%15;
        s.options=(struct SlicksRaceOptions){.weapons_enabled=flags&1,.inventory_mode=flags&2,.fuel=flags&4,.damage=flags&8};
        word(u,ds+0x3020,s.options.weapons_enabled);word(u,ds+0x3022,s.options.inventory_mode);word(u,ds+0x3024,s.options.fuel);word(u,ds+0x3026,s.options.damage);
        struct SlicksShopContent c={.session=&s,.rules=&rules,.items=(const unsigned char (*)[15])(data+0xfac),.footer=data+0x1531,.exit_label=data+0x1523,
            .register_label=data+0x1575,.separator=data+0x1307,.extra=(trial/16)&1,.track=(short)(trial+1),.total=195};
        check(uc_mem_write(u,ds+0x62f,&c.extra,1));word(u,ds+0x628,trial);word(u,ds+0x90,c.total);
        for(unsigned d=0;d<4;++d) {
            s.players.selected[d]=mask&(1<<d)?d+1:0;s.players.participation[d]=s.players.selected[d]?((d&1)?1:-1):0;s.players.vehicle[d]=(trial+d)%10;
            s.players.count+=s.players.selected[d]!=0;c.names[d]=names[d];s.cash[d]=1000-199*d;
            word(u,ds+0x44c+2*d,s.players.selected[d]);word(u,ds+0x4bf6+2*d,s.cash[d]);
            check(uc_mem_write(u,ds+0x4bc6+d,&s.players.participation[d],1));check(uc_mem_write(u,ds+0x4bc2+d,&s.players.vehicle[d],1));check(uc_mem_write(u,ds+0x36aa+21*(d+1),names[d],21));
            for(unsigned i=0;i<13;++i){s.inventory[d][i]=(trial+i+d)%(rules.capacity[i]+1);word(u,ds+0x6a7a+26*d+2*i,s.inventory[d][i]);}
        }
        check(uc_mem_write(u,ds+0x4c16,&s.players.count,1));
        signed char rows=0;while(rows<13&&slicks_shop_item(&rules,&s.options,s.inventory[0],s.players.participation[0],s.players.vehicle[0],c.extra,rows)>=0)++rows;
        memcpy(native,base,64000);memcpy(v.pixels,base,64000);
        for(unsigned i=0;i<2;++i)check(uc_mem_write(u,0x60000+0x2000*i,fonts[i],n.sizes[i]));
        for(unsigned i=0;i<23;++i)check(uc_mem_write(u,0x65000+0x800*i,raw_icons[i],raw_sizes[i]));
        registers(u,0xeb00);check(uc_emu_start(u,0x2ca23,0x2cf00,0,30000000));
        if(slicks_draw_shop_background(&c,rows,&ops))abort();compare(&painter,&v,trial,"background");++cases;
        memcpy(saved,native,64000);snapshot[0]=80;snapshot[1]=200;
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)snapshot[2+(x&3)*16000+y*80+x/4]=v.pixels[y*320+x];
        check(uc_mem_write(u,0x90000,snapshot,sizeof snapshot));
        for(signed char row=0;row<=rows;++row) {
            short args[]={trial%s.players.count,row,-1,-1,rows,slicks_ui_nearest(&painter.surface.ui,50,10,10),slicks_ui_nearest(&painter.surface.ui,70,70,10)};
            registers(u,0xe000);word(u,0x8e000,0);word(u,0x8e002,0x5000);for(unsigned i=0;i<7;++i)word(u,0x8e004+2*i,args[i]);
            check(uc_emu_start(u,0x2c574,0x50000,0,30000000));
            if(slicks_draw_shop_values(&s,&rules,c.extra,args[0],row,-1,-1,rows,args[5],args[6],c.exit_label,&ops.draw)<0)abort();
            compare(&painter,&v,trial,"values");++cases;
        }
    }
    check(uc_close(n.cpu));check(uc_close(u));printf("Original shop: %u full-screen comparisons with original assets and x86/68020 text pass\n",cases);return 0;
}
