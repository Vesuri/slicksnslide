#include <stdio.h>
#include <string.h>
#include "../src/platform/amiga/retention_snapshot.h"
int main(void)
{
    static struct SlicksRaceRuntime original,race;
    static struct SlicksRetentionSnapshot saved;
    unsigned char *bytes=(unsigned char *)&original;
    for(unsigned i=0;i<sizeof original;++i)bytes[i]=(unsigned char)(i*37+(i>>8));
    race=original;
    slicks_retention_capture(&saved,&race);
    memset(&race,0xa5,SLICKS_RETENTION_PREFIX);
    memset(race.steering_cache,0x5a,sizeof race.steering_cache);
    if(!slicks_retention_restore(&race,&saved) || memcmp(&race,&original,sizeof race))return 1;
    for(unsigned region=0;region<3;++region)for(unsigned edge=0;edge<4;++edge) {
        unsigned char *p;unsigned n;
        race=original;
        if(region==0){p=race.material_map;n=sizeof race.material_map;}
        else if(region==1){p=race.surface_map;n=sizeof race.surface_map;}
        else {p=(unsigned char *)race.particle_visibility;n=sizeof race.particle_visibility;}
        if(edge<2)p[edge?n-1:0]^=0x80;
        else memset(p,edge==2?0:255,n);
        if(slicks_retention_maps_match(&saved,&race) || slicks_retention_restore(&race,&saved)) {
            fprintf(stderr,"Undetected immutable mutation: region=%u mutation=%u\n",region,edge);
            return 1;
        }
    }
    printf("Retention snapshot: all mutable bytes restore; all three immutable arrays reject edge mutations and full clears/fills; runtime=%zu snapshot=%zu\n",sizeof race,sizeof saved);
    return 0;
}
