#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/title_navigation.h"

int main(void)
{
    unsigned char runtime[300000];
    FILE *file=fopen("disasm/runtime.bin","rb"); if(!file) return 2;
    size_t size=fread(runtime,1,sizeof runtime,file); fclose(file);
    if(size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    const short edges[]={-32768,-1,0,1,4,5,6,194,195,32767};
    unsigned cases=0;
    for(unsigned key=0;key<256;++key) for(short row=0;row<7;++row)
    for(unsigned pattern=0;pattern<10;++pattern) {
        short selected=row,count=edges[pattern],mode=edges[(pattern+3)%10];
        short total=edges[(pattern+5)%10]; unsigned char refresh=71;
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,key); word(u,0x8f006,0); word(u,0x8f008,0x7000);
        word(u,0x70000,row); word(u,0x3cbf0+0x90,(unsigned short)count);
        word(u,0x3cbf0+0x92,(unsigned short)mode); word(u,0x3cbf0+0x4da8,(unsigned short)total);
        check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        check(uc_emu_start(u,0x2a0cb,0x90000,0,300));
        uint16_t ip; check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        slicks_title_navigation(&selected,&count,total,&mode,&refresh,key);
        unsigned char actual; check(uc_mem_read(u,0x3cbf0+0x1146,&actual,1));
        if(ip || readword(u,0x70000)!=(unsigned short)selected ||
           readword(u,0x3cbf0+0x90)!=(unsigned short)count ||
           readword(u,0x3cbf0+0x92)!=(unsigned short)mode || actual!=refresh) {
            fprintf(stderr,"Title navigation mismatch key=%u row=%d pattern=%u\n",key,row,pattern); return 1;
        }
        ++cases;
    }
    unsigned mode_cases=0;
    for(unsigned key=0;key<256;++key) for(unsigned r=0;r<10;++r)
    for(unsigned pattern=0;pattern<10;++pattern) for(unsigned arcade=0;arcade<2;++arcade) {
        short selected=edges[r],count=edges[pattern],mode=arcade?5:4;
        short players=edges[(pattern+3)%10],total=195;
        unsigned char refresh=71;
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0);word(u,0x8f002,0x9000);
        word(u,0x8f004,key);word(u,0x8f006,0);word(u,0x8f008,0x7000);
        word(u,0x70000,(unsigned short)selected);word(u,0x3cbf0+0x90,(unsigned short)count);
        word(u,0x3cbf0+0x92,(unsigned short)mode);word(u,0x3cbf0+0x4da8,total);
        word(u,0x3cbf0+0xf1a,(unsigned short)players);
        check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        check(uc_emu_start(u,0x2a25b,0x90000,0,400));
        slicks_title_mode_navigation(&selected,&count,total,&mode,&players,&refresh,key);
        unsigned char actual;check(uc_mem_read(u,0x3cbf0+0x1146,&actual,1));
        if(readword(u,0x70000)!=(unsigned short)selected ||
           readword(u,0x3cbf0+0x90)!=(unsigned short)count ||
           readword(u,0x3cbf0+0x92)!=(unsigned short)mode ||
           readword(u,0x3cbf0+0xf1a)!=(unsigned short)players || actual!=refresh) {
            fprintf(stderr,"Mode title navigation mismatch key=%u row=%u pattern=%u arcade=%u\n",key,r,pattern,arcade);return 1;
        }
        ++mode_cases;
    }
    unsigned action_cases=0;
    for(unsigned m=0;m<10;++m) for(unsigned r=0;r<65536;++r) {
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0);word(u,0x8f002,0x9000);word(u,0x8f004,r);
        word(u,0x3cbf0+0x92,(unsigned short)edges[m]);
        check(uc_emu_start(u,0x2a28f,0x90000,0,100));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(ax!=(unsigned short)slicks_title_action_selection(edges[m],(short)r)) {
            fprintf(stderr,"Title action mapping mismatch mode=%d row=%u\n",edges[m],r);return 1;
        }
        ++action_cases;
    }
    /* Reachable title selections, not arbitrary injected rows. Start at the
     * original title-entry row zero in every mode. Permit Options to return
     * any mode while preserving its caller's selection, and feed every scan
     * through the original mode-navigation routine. Count/player edits cannot
     * affect selection routing; use valid values for those independent fields.
     * This is a title-owner boundary proof, not whole-program reachability. */
    /* Pin the actual shortcut table used by 2a3ac so activation scans in the
     * traversal cannot silently drift from the supplied executable. */
    const unsigned short shortcut_scans[]={1,0x1c,0x1d,0x39,0x3b,0x43,0x44,0x58};
    const unsigned long shortcut_targets[]={0x2a562,0x2a4cc,0x2a4cc,0x2a4cc,
        0x2a3cc,0x2a4c5,0x2a562,0x2a3db};
    for(unsigned i=0;i<8;++i)
        if(readword(u,0x2a61e + 2*i)!=shortcut_scans[i] ||
           0x266c0UL+readword(u,0x2a62e + 2*i)!=shortcut_targets[i]) {
            fprintf(stderr,"Original title shortcut table changed\n");return 1;
        }
    unsigned char reached[6][7]={{0}};
    for(unsigned mode=0;mode<6;++mode) reached[mode][0]=1;
    unsigned changed=1,reachable_cases=0,states=0;
    while(changed) {
        changed=0;
        for(unsigned mode=0;mode<6;++mode) for(unsigned row=0;row<7;++row) {
            if(!reached[mode][row]) continue;
            for(unsigned scan=0;scan<256;++scan) {
                uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
                check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
                check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
                word(u,0x8f000,0);word(u,0x8f002,0x9000);
                word(u,0x8f004,scan);word(u,0x8f006,0);word(u,0x8f008,0x7000);
                word(u,0x70000,row);word(u,0x3cbf0+0x90,100);
                word(u,0x3cbf0+0x92,mode);word(u,0x3cbf0+0x4da8,195);
                word(u,0x3cbf0+0xf1a,2);
                check(uc_emu_start(u,0x2a25b,0x90000,0,400));
                unsigned next_row=readword(u,0x70000),next_mode=readword(u,0x3cbf0+0x92);
                if(next_mode>=6 || next_row>=7 || next_row==4) {
                    fprintf(stderr,"Unexpected reachable title state mode=%u row=%u scan=%u -> %u/%u\n",
                        mode,row,scan,next_mode,next_row);return 1;
                }
                if(!reached[next_mode][next_row]) {reached[next_mode][next_row]=1;changed=1;}
                /* Execute original action mapping too; Arcade row 1 is Options. */
                check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
                word(u,0x8f004,next_row);
                check(uc_emu_start(u,0x2a28f,0x90000,0,100));
                check(uc_reg_read(u,UC_X86_REG_AX,&ax));
                if(ax==4) {fprintf(stderr,"Reachable Load Game title action\n");return 1;}
                if(ax==3 && (scan==0x1c || scan==0x1d || scan==0x39))
                    for(unsigned returned_mode=0;returned_mode<6;++returned_mode)
                        if(!reached[returned_mode][next_row]) {reached[returned_mode][next_row]=1;changed=1;}
                ++reachable_cases;
            }
        }
    }
    for(unsigned mode=0;mode<6;++mode) for(unsigned row=0;row<7;++row) states+=reached[mode][row];
    printf("Original title reachability: %u states, %u scan transitions; hidden Load row/action unreachable within audited owner boundaries\n",
        states,reachable_cases);
    uc_close(u);
    printf("Original title navigation: %u input/selection/count/mode/refresh comparisons pass\n",cases);
    printf("Original title mode dispatch: %u navigation and %u action-map comparisons pass\n",mode_cases,action_cases);
    return 0;
}
