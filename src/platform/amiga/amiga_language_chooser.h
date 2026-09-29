#ifndef SLICKS_AMIGA_LANGUAGE_CHOOSER_H
#define SLICKS_AMIGA_LANGUAGE_CHOOSER_H
/* Included by the startup owner and its host API-fault harness. Keep console
 * lifetime logic identical; the harness substitutes only AmigaDOS calls. */
static int choose_startup_language(struct SlicksResourceArchive *archive)
{
    static unsigned char resource[512],labels[8][40];
    unsigned char count=0,selected=0;
    for(unsigned i=1;i<=8;++i) {
        char name[10];
        if(slicks_language_resource(name,i)) return -1;
        long size=slicks_resource_archive_load(archive,name,resource,sizeof resource);
        if(size<0) break;
        unsigned n=0;
        while(n<39 && n<(unsigned long)size && resource[n] && resource[n]!='\n' && resource[n]!='\r') {
            labels[count][n]=resource[n]; ++n;
        }
        labels[count++][n]=0;
    }
    if(!count) return 1;
    BPTR input=Input();
    if(language_choice_test!=1) {
        if(!IsInteractive(input) || !SetMode(input,1)) return -1;
        if(language_choice_test) g_slicks_language_console_modes|=1;
    }
    PutStr((CONST_STRPTR)"Choose language: (Up/Down, Enter)\n");
    int result=-1; unsigned step=0;
    for(;;) {
        for(unsigned i=0;i<count;++i) {
            PutStr((CONST_STRPTR)(i==selected?"> ":"  "));
            PutStr((CONST_STRPTR)labels[i]); PutStr((CONST_STRPTR)"\n");
        }
        unsigned char key;
        if(language_choice_test==1) {
            static const unsigned char keys[]={72,80,80,72,27};
            if(step>=sizeof keys) break;
            key=keys[step++];
        } else {
            if(language_choice_test==2 && language_console_test_key(step++)) break;
            if(Read(input,&key,1)!=1) break;
            if(language_choice_test) ++g_slicks_language_console_bytes;
            if(key==0x9b) {
                /* Amiga console special-key report; consume the full CSI. */
                do { if(Read(input,&key,1)!=1) goto finished; } while(key<0x40);
                key=key=='A'?72:key=='B'?80:0;
            }
        }
        int accepted=slicks_language_choice_key(&selected,count,key);
        if(accepted) { result=accepted<0?-1:selected+1; break; }
        char up[]={ (char)0x9b,(char)('0'+count),'A',0 };
        PutStr((CONST_STRPTR)up);
    }
finished:
    if(language_choice_test!=1) {
        if(!SetMode(input,0)) result=-1;
        else if(language_choice_test) g_slicks_language_console_modes|=2;
    }
    return result;
}
#endif
