/* Raw big-endian oracle, independent of native dispatch and address maths.
 * The full-frame RETCHECK comparison remains required for integration. */
#define main point_restore_verifier_main
#include "verify_point_restore.c"
#undef main
enum { CODE=0x12000, BASE=0x20000, N=0x40000, PIX=0x20000,
       STACK=0xf0000, STOP=0x18000 };
static unsigned PART,NEXT,INDEX,CHUNKY,ACTORS,PREV,HANDLES,COUNT;
static unsigned get32(const unsigned char *p)
{return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void sprite_restore(unsigned char *m,unsigned h)
{
    unsigned char *a=m+ACTORS+h*164,*p=m+PREV+h*12;
    a[33]=0;
    if(!a[32])return;
    int x=(short)word(a+26),y=(short)word(a+28),w=a[30],height=a[31];
    for(int j=0;j<height;++j)for(int i=0;i<w;++i)
        if(x+i>=0 && x+i<320 && y+j>=0 && y+j<200)
            m[PIX+(y+j)*320+x+i]=a[36+j*w+i];
    memcpy(p,a+26,6);p[6]=a[21];p[7]=a[20];p[8]=a[16];p[9]=a[24];
    p[10]=a[22];p[11]=a[23];a[32]=0;
    unsigned count=word(m+COUNT);m[HANDLES+count]=h;be16(m+COUNT,count+1);
}
static unsigned reference(unsigned char *m,unsigned h)
{
    while(h){
        short index=(short)word(m+INDEX+2*h);
        if(index>=0){
            unsigned char *p=m+PART+index*24;
            if(p[20]&1){m[PIX+word(p+14)*320+word(p+12)]=p[16];p[20]=2;}
        }else{
            unsigned char *a=m+ACTORS+h*164;
            if((a[33]&1) && a[32]){m[PREV+h*12+6]=a[21];a[33]=2;}
            else{
                a[33]=0;
                int x=(short)word(a+26),y=(short)word(a+28);
                if(!a[32] || x<0 || y<0 || !a[30] || !a[31] ||
                   x+a[30]>320 || y+a[31]>200)return h;
                sprite_restore(m,h);
            }
        }
        h=m[NEXT+h];
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    PART=off(argv[2],"RACE_TRAIL_PARTICLES");NEXT=off(argv[2],"RACE_ACTOR_ORDER_PREVIOUS");
    INDEX=off(argv[2],"RACE_TRAIL_INDEX");CHUNKY=off(argv[2],"RACE_CHUNKY");
    ACTORS=off(argv[2],"RACE_ACTORS");PREV=off(argv[2],"RACE_SPRITE_DIRTY_PREVIOUS");
    HANDLES=off(argv[2],"RACE_SPRITE_DIRTY_HANDLES");COUNT=off(argv[2],"RACE_SPRITE_DIRTY_COUNT");
    unsigned char code[16384];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t bytes=fread(code,1,sizeof code,f);fclose(f);
    if(bytes<4 || bytes==sizeof code)return 2;
    unsigned entry=CODE+get32(code+bytes-4);
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,bytes));
    unsigned char rows[800];for(unsigned y=0;y<200;++y)be32(rows+4*y,320*y);
    ck(uc_mem_write(u,0x80000,rows,sizeof rows));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
        UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned calls=0,fallbacks=0;
    for(unsigned trial=0;trial<1024;++trial){
        static unsigned char actual[N],expected[N],got[N];
        for(unsigned j=0;j<N;++j)actual[j]=rnd();
        unsigned order[199];for(unsigned j=0;j<199;++j)order[j]=j+1;
        for(unsigned j=198;j;j--){unsigned k=rnd()%(j+1),t=order[j];order[j]=order[k];order[k]=t;}
        unsigned count=trial%200;
        be32(actual+CHUNKY,BASE+PIX);be16(actual+COUNT,0);
        for(unsigned j=0;j<count;++j){
            unsigned h=order[j];actual[NEXT+h]=j+1<count?order[j+1]:0;
            /* Include homogeneous chains and alternating/mixed chains. */
            unsigned sprite=trial%4==0?0:trial%4==1?1:rnd()%2;
            be16(actual+INDEX+2*h,sprite?0xffff:j);
            if(!sprite){
                unsigned char *p=actual+PART+j*24;p[20]=rnd()%8;
                /* Repeated pixels deliberately test exact ordering. */
                unsigned x=trial%3?rnd()%320:31,y=trial%3?rnd()%200:47;
                be16(p+12,p[20]&1?x:0xffff);be16(p+14,p[20]&1?y:0xffff);
            }else{
                unsigned char *a=actual+ACTORS+h*164,*p=actual+PREV+h*12;
                a[30]=1+rnd()%12;a[31]=1+rnd()%10;
                be16(a+26,trial%3?rnd()%308:30);be16(a+28,trial%3?rnd()%190:46);
                a[21]=1+rnd()%3;a[32]=1;a[33]=0;
                switch((trial+j)%13){
                case 0:a[32]=0;break;
                case 1:be16(a+26,0xffff);break;
                case 2:be16(a+28,199);break;
                case 3:a[30]=0;break;
                case 4:a[31]=0;break;
                case 5:be16(a+26,319);break;
                case 6:be16(a+28,0xffff);break;
                case 7:case 8:
                    /* NEXT producer has a matching descriptor, kind cleared. */
                    memcpy(p,a+26,6);p[6]=0;p[7]=a[20];p[8]=a[16];p[9]=a[24];
                    p[10]=a[22];p[11]=a[23];a[33]=1;break;
                default:break;
                }
            }
        }
        memcpy(expected,actual,N);
        unsigned first=count?order[0]:0;
        do{
            unsigned stop=reference(expected,first);
            ck(uc_mem_write(u,BASE,actual,N));
            unsigned char args[12];be32(args,STOP);be32(args+4,BASE);be32(args+8,first);
            ck(uc_mem_write(u,STACK,args,sizeof args));unsigned sp=STACK;
            ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
            for(unsigned r=0;r<11;++r){unsigned v=0xa5000000u+r*0x10101+trial;ck(uc_reg_write(u,regs[r],&v));}
            ck(uc_emu_start(u,entry,STOP,0,1000000));++calls;
            unsigned pc,result;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_D0,&result));
            ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
            if(pc!=STOP || result!=stop || sp!=STACK+4){fprintf(stderr,"Return mismatch trial=%u got=%u expected=%u pc=%x\n",trial,result,stop,pc);return 1;}
            for(unsigned r=0;r<11;++r){unsigned v;ck(uc_reg_read(u,regs[r],&v));if(v!=0xa5000000u+r*0x10101+trial)return 1;}
            ck(uc_mem_read(u,BASE,got,N));
            if(memcmp(got,expected,N)){
                for(unsigned j=0;j<N;++j)if(got[j]!=expected[j]){
                    fprintf(stderr,"Trial %u first=%u offset=%u got=%u expected=%u\n",trial,first,j,got[j],expected[j]);break;
                }
                return 1;
            }
            memcpy(actual,got,N);
            if(!stop)break;
            /* Apply the independent clipped/unsaved fallback to both states,
             * then check resumption without resetting metadata or pixels. */
            sprite_restore(actual,stop);sprite_restore(expected,stop);++fallbacks;
            first=actual[NEXT+stop];
        }while(first);
    }
    uc_close(u);printf("Mixed actor restoration: 1024 chains, %u native segments, %u fallback resumptions; exact pixels/state/canaries/register ABI passed\n",calls,fallbacks);
    return 0;
}
