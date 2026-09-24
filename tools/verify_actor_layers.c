#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"

/* Replay real DOS transition inputs through the production native routine.
 * This checks transitions, not the upstream simulation producing the inputs. */
struct Sample {
    unsigned frame, car, layer, mode0, mode1, contact, previous;
    int special;
    unsigned selected, effective;
    long x, y;
};

static long load(const char *path, unsigned char *bytes, unsigned capacity)
{
    FILE *file=fopen(path,"rb");
    long size;
    if(!file) return -1;
    size=(long)fread(bytes,1,capacity,file);
    if(ferror(file) || fgetc(file)!=EOF) size=-1;
    fclose(file);
    return size;
}

int main(int argc, char **argv)
{
    static struct SlicksRaceRuntime race;
    struct Sample pre[4] = {{0}}, s;
    unsigned pending[4] = {0}, count = 0, entered = 0, exited = 0;
    unsigned contacts = 0, suppressed = 0, specials = 0, skipped = 0;
    char line[2048], stage[8];
    FILE *file;
    if ((argc != 2 && argc != 4) || !(file = fopen(argv[1], "r"))) {
        fprintf(stderr, "usage: verify_actor_layers DOS-layer-log [SLICKS.DAT TRACK.SS]\n");
        return 1;
    }
    if(argc==4) {
        static unsigned char dat[65536],track[8192],arena[65536],logical[0x40000];
        long dat_size=load(argv[2],dat,sizeof dat),track_size=load(argv[3],track,sizeof track);
        if(dat_size<=0 || track_size<=0 ||
           slicks_build_track_scene(logical,race.material_map,race.surface_map,
               dat,dat_size,track,track_size,arena,sizeof arena,&race.navigation)<=0)
            return fprintf(stderr,"track decode failed\n"),1;
    }
    while (fgets(line, sizeof line, file)) {
        char *start = strstr(line, "SLICKS_LAYER_");
        if (!start) continue;
        if (sscanf(start, "SLICKS_LAYER_%7s frame=%u car=%u layer=%u mode0=%u mode1=%u contact=%u previous=%u special=%d selected=%u effective=%u x=%ld y=%ld",
                   stage, &s.frame, &s.car, &s.layer, &s.mode0, &s.mode1,
                   &s.contact, &s.previous, &s.special, &s.selected,
                   &s.effective, &s.x, &s.y) != 13 || s.car >= 4)
            return fprintf(stderr, "malformed layer record: %s", line), 1;
        if (!strcmp(stage, "PRE")) {
            if (pending[s.car]) return fprintf(stderr, "unpaired PRE\n"), 1;
            pre[s.car] = s;
            pending[s.car] = 1;
        } else if (!strcmp(stage, "POST")) {
            struct Sample *p = &pre[s.car];
            struct SlicksRaceCar car = {0};
            long x = p->x / 100, y = p->y / 100;
            if (!pending[s.car] || p->frame != s.frame)
                return fprintf(stderr, "unpaired POST\n"), 1;
            pending[s.car] = 0;
            if (p->x != s.x || p->y != s.y || p->mode0 != s.mode0 ||
                p->mode1 != s.mode1 || p->contact != s.contact ||
                p->special != s.special)
                return fprintf(stderr, "transition inputs changed inside pair\n"), 1;
            if (x < 0 || x >= 320 || y < 0 || y >= 190) {
                ++skipped;
                continue;
            }
            car.x = p->x; car.y = p->y;
            car.actor_layer = p->layer;
            car.special_drive_state = p->special;
            car.actor_contact = p->contact;
            if(argc==4) {
                if(race.material_map[y*320+x]!=p->mode0 || race.surface_map[y*320+x]!=p->mode1)
                    return fprintf(stderr,"decoded map mismatch frame=%u car=%u xy=%ld,%ld special=%d native=%u,%u DOS=%u,%u\n",
                        p->frame,p->car,x,y,p->special,race.material_map[y*320+x],race.surface_map[y*320+x],p->mode0,p->mode1),1;
            } else {
                race.material_map[y * 320 + x] = p->mode0;
                race.surface_map[y * 320 + x] = p->mode1;
            }
            update_actor_layer(&race, &car);
            if (car.actor_layer != s.layer || car.effective_surface != s.effective)
                return fprintf(stderr, "frame %u car %u: native layer/surface %u/%u DOS %u/%u\n",
                               s.frame, s.car, car.actor_layer, car.effective_surface,
                               s.layer, s.effective), 1;
            ++count;
            entered += !p->layer && s.layer;
            exited += p->layer && !s.layer;
            contacts += !!p->contact;
            suppressed += !p->layer && !p->mode0 && !p->special && p->contact;
            specials += !!p->special;
        } else return fprintf(stderr, "unknown stage\n"), 1;
    }
    fclose(file);
    for (unsigned i = 0; i < 4; ++i)
        if (pending[i]) return fprintf(stderr, "truncated pair\n"), 1;
    printf("DOS/native layers: %u matched, %u entries, %u exits, %u contact (%u entry-suppressed), %u special, %u out-of-bounds skipped\n",
           count, entered, exited, contacts, suppressed, specials, skipped);
    if(argc==4) puts("Decoded original track maps match both DOS surface inputs at every in-bounds sample.");
    return count ? 0 : 1;
}
