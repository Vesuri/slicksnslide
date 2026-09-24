#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/track_info.h"

struct PreviewDirty { short left,top,right,bottom; unsigned calls; };
static void preview_dirty(void *context,short l,short t,short r,short b)
{ struct PreviewDirty *d=context; d->left=l; d->top=t; d->right=r; d->bottom=b; ++d->calls; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f); if(size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200); word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
    struct Vga v={0}; uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    const unsigned dims[][2]={{1,1},{4,5},{5,4},{6,9},{15,16},{17,21},{32,32},{64,41},{127,113},{320,190}};
    const short origins[][2]={{245,20},{0,0},{-8,-7},{310,190},{-65,80},{100,200}};
    static unsigned char source[64000],sprite[65000],pixels[64000]; unsigned cases=0;
    for(unsigned shape=0;shape<sizeof dims/sizeof dims[0];++shape)
    for(unsigned rotation=0;rotation<4;++rotation) {
        unsigned width=dims[shape][0],height=dims[shape][1];
        for(unsigned i=0;i<width*height;++i) source[i]=i%7?(unsigned char)(1+i%253):0;
        unsigned w=rotation&1?height:width,h=rotation&1?width:height;
        /* Original sprite format has byte height. Oversized rotated height
         * cannot be encoded and is deliberately not an oracle fixture. */
        if(h>255) continue;
        unsigned stride=(w+3)/4,padded=4*stride;
        memset(sprite,0,sizeof sprite); sprite[0]=stride; sprite[1]=h;
        sprite[2+padded*h]=(unsigned char)(padded-w);
        /* Forward rotation into independent DOS four-bank storage. */
        for(unsigned sy=0;sy<height;++sy) for(unsigned sx=0;sx<width;++sx) {
            unsigned dx=sx,dy=sy;
            if(rotation==1) { dx=height-1-sy; dy=sx; }
            else if(rotation==2) { dx=width-1-sx; dy=height-1-sy; }
            else if(rotation==3) { dx=sy; dy=width-1-sx; }
            sprite[2+(dx&3)*stride*h+dy*stride+dx/4]=source[sy*width+sx];
        }
        check(uc_mem_write(u,0x50000,sprite,2+padded*h+1));
        for(unsigned origin=0;origin<sizeof origins/sizeof origins[0];++origin) {
            memset(pixels,254,sizeof pixels); memset(v.pixels,254,sizeof v.pixels);
            short x=origins[origin][0],y=origins[origin][1];
            uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            word(u,0x8f000,0); word(u,0x8f002,0x9000);
            word(u,0x8f004,x); word(u,0x8f006,y); word(u,0x8f008,0); word(u,0x8f00a,0x5000);
            word(u,0x8f00c,0); word(u,0x8f00e,5); word(u,0x8f010,0);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x35719,0x90000,0,10000000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); if(ip || sp!=0xf004) abort();
            struct PreviewDirty dirty={0}; struct SlicksChunkyUi ui={pixels,0,preview_dirty,&dirty};
            if(slicks_track_preview_sprite(&ui,source,width,height,rotation,x,y)) abort();
            if(memcmp(pixels,v.pixels,sizeof pixels)) {
                for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) {
                    fprintf(stderr,"Preview mismatch shape=%u rotation=%u origin=%u pixel=%u,%u native=%u DOS=%u\n",shape,rotation,origin,i%320,i/320,pixels[i],v.pixels[i]); break;
                }
                return 1;
            }
            short l=320,t=200,r=0,b=0;
            for(unsigned py=0;py<200;++py) for(unsigned px=0;px<320;++px) if(pixels[py*320+px]!=254) {
                if((short)px<l) l=(short)px; if((short)py<t) t=(short)py;
                if((short)(px+1)>r) r=(short)(px+1); if((short)(py+1)>b) b=(short)(py+1);
            }
            if(dirty.calls!=(unsigned)(l<r && t<b) || (dirty.calls &&
                (dirty.left!=l || dirty.top!=t || dirty.right!=r || dirty.bottom!=b))) abort();
            ++cases;
        }
    }
    check(uc_close(u)); printf("Original one-fifth preview scaler: %u full-screen comparisons and exact producer dirty bounds pass\n",cases); return 0;
}
