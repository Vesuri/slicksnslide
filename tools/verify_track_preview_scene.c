#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
/* Reuse the already established DAT decoder for the oracle's resource-bank
 * fixture only. Original object selection, positioning, scaling and drawing
 * execute independently; this gate does not prove DAT decompression. */
#include "../src/game/track_scene.c"
static struct TrackSprite fixtures[SLICKS_SPRITE_COUNT];
static unsigned char bank_rotations[440];
static unsigned bank_cursor=110;
static unsigned char oracle_pixels[64000],native_pixels[64000];
static void pixels_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address!=0x13b07 && address!=0x139fd && address!=0x3528a &&
        address!=0x39ed8 && address!=0x3b55e && address!=0x35643) { boundary(u,address,size,p); return; }
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x13b07) { REQUIRE(get(u,stack+4)==30000); dx=0x6000; }
    else if(address==0x35643) {
        unsigned index=get(u,stack+4); REQUIRE(index>=1 && index<=110 && bank_cursor<440);
        REQUIRE(index-1==bank_cursor%110);
        bank_rotations[bank_cursor++]=(unsigned char)get(u,stack+6); ax=1;
    }
    else if(address==0x3528a) {
        unsigned index=get(u,stack+8);
        if(index<1 || index>440) fprintf(stderr,"Resource index %u (base=%u count=%u)\n",index,get(u,0x3cbf0+0x53ac),get(u,0x3cbf0+0x53b0));
        REQUIRE(index>=1 && index<=440); --index;
        const struct TrackSprite *s=&fixtures[index%110]; unsigned rotation=bank_rotations[index];
        unsigned w=rotation&1?s->height:s->width,h=rotation&1?s->width:s->height;
        unsigned stride=(w+3)/4,padded=4*stride;
        unsigned char bank[30000]={0}; REQUIRE(3+padded*h<=sizeof bank && h<=255);
        bank[0]=stride; bank[1]=h; bank[2+padded*h]=padded-w;
        for(unsigned sy=0;sy<s->height;++sy) for(unsigned sx=0;sx<s->width;++sx) {
            unsigned x=sx,y=sy;
            if(rotation==1) { x=s->height-1-sy; y=sx; }
            else if(rotation==2) { x=s->width-1-sx; y=s->height-1-sy; }
            else if(rotation==3) { x=sy; y=s->width-1-sx; }
            bank[2+(x&3)*stride*h+y*stride+x/4]=s->pixels[sy*s->width+sx];
        }
        REQUIRE(get(u,stack+4)==0 && get(u,stack+6)==0x6000);
        check(uc_mem_write(u,0x60000,bank,3+padded*h));
    } else if(address==0x39ed8) {
        struct SlicksChunkyUi ui={oracle_pixels,0,0,0};
        slicks_ui_rectangle(&ui,(short)get(u,stack+4),(short)get(u,stack+6),
            (short)get(u,stack+8),(short)get(u,stack+10),(unsigned char)get(u,stack+12));
    } else if(address==0x3b55e) {
        short x=(short)get(u,stack+4),y=(short)get(u,stack+6);
        if(x>=0 && x<320 && y>=0 && y<200) oracle_pixels[mult320[(unsigned)y]+x]=(unsigned char)get(u,stack+8);
    }
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],dat[65536],oracle_arena[512000],native_arena[512000];
    FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); check(uc_mem_write(u,0x65000,"BASIC",6));
    word(u,0x3cbf0+0x53ac,1); word(u,0x3cbf0+0x53b0,110);
    const unsigned addresses[]={0x11eaf,0x1219b,0x12d8a,0x119c2,0x36c97,0x301ab,0x39ed8,0x3b55e,0x13b07,0x139fd,0x3528a,0x35643};
    for(unsigned i=0;i<sizeof addresses/sizeof addresses[0];++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,pixels_boundary,0,addresses[i],addresses[i]));
    }
    /* Execute the original bank-construction decisions, including its six
     * special scenery types, before serving decoded resource fixtures. */
    check(uc_mem_write(u,0x8eefa,runtime+0x3cbf0+0x777-0x10100,6));
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xee00,bp=0xef00;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_emu_start(u,0x1bf20,0x1bf96,0,1000000));
    uint16_t ip; check(uc_reg_read(u,UC_X86_REG_IP,&ip)); REQUIRE(ip==0x1bf96-0x19870 && bank_cursor==440);
    f=fopen("ref/SLICKS.DAT","rb"); REQUIRE(f); unsigned dat_size=(unsigned)fread(dat,1,sizeof dat,f); fclose(f);
    REQUIRE(dat_size<sizeof dat && !decode_dat_images(dat,dat_size,oracle_arena,sizeof oracle_arena,fixtures));
    DIR *dir=opendir("ref/TRACKS"); REQUIRE(dir); struct dirent *entry; unsigned tracks=0,rejected=0;
    origin_x=245; origin_y=20;
    while((entry=readdir(dir))) {
        size_t len=strlen(entry->d_name); if(len<3 || strcmp(entry->d_name+len-3,".SS")) continue;
        char path[1024]; snprintf(path,sizeof path,"ref/TRACKS/%s",entry->d_name);
        f=fopen(path,"rb"); REQUIRE(f); length=(unsigned)fread(bytes,1,sizeof bytes,f); fclose(f); REQUIRE(length<sizeof bytes);
        for(unsigned i=0;i<64000;++i) oracle_pixels[i]=native_pixels[i]=(unsigned char)(i*17+i/320);
        struct SlicksChunkyUi ui={native_pixels,0,0,0};
        struct SlicksTrackPreview candidate;
        REQUIRE(!slicks_track_preview_open(&candidate,bytes,length));
        unsigned unsupported=0;
        for(unsigned i=0;i<candidate.count;++i)
            if(candidate.objects[5*i+3]<110 && candidate.objects[5*i+4]>3) ++unsupported;
        if(unsupported) {
            REQUIRE(!strcmp(entry->d_name,"RAILROAD.SS") && unsupported==5);
            REQUIRE(slicks_build_track_preview(&ui,dat,dat_size,bytes,length,native_arena,sizeof native_arena,origin_x,origin_y)==-1);
            REQUIRE(!memcmp(oracle_pixels,native_pixels,64000)); ++rejected; continue;
        }
        run(u,0x1a32d);
        REQUIRE(!slicks_build_track_preview(&ui,dat,dat_size,bytes,length,native_arena,sizeof native_arena,origin_x,origin_y));
        if(memcmp(oracle_pixels,native_pixels,64000)) {
            for(unsigned i=0;i<64000;++i) if(oracle_pixels[i]!=native_pixels[i]) {
                fprintf(stderr,"Track preview %s at %u,%u: native=%u DOS=%u\n",entry->d_name,i%320,i/320,native_pixels[i],oracle_pixels[i]); break;
            }
            return 1;
        }
        ++tracks;
    }
    closedir(dir); REQUIRE(tracks==194 && rejected==1); check(uc_close(u));
    printf("Original preview composition: %u actual tracks match all 64000 pixels using DAT-derived resource fixtures; RAILROAD's five unsupported bank selectors rejected without drawing\n",tracks); return 0;
}
