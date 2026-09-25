#define main menu_icon_verifier_main
#include "verify_menu_icon.c"
#undef main
#include "../src/game/track_actor_assets.h"
int main(void)
{
    static unsigned char runtime[300000],arena[65536];
    static struct SlicksTrackActorAsset images[14];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};uc_hook h;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)
        check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    f=fopen("ref/SLICKS.DAT","rb");if(!f)return 2;
    length=fread(source,1,sizeof source,f);fclose(f);
    if(slicks_decode_track_actor_assets(source,length,arena,sizeof arena,images))return 1;
    unsigned verified=0;
    for(unsigned image=0;image<119;++image) {
        if(image==110) {
            while(cursor+3<=length && memcmp(source+cursor,"\x12\x34\0",3)) ++cursor;
            if(cursor+3>length)abort();cursor+=3;
        }
        allocations=0;
        word(u,0x8f004,0);word(u,0x8f006,0x9000);word(u,0x8f008,0);word(u,0x8f00a,0x6000);word(u,0x8f00c,2);
        start(u,0x2e51a);
        int index=-1;
        const unsigned base[5]={79,80,81,82,89};
        for(unsigned i=0;i<5;++i)if(image==base[i])index=i;
        if(image>=110)index=image-105;
        if(index<0)continue;
        struct SlicksTrackActorAsset *a=&images[index];
        unsigned char decoded[1024];check(uc_mem_read(u,0x50000,decoded,sizeof decoded));
        unsigned stride=(a->width+3)/4;
        if(decoded[0]!=stride || decoded[1]!=a->height) {
            fprintf(stderr,"Frame %u header %u,%u,%u expected %u,%u\n",image,
                decoded[0],decoded[1],decoded[2],a->width,a->height);return 1;
        }
        for(unsigned y=0;y<a->height;++y)for(unsigned x=0;x<a->width;++x)
            if(a->pixels[y*a->width+x]!=decoded[2+(x&3)*stride*a->height+y*stride+x/4]) {
                fprintf(stderr,"Track actor asset %u pixel %u,%u differs\n",index,x,y);return 1;
            }
        ++verified;
    }
    for(unsigned cut=0;cut<length-3;++cut)
        if(!slicks_decode_track_actor_assets(source,cut,arena,sizeof arena,images)) {
            fprintf(stderr,"Truncation %u accepted of %u\n",cut,length);return 1;
        }
    printf("Track actor assets: %u original decoded frames match; truncated inputs rejected\n",verified);
    check(uc_close(u));return verified==14?0:1;
}
