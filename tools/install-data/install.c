/* Native, bounded installer for the publisher's Slix151.zip.
 * Only original runtime data is extracted; executables, keys and settings
 * are never selected. Destination must be new: existing saves are untouched. */
#include "io.h"
#include "sha256.h"
#include "puff.h"
#include <string.h>
static unsigned char packed[300000], plain[650000];
static char created[210][32];
static unsigned count;
static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static unsigned le16(const unsigned char *p) { return p[0]|(unsigned)p[1]<<8; }
static int eq(const char *a,const char *b) { return strlen(a)==strlen(b) && !memcmp(a,b,strlen(a)); }
static int path(char *out,const char *dir,const char *name)
{
    unsigned n=strlen(dir),m=strlen(name);
    if(!n || n+m+2>512) return 0;
    memcpy(out,dir,n); if(out[n-1]!=':' && out[n-1]!='/') out[n++]='/';
    memcpy(out+n,name,m+1);return 1;
}
static int selected(const char *s)
{
    unsigned n=strlen(s);
    if(eq(s,"SLICKS.000") || eq(s,"SLICKS.DAT")) return 1;
    if(n<11 || n>19 || memcmp(s,"TRACKS/",7) || !eq(s+n-3,".SS")) return 0;
    for(unsigned i=7;i<n-3;++i)
        if(!((s[i]>='A' && s[i]<='Z') || (s[i]>='0' && s[i]<='9') || s[i]=='_' || s[i]=='-')) return 0;
    return 1;
}
static uint32_t crc32(const unsigned char *p,uint32_t n)
{
    uint32_t c=0xffffffffUL;
    while(n--) { c^=*p++; for(unsigned i=0;i<8;++i)c=(c>>1)^(0xedb88320UL & (0UL-(c&1))); }
    return ~c;
}
int install_data(const char *source,const char *dest,const char *temp)
{
    static const unsigned char expected[32]={
        0xd1,0x21,0x14,0xfc,0xcd,0xd8,0x6b,0x10,0xe9,0xd7,0x7d,0x65,0x2a,0xce,0xc3,0x5e,
        0xa0,0xa4,0x52,0x30,0xfc,0xbe,0x3f,0xb3,0x29,0x70,0x43,0x1c,0x18,0xb5,0x5c,0x2c};
    File f={0},out={0};SHA256 sha;unsigned char hash[32],header[30];
    char name[256],target[512];int made=0,tracks=0,ok=0;unsigned core=0;
    (void)temp;count=0;
    if(io_exists(dest)) { io_message("Destination exists; refusing to replace files.");return 20; }
    if(!path(target,dest,"TRACKS") || !io_open(&f,source,0)) goto done;
    io_message("Checking original Slix151.zip...");
    sha_init(&sha);
    while(f.pos<f.size) {
        uint32_t n=f.size-f.pos;if(n>sizeof packed)n=sizeof packed;
        if(io_cancelled() || !io_read(&f,packed,n)) goto done;
        sha_update(&sha,packed,n);
    }
    sha_final(&sha,hash);
    if(memcmp(hash,expected,32)) { io_message("Unsupported or damaged ZIP. Use the original Slix151.zip download.");goto done; }
    if(!io_seek(&f,0) || !io_mkdir(dest)) goto done;
    made=1;if(!io_mkdir(target)) goto done;tracks=1;
    io_message("Extracting verified game data and tracks...");
    while(f.pos+30<=f.size) {
        if(io_cancelled() || !io_read(&f,header,30)) goto done;
        if(le32(header)==0x02014b50UL) break;
        if(le32(header)!=0x04034b50UL || (le16(header+6)&9)) goto done;
        uint32_t compressed=le32(header+18),size=le32(header+22);
        unsigned method=le16(header+8),namesize=le16(header+26),extra=le16(header+28);
        if(namesize>=sizeof name || namesize+extra>f.size-f.pos) goto done;
        if(!io_read(&f,name,namesize))goto done;
        name[namesize]=0;
        if(strlen(name)!=namesize || !io_seek(&f,f.pos+extra) || compressed>f.size-f.pos)goto done;
        if(!selected(name)) { if(!io_seek(&f,f.pos+compressed))goto done;continue; }
        if(count>=210 || size>sizeof plain || compressed>sizeof packed || !path(target,dest,name) || io_exists(target))goto done;
        if(!io_read(&f,packed,compressed))goto done;
        unsigned long in=compressed,n=size;
        if(method==8) { if(puff(plain,&n,packed,&in) || n!=size || in!=compressed)goto done; }
        else if(method==0 && compressed==size)memcpy(plain,packed,size);
        else goto done;
        if(crc32(plain,size)!=le32(header+14) || !io_open(&out,target,1))goto done;
        memcpy(created[count++],name,namesize+1);
        if(!io_write(&out,plain,size) || !io_close(&out))goto done;
        if(eq(name,"SLICKS.000"))core|=1;
        if(eq(name,"SLICKS.DAT"))core|=2;
    }
    if(core!=3 || count!=197 || !io_close(&f))goto done;
    ok=1;io_message("Installed 2 original data files and 195 tracks. No key or settings copied.");
done:
    io_close(&out);io_close(&f);
    if(!ok && made) {
        while(count) { --count;if(path(target,dest,created[count]))io_remove(target); }
        if(tracks && path(target,dest,"TRACKS"))io_remove(target);
        io_remove(dest);
    }
    if(!ok)io_message("Installation failed or cancelled; existing installations were not changed.");
    return ok?0:20;
}
