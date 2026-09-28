#include <stdio.h>
#include <string.h>
#include "../src/platform/amiga/amiga_key_scan.h"
#include "../src/ui/controllers_dialog.h"
#include "../src/game/driver_input.h"
#include "../src/gen/setup_defaults.h"
#include "../src/ui/track_menu.h"
#include "../src/ui/list_dialog.h"

int main(void)
{
    unsigned cases=0;
    for(unsigned index=1;index<69;++index) {
        unsigned scan=slicks_original_controller_scans[index],raw=0;
        while(raw<128 && amiga_raw_to_dos_scan((unsigned short)raw)!=scan) ++raw;
        if(raw==128) { fprintf(stderr,"Original binding %u has no Amiga key\n",scan); return 1; }
        for(unsigned group=0;group<4;++group) for(unsigned action=0;action<5;++action) {
            struct SlicksConfiguration c=slicks_original_configuration;
            memset(c.keys,0,sizeof c.keys);
            struct SlicksControllersDialog d={group,action+1,0,1,0};
            if(slicks_controllers_capture(&d,&c,slicks_original_controller_scans,
                amiga_raw_to_dos_scan(raw)) || c.keys[group*5+action]!=scan) return 1;
            unsigned char saved[142]; struct SlicksConfiguration restored=slicks_original_configuration;
            c.player_input[group]=(unsigned char)(action%3);
            if(slicks_save_configuration(&c,saved,sizeof saved,0xa1)!=142 ||
                slicks_load_configuration(&restored,saved,sizeof saved,0xa1,0,0,0)!=1 ||
                memcmp(restored.keys,c.keys,20) || memcmp(restored.player_input,c.player_input,4)) return 1;
            unsigned char controls[4]={0},order[4]={2,0,3,1};
            slicks_driver_key(controls,restored.keys,order,amiga_raw_to_dos_scan(raw));
            for(unsigned i=0;i<4;++i)
                if(controls[i]!=(i==order[group]?(1U<<action):0)) return 1;
            slicks_driver_key(controls,restored.keys,order,(unsigned char)(amiga_raw_to_dos_scan(raw|128)|128));
            for(unsigned i=0;i<4;++i) if(controls[i]) return 1;
            ++cases;
        }
    }
    const unsigned char letters[]={0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,
        0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31};
    const unsigned char pc[]={0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
        0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    for(unsigned i=0;i<26;++i) if(amiga_raw_to_dos_scan(letters[i])!=pc[i]) return 1;
    for(unsigned i=0;i<10;++i) if(amiga_raw_to_dos_scan(0x50+i)!=0x3b+i) return 1;
    for(unsigned raw=0;raw<128;++raw)
        if(amiga_raw_to_dos_scan(raw)!=amiga_raw_to_dos_scan(raw|128)) return 1;
    if(amiga_raw_to_dos_scan(0x66) || amiga_raw_to_dos_scan(0x67) || amiga_raw_to_dos_scan(0x7f)) return 1;
    for(unsigned raw=0;raw<256;++raw) {
        unsigned key=raw&127;
        unsigned expected=key==0x1a?0x49:key==0x1b?0x51:amiga_raw_to_dos_scan(raw);
        if(amiga_raw_to_menu_scan(raw)!=expected) return 1;
    }
    if(amiga_raw_to_dos_scan(0x1a)!=0x1a || amiga_raw_to_dos_scan(0x1b)!=0x1b) return 1;
    const unsigned char page_raw[]={0x3f,0x1a,0x1f,0x1b};
    for(unsigned i=0;i<4;++i) {
        unsigned char scan=(unsigned char)amiga_raw_to_menu_scan(page_raw[i]);
        struct SlicksTrackMenu track={.cursor=50};
        struct SlicksListDialog list={.selected=30,.count=100,.visible=10};
        slicks_track_menu_key(&track,195,scan);
        slicks_list_dialog_key(&list,scan);
        if(track.cursor!=(i<2?29:71) || list.selected!=(i<2?21:39) ||
           list.redraw_list!=255) return 1;
    }
    unsigned help_cases=0;
    for(unsigned raw=0;raw<256;++raw) for(unsigned ascii=0;ascii<256;++ascii) {
        struct SlicksAmigaHelpKey key=slicks_amiga_help_key(raw,ascii);
        unsigned expected_ascii=0,expected_scan=0;
        if(raw<128) {
            if(raw==0x3f || raw==0x1a) expected_scan=0x49;
            else if(raw==0x1f || raw==0x1b) expected_scan=0x51;
            else if(ascii) expected_ascii=ascii;
            else if(raw<0x60) expected_scan=amiga_raw_to_dos_scan(raw);
        }
        if(key.ascii!=expected_ascii || key.scan!=expected_scan || (key.ascii && key.scan)) return 1;
        ++help_cases;
    }
    printf("Amiga Help keys: %u ASCII/extended/page-key/release cases pass\n",help_cases);
    puts("Amiga menu keys: 256 mappings and all four aliases in track/list paging pass; physical bindings unchanged");
    printf("Amiga controller keys: all 68 original assignable scans reachable; %u capture/CFG roundtrip/ordered-driver press/release cases pass\n",cases);
    return 0;
}
