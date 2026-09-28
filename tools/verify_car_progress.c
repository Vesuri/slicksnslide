/* Independent scalar clock/checkpoint/layer helpers against the raw native
 * block. Cold side effects are synthetic here, intentionally invalidating
 * coordinates and volatile registers; real effects need the live shadow. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>
enum { CODE=0x10000, STACK=0x80000, STOP=0x90000, ROWS=0xa0000,
       RACE=0x100000, N=262144 };
#define FIELDS(X) \
 X(CAR_X,x,4) X(CAR_Y,y,4) X(CAR_FINISHED,finished,1) \
 X(CAR_ELAPSED_UNITS,elapsed_time_units,4) X(CAR_CURRENT_LAP_UNITS,current_lap_time_units,4) \
 X(CAR_LAST_LAP_UNITS,last_lap_time_units,4) X(CAR_BEST_LAP_UNITS,best_lap_time_units,4) \
 X(CAR_ELAPSED_CENTISECONDS,elapsed_centiseconds,2) \
 X(CAR_CURRENT_LAP_CENTISECONDS,current_lap_centiseconds,2) \
 X(CAR_LAST_LAP_CENTISECONDS,last_lap_centiseconds,2) \
 X(CAR_BEST_LAP_CENTISECONDS,best_lap_centiseconds,2) \
 X(CAR_CHECKPOINT,checkpoint,2) X(CAR_LAP,lap,2) \
 X(CAR_ACTOR_LAYER,actor_layer,1) X(CAR_ACTOR_CONTACT,actor_contact,1) \
 X(CAR_SPECIAL_DRIVE_STATE,special_drive_state,2) \
 X(CAR_SELECTED_SURFACE,selected_surface,1) X(CAR_EFFECTIVE_SURFACE,effective_surface,1) \
 X(CAR_OIL_ACTIVE,oil_active,1) X(CAR_COLLISION_SAMPLING,collision_sampling,1)
#define DECL(o,m,w) static unsigned o;
FIELDS(DECL)
static unsigned RACE_CARS,RACE_MATERIAL_MAP,RACE_SURFACE_MAP,RACE_CHECKPOINTS,RACE_CHECKPOINT_COUNT;
static struct SlicksRaceRuntime race;
static unsigned char initial[N],expected[N],got[N],before[2][N],after[2][N];
static unsigned char lower[65536],upper[65536];
static unsigned test,stages,stage_at,kinds[2],calls[2];
static unsigned seed=902731;
static unsigned rnd(void){seed=seed*1664525U+1013904223U;return seed;}
static void ck(uc_err e){if(e){fprintf(stderr,"case %u: %s\n",test,uc_strerror(e));exit(2);}}
static unsigned offset(const char *path,const char *name){
 FILE *f=fopen(path,"r");char s[256];if(!f)exit(2);
 while(fgets(s,sizeof s,f))if(!strncmp(s,name,strlen(name))&&s[strlen(name)]==' '){
  unsigned v=strtoul(strstr(s,"equ")+3,0,0);fclose(f);return v;}
 fprintf(stderr,"missing %s\n",name);exit(2);
}
static void put(unsigned char *p,unsigned v,unsigned w){while(w){p[--w]=v;v>>=8;}}
static unsigned get(const unsigned char *p,unsigned w){unsigned v=0;while(w--)v=(v<<8)|*p++;return v;}
static void pack(unsigned char *p){
 struct SlicksRaceCar *c=&race.cars[0];unsigned char *q=p+RACE_CARS;
#define FIELD(o,m,w) put(q+o,(unsigned)c->m,w);
 FIELDS(FIELD)
 put(p+RACE_CHECKPOINT_COUNT,race.navigation.checkpoint_count,2);
 for(unsigned i=0;i<100;++i){const struct SlicksTrackCheckpoint *cp=&race.navigation.checkpoints[i];
  put(p+RACE_CHECKPOINTS+i*6,cp->x[0],2);put(p+RACE_CHECKPOINTS+i*6+2,cp->x[1],2);
  p[RACE_CHECKPOINTS+i*6+4]=cp->y[0];p[RACE_CHECKPOINTS+i*6+5]=cp->y[1];}
 memcpy(p+RACE_MATERIAL_MAP,race.material_map,sizeof race.material_map);
 memcpy(p+RACE_SURFACE_MAP,race.surface_map,sizeof race.surface_map);
}
static void compare(const unsigned char *a,const unsigned char *b,const char *where){
 for(unsigned i=0;i<N;++i)if(a[i]!=b[i]){
  fprintf(stderr,"case %u %s byte %u expected %u actual %u\n",test,where,i,a[i],b[i]);exit(1);}
}
static void stage(unsigned kind){
 unsigned s=stages++;kinds[s]=kind;++calls[kind-1];
 memcpy(before[s],initial,N);pack(before[s]);
 if(kind==1 && test%3==0){
  /* A bridge may invalidate both coordinates and map-selection state. */
  race.cars[0].x=(test&1)?17500:-6553600;
  race.cars[0].y=(test&2)?19000:9200;
  race.cars[0].actor_layer^=1;
  race.cars[0].special_drive_state=(test&4)?-1:0;
 }
 if(kind==2)race.cars[0].elapsed_time_units^=0x12345678;
 memcpy(after[s],initial,N);pack(after[s]);
}
static void callback(uc_engine *u,uint64_t address,uint32_t size,void *opaque){
 (void)size;(void)opaque;unsigned s=stage_at++,sp,v;unsigned char args[12];
 if(s>=stages || address!=STOP+16*kinds[s]){fprintf(stderr,"case %u callback order\n",test);exit(1);}
 ck(uc_mem_read(u,RACE,got,N));compare(before[s],got,"callback entry");
 ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));ck(uc_mem_read(u,sp,args,sizeof args));
 if(get(args+4,4)!=RACE || get(args+8,4)!=RACE+RACE_CARS){fprintf(stderr,"callback arguments\n");exit(1);}
 ck(uc_mem_write(u,RACE,after[s],N));
 const int scratch[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_A0,UC_M68K_REG_A1};
 v=0xdeadbeef;for(unsigned i=0;i<4;++i)ck(uc_reg_write(u,scratch[i],&v));
}
int main(int argc,char **argv){
 if(argc!=3)return 2;
#define LOAD(o,m,w) o=offset(argv[2],#o);
 FIELDS(LOAD)
#define EXTRA(o) o=offset(argv[2],#o)
 EXTRA(RACE_CARS);EXTRA(RACE_MATERIAL_MAP);EXTRA(RACE_SURFACE_MAP);
 EXTRA(RACE_CHECKPOINTS);EXTRA(RACE_CHECKPOINT_COUNT);
 unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t size=fread(code,1,sizeof code,f);fclose(f);if(size<4 || size==sizeof code)return 2;
 unsigned register_entry=get(code+size-4,4);
 uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
 ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
 ck(uc_mem_map(u,CODE,65536,UC_PROT_ALL));ck(uc_mem_write(u,CODE,code,size));
 ck(uc_mem_map(u,STACK,65536,UC_PROT_ALL));ck(uc_mem_map(u,STOP,65536,UC_PROT_ALL));
 ck(uc_mem_map(u,ROWS,4096,UC_PROT_ALL));ck(uc_mem_map(u,RACE,N,UC_PROT_ALL));
 unsigned char rows[800];for(unsigned i=0;i<200;++i)put(rows+4*i,i*320,4);
 ck(uc_mem_write(u,ROWS,rows,sizeof rows));
 const unsigned char rts[]={0x4e,0x75};ck(uc_mem_write(u,STOP+16,rts,2));ck(uc_mem_write(u,STOP+32,rts,2));
 uc_hook hook;ck(uc_hook_add(u,&hook,UC_HOOK_CODE,callback,0,STOP+16,STOP+32));
 const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
  UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
  UC_M68K_REG_A5,UC_M68K_REG_A6};
 static const int32_t coords[]={INT32_MIN,INT32_MAX,-6553600,6553600,-101,-100,-99,-1,0,99,100,31899,31900,31999,32000,18999,19000};
 static const unsigned clocks[]={0,1,17998,17999,18000,65535,65536,0x7fffffff,0x80000000,0xfffffffe,0xffffffff};
 for(test=0;test<4096;++test){
  memset(&race,0,sizeof race);memset(initial,0xa7,N);stages=stage_at=0;
  struct SlicksRaceCar *c=&race.cars[0];
  unsigned ticks=(test%4)?test%6:(rnd()&65535);
  c->x=test%3?(long)(rnd()%33000)-500:coords[(test/3)%17];
  c->y=test%3?(long)(rnd()%20000)-500:coords[(test/51)%17];
  c->finished=test%5==0;c->elapsed_time_units=clocks[test%11];
  c->current_lap_time_units=clocks[(test/11)%11];c->last_lap_time_units=rnd();c->best_lap_time_units=clocks[(test/121)%11];
  c->elapsed_centiseconds=123;c->current_lap_centiseconds=456;c->last_lap_centiseconds=789;c->best_lap_centiseconds=987;
  c->lap=test%17==0?65535:1+test%12;
  c->actor_layer=test%4;c->actor_contact=test%7==0;
  c->special_drive_state=test%4==0?-1:test%4==1?2:0;
  c->selected_surface=test%3==0?17:23;c->effective_surface=99;c->oil_active=1;c->collision_sampling=1;
  race.laps_to_run=500; /* No real flags/Arcade side effects in this oracle. */
  race.navigation.checkpoint_count=test%101;
  c->checkpoint=test%4==0?race.navigation.checkpoint_count:(race.navigation.checkpoint_count?test%race.navigation.checkpoint_count:0);
  for(unsigned i=0;i<100;++i){struct SlicksTrackCheckpoint *p=&race.navigation.checkpoints[i];
   p->x[0]=0;p->x[1]=320;p->y[0]=0;p->y[1]=190;
   if(test%7==0){p->x[0]=(unsigned short)(c->x/100+1);p->x[1]=p->x[0];p->y[0]=(unsigned char)(c->y/100+1);p->y[1]=p->y[0];}
  }
  memset(lower,test%5==0?17:test%5==1?0:test%5==2?18:test%5==3?19:255,sizeof lower);
  memset(upper,test%4==0?17:test%4==1?18:test%4==2?19:255,sizeof upper);
  memcpy(race.material_map,lower,sizeof race.material_map);
  memcpy(race.surface_map,upper,sizeof race.surface_map);pack(initial);
  if(!c->finished)advance_car_clock(c,(unsigned short)ticks);
  unsigned old_checkpoint=c->checkpoint;
  advance_checkpoint(&race,c);
  if(c->checkpoint!=old_checkpoint)stage(1);
  update_actor_layer(&race,c);
  if(c->checkpoint>=race.navigation.checkpoint_count &&
    (c->selected_surface==17 || (c->x/100>=0 && c->x/100<320 && c->y/100>=0 && c->y/100<190 &&
     lower[(c->y/100)*320+c->x/100]==17))){
   c->checkpoint=0;record_lap_clock(c);stage(2);
  }
  memcpy(expected,initial,N);pack(expected);
  ck(uc_mem_write(u,RACE,initial,N));
  unsigned sp=STACK+32768,v=0x2000;ck(uc_reg_write(u,UC_M68K_REG_SR,&v));
  ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
  for(unsigned i=0;i<11;++i){v=0xa1230000+i;ck(uc_reg_write(u,regs[i],&v));}
  if(test&1){v=RACE;ck(uc_reg_write(u,UC_M68K_REG_A2,&v));v=RACE+RACE_CARS;ck(uc_reg_write(u,UC_M68K_REG_A3,&v));v=ticks;ck(uc_reg_write(u,UC_M68K_REG_D1,&v));}
  unsigned char args[16];put(args,STOP,4);put(args+4,RACE,4);put(args+8,RACE+RACE_CARS,4);put(args+12,ticks,4);ck(uc_mem_write(u,sp,args,sizeof args));
  ck(uc_emu_start(u,CODE+((test&1)?register_entry:0),STOP,0,10000));ck(uc_reg_read(u,UC_M68K_REG_PC,&v));
  if(v!=STOP || stage_at!=stages){fprintf(stderr,"case %u incomplete execution\n",test);return 1;}
  ck(uc_mem_read(u,RACE,got,N));compare(expected,got,"return");
  ck(uc_reg_read(u,UC_M68K_REG_A7,&v));if(v!=sp+4)return 1;
  for(unsigned i=0;i<11;++i){
   if((test&1) && i<2)continue;
   unsigned wanted=(test&1) && i==6?RACE:(test&1) && i==7?RACE+RACE_CARS:0xa1230000+i;
   ck(uc_reg_read(u,regs[i],&v));if(v!=wanted)return 1;
  }
 }
 uc_close(u);printf("Native car progress: 4096 full-state cases across C and register entries, %u checkpoint and %u lap callbacks; scalar clocks, layers, signed coordinates, ABI and callback invalidation match\n",calls[0],calls[1]);return 0;
}
