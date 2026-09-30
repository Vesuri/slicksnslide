#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/championship.h"
static const unsigned char names[3][9]={"FIRST","SECOND","THIRD"};
static const unsigned char *name(void *context,unsigned i) { (void)context; return names[i]; }
int main(void)
{
    struct SlicksPlayerProfiles profiles={.count=3};
    memcpy(profiles.names[1],"COMPUTER",9); memcpy(profiles.names[2],"DRIVER",7);
    profiles.setup[1].flags=7; profiles.setup[2].flags=2;
    for(unsigned j=0;j<6;++j) profiles.setup[2].colours[j]=(unsigned char)(10+j);
    struct SlicksSetupSession source={.random_state=123456};
    struct SlicksConfiguration config={0};
    const unsigned char fallback[4][6]={{1},{2},{3},{4}},scales[4]={25,50,100,200};
    const short selection[3]={2,0,1};
    unsigned char tracks[256][8],decoded_tracks[256][8],bytes[SLICKS_SAVED_GAME_MAX_BYTES];
    for(unsigned i=0;i<4;++i) {
        source.players.selected[i]=i==3?-2:i==0?1:2;
        source.players.participation[i]=i==3?0:i==0?1:-1;
        source.players.vehicle[i]=(signed char)(i+2);
        source.points[i]=(short)(30+i); source.cash[i]=(short)(1000+101*i);
        for(unsigned j=0;j<13;++j) source.inventory[i][j]=(short)(13*i+j);
    }
    struct SlicksSavedGame game,decoded;
    assert(!slicks_championship_export(&game,tracks,256,selection,3,1,3,name,0,&source,&profiles,scales));
    long size=slicks_save_game_bytes(&game,bytes,sizeof bytes); assert(size>0);
    assert(!slicks_load_game_bytes(&decoded,decoded_tracks,256,bytes,size));
    struct SlicksSetupSession current={.random_state=987654},staged;
    short resolved_tracks[256];
    struct SlicksSavedGameResolved resolved={.tracks=resolved_tracks,.track_capacity=256};
    assert(slicks_championship_stage(&staged,&resolved,&decoded,&current,&config,&profiles,fallback,10,3,name,0)==SLICKS_RESUME_READY);
    assert(staged.random_state==current.random_state && staged.players.count==3 && staged.saved_position_scale_valid);
    assert(!memcmp(staged.points,source.points,sizeof source.points));
    assert(!memcmp(staged.cash,source.cash,sizeof source.cash));
    assert(!memcmp(staged.inventory,source.inventory,sizeof source.inventory));
    assert(!memcmp(staged.saved_position_scale,scales,4));
    assert(!memcmp(resolved.tracks,selection,sizeof selection));
    assert(!memcmp(staged.players.colours[1],profiles.setup[2].colours,6));
    assert(staged.players.selected[1]==2 && staged.players.selected[2]==2 && staged.players.selected[3]==-2);
    struct SlicksSetupSession previous=staged;
    struct SlicksSavedGameResolved previous_resolved=resolved;
    for(unsigned fault=0;fault<9;++fault) {
        struct SlicksSavedGame bad=decoded; struct SlicksConfiguration badconfig=config;
        struct SlicksPlayerProfiles badprofiles=profiles;
        if(fault==0) bad.next_track=-1;
        if(fault==1) bad.next_track=3;
        if(fault==2) bad.track_count=0;
        if(fault==3) bad.names[1][0]='X';
        if(fault==4) for(unsigned i=0;i<4;++i) bad.participation[i]=0;
        if(fault==5) { badconfig.options[0]=5; badconfig.options[14]=1; }
        if(fault==6) badprofiles.count=1;
        if(fault==7) resolved.track_capacity=2;
        if(fault==8) decoded_tracks[2][0]='X';
        previous_resolved=resolved;
        assert(slicks_championship_stage(&staged,&resolved,&bad,&current,&badconfig,&badprofiles,fallback,10,3,name,0)!=SLICKS_RESUME_READY);
        assert(!memcmp(&staged,&previous,sizeof staged) && !memcmp(&resolved,&previous_resolved,sizeof resolved));
        assert(!memcmp(resolved_tracks,selection,sizeof selection));
        resolved.track_capacity=256;
    }
    puts("Championship export/encode/decode/stage: shared human, AI, inactive, inventory, standings, scale, RNG and atomic rejection PASS");
    static unsigned char large_tracks[SLICKS_SAVED_GAME_TRACK_MAX][8];
    static unsigned char large_decoded[SLICKS_SAVED_GAME_TRACK_MAX][8];
    static short large_selection[SLICKS_SAVED_GAME_TRACK_MAX],large_resolved[SLICKS_SAVED_GAME_TRACK_MAX];
    const unsigned large_counts[]={257,300,SLICKS_SAVED_GAME_TRACK_MAX};
    for(unsigned c=0;c<sizeof large_counts/sizeof large_counts[0];++c) {
        unsigned count=large_counts[c];
        for(unsigned i=0;i<count;++i) large_selection[i]=(short)(i%3);
        memset(large_tracks,0xa5,sizeof large_tracks);
        struct SlicksSavedGame before_game=game;
        assert(slicks_championship_export(&game,large_tracks,count-1,large_selection,count,1,
            3,name,0,&source,&profiles,scales)==-1);
        assert(!memcmp(&game,&before_game,sizeof game));
        for(unsigned i=0;i<sizeof large_tracks;++i) assert(((unsigned char *)large_tracks)[i]==0xa5);
        assert(!slicks_championship_export(&game,large_tracks,count,large_selection,count,1,
            3,name,0,&source,&profiles,scales));
        size=slicks_save_game_bytes(&game,bytes,sizeof bytes); assert(size>0);
        assert(!slicks_load_game_bytes(&decoded,large_decoded,count,bytes,size));
        resolved.tracks=large_resolved; resolved.track_capacity=count;
        assert(slicks_championship_stage(&staged,&resolved,&decoded,&current,&config,&profiles,
            fallback,10,3,name,0)==SLICKS_RESUME_READY);
        assert(!memcmp(large_resolved,large_selection,count*sizeof(short)));
        assert(!memcmp(staged.inventory,source.inventory,sizeof source.inventory));
        assert(staged.random_state==current.random_state);
    }
    puts("Large championship capacity/round-trip/staging: 257, 300 and 10000 tracks PASS (host boundary tests)");
}
