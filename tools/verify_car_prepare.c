/* Whole preparation arithmetic and callback contract oracle. Callback bodies
 * are synthetic and deliberately mutate inputs: this proves ordering/ABI,
 * not real AI/weapon/integration behaviour. Live integration remains a gate. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>
enum { CODE=0x10000, STACK=0x80000, STOP=0x90000, RACE=0x100000, N=262144 };
#define R_FIELDS(X) \
 X(RACE_PARTICIPATION_READY,participation_ready,1) X(RACE_HUMAN_CONTROL,human_control,1) \
 X(RACE_CONTROLS,controls,1) X(RACE_GAME_CLOCK,game_clock_ticks,4) \
 X(RACE_FINISH_DEADLINE,finish_deadline,4) X(RACE_COMPLETE,race_complete,1) \
 X(RACE_FUEL_OPTION,fuel_option,2) X(RACE_DAMAGE_ENABLED,damage_enabled,1) \
 X(RACE_WEAPONS_READY,weapons.ready,1)
#define C_FIELDS(X) \
 X(CAR_FUEL,fuel,4) X(CAR_SPEED_FIXED,speed_fixed,4) X(CAR_SPEED,speed,2) \
 X(CAR_HEADING,heading,2) X(CAR_MEASURED_SPEED,measured_speed,4) \
 X(CAR_VELOCITY_X,velocity_x,4) X(CAR_VELOCITY_Y,velocity_y,4) \
 X(CAR_SERVICE_FLAGS,service_flags,1) X(CAR_FINISHED,finished,1) \
 X(CAR_AI_CONTROL_LATCH,ai_control_latch,1) X(CAR_MAXIMUM_SPEED,maximum_speed,2) \
 X(CAR_POSITION_SCALE,position_scale,1) X(CAR_SPECIAL_DRIVE_STATE,special_drive_state,2) \
 X(CAR_FORWARD_DRIVE_LATCH,forward_drive_latch,1) X(CAR_DRIVE_BIAS,drive_bias,2) \
 X(CAR_VEHICLE,vehicle,1) X(CAR_STEERING_AMOUNT,steering_amount,2) \
 X(CAR_STEERING_SCALE,steering_scale,2) X(CAR_STEERING_PROPERTY,steering_property,2) \
 X(CAR_DAMAGE_TURN_SIGN,damage_turn_sign,1)
#define S_FIELDS(X) \
 X(STEERING_INPUT,input,2) X(STEERING_SCALE,scale,2) X(STEERING_DAMAGE,damage,2) \
 X(STEERING_PROPERTY,property,2) X(STEERING_DELTA,delta,2) X(STEERING_VALID,valid,1)
#define DECL(o,m,w) static unsigned o;
R_FIELDS(DECL) C_FIELDS(DECL) S_FIELDS(DECL)
static unsigned RACE_CARS,RACE_PARTICIPATION,RACE_DRIVER_CONTROLS,RACE_PROPERTIES;
static unsigned CAR_SIZE,CAR_DAMAGE,PROPERTY_SIZE,PROPERTY_ENGINE_SOUND;
static unsigned RACE_STEERING_CACHE,STEERING_CACHE_SIZE;
static struct SlicksRaceRuntime race;
static unsigned char initial[N],expected[N],got[N],before[3][N],after[3][N];
static unsigned stage_count,stage_at,kind[3],args[3][5],arg_count[3],result[3],test;
static unsigned seed=8723;
static unsigned rnd(void){seed=seed*1664525U+1013904223U;return seed;}
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned offset(const char *path,const char *name){
 FILE *f=fopen(path,"r");char s[256];if(!f)exit(2);
 while(fgets(s,sizeof s,f))if(!strncmp(s,name,strlen(name))&&s[strlen(name)]==' '){
  unsigned v=strtoul(strstr(s,"equ")+3,0,0);fclose(f);return v;}
 fprintf(stderr,"missing %s\n",name);exit(2);
}
static void put(unsigned char *p,unsigned v,unsigned w){while(w){p[--w]=v;v>>=8;}}
static unsigned get(const unsigned char *p,unsigned w){unsigned v=0;while(w--)v=(v<<8)|*p++;return v;}
static void pack(unsigned char *p){
#define RF(o,m,w) put(p+o,(unsigned)race.m,w);
 R_FIELDS(RF)
 for(unsigned i=0;i<4;++i){struct SlicksRaceCar *c=&race.cars[i];
  unsigned char *q=p+RACE_CARS+i*CAR_SIZE;
#define CF(o,m,w) put(q+o,(unsigned)c->m,w);
  C_FIELDS(CF)
  for(unsigned j=0;j<4;++j)put(q+CAR_DAMAGE+j*2,c->damage[j],2);
  p[RACE_PARTICIPATION+i]=race.participation[i];p[RACE_DRIVER_CONTROLS+i]=race.driver_controls[i];
  struct SlicksSteeringCache *s=&race.steering_cache[i];q=p+RACE_STEERING_CACHE+i*STEERING_CACHE_SIZE;
#define SF(o,m,w) put(q+o,(unsigned)s->m,w);
  S_FIELDS(SF)
 }
 for(unsigned i=0;i<10;++i)p[RACE_PROPERTIES+i*PROPERTY_SIZE+PROPERTY_ENGINE_SOUND]=race.properties[i].engine_sound;
}
static void compare(const unsigned char *a,const unsigned char *b,const char *where){
 if(!memcmp(a,b,N))return;
 for(unsigned i=0;i<N;++i)if(a[i]!=b[i]){fprintf(stderr,"case %u %s byte %u expected %u actual %u\n",test,where,i,a[i],b[i]);exit(1);}
}
static void callback(uc_engine *u,uint64_t address,uint32_t size,void *opaque){
 (void)size;(void)opaque;
 unsigned at=stage_at++,sp,v;unsigned char stack[24];
 if(at>=stage_count || address!=STOP+16*kind[at]){fprintf(stderr,"case %u callback order\n",test);exit(1);}
 ck(uc_mem_read(u,RACE,got,N));compare(before[at],got,"callback entry");
 ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));ck(uc_mem_read(u,sp,stack,sizeof stack));
 for(unsigned i=0;i<arg_count[at];++i)if(get(stack+4+i*4,4)!=args[at][i]){
  fprintf(stderr,"case %u callback %u argument %u\n",test,kind[at],i);exit(1);}
 ck(uc_mem_write(u,RACE,after[at],N));
 v=0xd00d1234;ck(uc_reg_write(u,UC_M68K_REG_D1,&v));
 v=0xdead0100;ck(uc_reg_write(u,UC_M68K_REG_A0,&v));ck(uc_reg_write(u,UC_M68K_REG_A1,&v));
 v=result[at];ck(uc_reg_write(u,UC_M68K_REG_D0,&v));
}
static void guard(uc_engine *u,uc_mem_type type,uint64_t at,int n,int64_t value,void *opaque){
 (void)u;(void)type;(void)value;(void)opaque;
 if((at>=RACE&&at+(unsigned)n<=RACE+N)||(at>=STACK-128&&at+(unsigned)n<=STACK))return;
 fprintf(stderr,"case %u write outside state/stack %llx\n",test,at);exit(1);
}
static unsigned start_stage(unsigned k,const unsigned *a,unsigned n){
 unsigned s=stage_count++;kind[s]=k;arg_count[s]=n;memcpy(args[s],a,n*4);
 memcpy(before[s],initial,N);pack(before[s]);return s;
}
static void end_stage(unsigned s,unsigned value){memcpy(after[s],initial,N);pack(after[s]);result[s]=value;}
static unsigned reference(unsigned index,unsigned ticks){
 struct SlicksRaceCar *c=&race.cars[index];unsigned controls;
 consume_idle_fuel(&race,c,ticks);
 if(driver_role(&race,index)<0)controls=race.participation_ready?race.driver_controls[index]:race.controls;
 else {
  unsigned a[]={RACE,index,ticks,RACE+RACE_CARS+index*CAR_SIZE};unsigned s=start_stage(1,a,4);
  /* AI can update car fields before finish gating/input calculation. */
  c->position_scale=(unsigned char)(c->position_scale+3);c->ai_control_latch=(test>>2)&15;
  controls=c->ai_control_latch;end_stage(s,controls);
 }
 controls=apply_finish_gate(&race,index,controls);
 short input=profile_steering_input(c->position_scale,driver_role(&race,index));
 if(c->special_drive_state)controls=0;
 if(controls&1)apply_throttle(c,ticks);
 unsigned gate=!c->special_drive_state&&!slicks_finish_controls_suppressed(race.game_clock_ticks,race.finish_deadline,c->finished?1:-1);
 if(race.weapons.ready){
  unsigned a[]={RACE,index,ticks,controls,gate};unsigned s=start_stage(2,a,5);
  /* Deliberately mutate inputs after steering input was captured. */
  c->position_scale^=0x3f;c->steering_scale=(short)(c->steering_scale+10);
  controls=(controls^((test>>3)&3))&15;end_stage(s,controls);
 }else if(controls&2)apply_brake(&race,c,ticks);
 c->steering_amount=input;
 short step=cached_steering_delta(c,&race.steering_cache[index],input,ticks);
 if(controls&4)c->heading=(short)(c->heading-step);
 if(controls&8)c->heading=(short)(c->heading+step);
 c->heading=(short)(c->heading+damage_heading_delta(c,ticks));
 while(c->heading<0)c->heading=(short)(c->heading+19200);
 while(c->heading>=19200)c->heading=(short)(c->heading-19200);
 unsigned a[]={RACE,RACE+RACE_CARS+index*CAR_SIZE,ticks,controls&3};unsigned s=start_stage(3,a,4);
 c->speed_fixed=(int32_t)((uint32_t)c->speed_fixed+0x10203U);
 c->heading=(short)(c->heading+23000);end_stage(s,0);
 c->speed=(short)slicks_div100(c->speed_fixed);
 while(c->heading<0)c->heading=(short)(c->heading+19200);
 while(c->heading>=19200)c->heading=(short)(c->heading-19200);
 return controls;
}
int main(int argc,char **argv){
 if(argc!=3)return 2;
#define LOAD(o,m,w) o=offset(argv[2],#o);
 R_FIELDS(LOAD) C_FIELDS(LOAD) S_FIELDS(LOAD)
#define O(o) o=offset(argv[2],#o)
 O(RACE_CARS);O(RACE_PARTICIPATION);O(RACE_DRIVER_CONTROLS);O(RACE_PROPERTIES);
 O(CAR_SIZE);O(CAR_DAMAGE);O(PROPERTY_SIZE);O(PROPERTY_ENGINE_SOUND);
 O(RACE_STEERING_CACHE);O(STEERING_CACHE_SIZE);
 if(RACE_STEERING_CACHE+4*STEERING_CACHE_SIZE>N)return 2;
 unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
 uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
 ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));ck(uc_mem_write(u,CODE,code,n));
 unsigned char rts[]={0x4e,0x75};for(unsigned i=1;i<=3;++i)ck(uc_mem_write(u,STOP+i*16,rts,2));
 uc_hook h,g;ck(uc_hook_add(u,&h,UC_HOOK_CODE,callback,0,STOP+16,STOP+48));
 ck(uc_hook_add(u,&g,UC_HOOK_MEM_WRITE,guard,0,1,0));
 const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
 unsigned ai=0,weapons=0,hits=0,extremes=0;
 for(test=0;test<4096;++test){
  memset(&race,0,sizeof race);memset(initial,0xa5,N);stage_count=stage_at=0;
  unsigned index=test%4,ticks=(test/4)%5;struct SlicksRaceCar *c=&race.cars[index];
  race.participation_ready=(test/20)%2;race.human_control=(test/40)%2;
  race.participation[index]=(test/80)%2?-1:1;race.driver_controls[index]=test&15;race.controls=(test>>4)&15;
  race.fuel_option=(test/3)%2;race.damage_enabled=(test/7)%2;
  race.game_clock_ticks=1000;race.finish_deadline=(test/9)%3==0?0:(test/9)%3==1?1200:1300;
  if(test%11==0)race.game_clock_ticks=1400;
  race.weapons.ready=(test/13)%2;c->finished=(test/17)%2;
  c->fuel=test%3==0?0:rnd()%10000;c->speed_fixed=(int32_t)(rnd()%200001)-100000;
  c->service_flags=(test/19)%4;c->maximum_speed=rnd()%65536;c->forward_drive_latch=(test/23)%3;
  c->position_scale=rnd()%256;c->special_drive_state=(test/29)%3-1;c->heading=(short)rnd();
  c->measured_speed=(int)(rnd()%200)-50;c->drive_bias=(short)rnd();
  c->velocity_x=(int32_t)(rnd()%200001)-100000;c->velocity_y=-c->velocity_x;
  c->vehicle=test%10;race.properties[c->vehicle].engine_sound=(test/31)%2;
  c->steering_scale=(short)rnd();c->steering_property=(short)rnd();
  for(unsigned i=0;i<4;++i)c->damage[i]=(short)rnd();
  c->damage_turn_sign=(signed char)(test%3-1);
  if(test%16==0){
   static const unsigned tick_edges[]={0,1,204,205,409,32767,32768,65535};
   static const unsigned long_edges[]={0,1,0x7fffffffU,0x80000000U,0xffffffffU};
   unsigned e=test/16;ticks=tick_edges[e%8];
   c->fuel=long_edges[(e/8)%5];c->speed_fixed=(int32_t)long_edges[(e/40)%5];
   race.game_clock_ticks=long_edges[e%5];race.finish_deadline=long_edges[(e/5)%5];
   ++extremes;
  }
  if(test%5==0){
   short input=profile_steering_input(c->position_scale,driver_role(&race,index));
   (void)cached_steering_delta(c,&race.steering_cache[index],input,1);++hits;
  }
  pack(initial);ck(uc_mem_write(u,RACE,initial,N));
  unsigned want=reference(index,ticks);memcpy(expected,initial,N);pack(expected);
  for(unsigned i=0;i<stage_count;++i){ai+=kind[i]==1;weapons+=kind[i]==2;}
  unsigned char stack[16];put(stack,STOP,4);put(stack+4,RACE,4);put(stack+8,index,4);put(stack+12,ticks,4);
  ck(uc_mem_write(u,STACK,stack,sizeof stack));unsigned sp=STACK,values[11],v;
  ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
  for(unsigned i=0;i<11;++i){values[i]=0xa5000000+i*123+test;ck(uc_reg_write(u,regs[i],values+i));}
  ck(uc_emu_start(u,CODE,STOP,0,1000000));ck(uc_reg_read(u,UC_M68K_REG_PC,&v));if(v!=STOP)return 1;
  ck(uc_mem_read(u,RACE,got,N));compare(expected,got,"final");
  ck(uc_reg_read(u,UC_M68K_REG_D0,&v));if(v!=want||stage_at!=stage_count)return 1;
  ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
  for(unsigned i=0;i<11;++i){ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
 }
 ck(uc_close(u));printf("Car preparation: 4096 complete-image/ABI cases (%u extreme tick/fuel/clock cases), %u AI and %u weapon callback boundaries, %u seeded caches pass (synthetic callbacks; not integration acceptance)\n",extremes,ai,weapons,hits);return 0;
}
