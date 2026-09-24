/* macOS diagnostic: target only the explicitly supplied FS-UAE process.
 * F6/F7/F8 are mapped by debug.sh, never by the normal launcher. */
#include <ApplicationServices/ApplicationServices.h>
#include <libproc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    if(!CGPreflightPostEventAccess()) {
        fputs("Host event posting is not authorized; no event sent.\n",stderr);
        return 2;
    }
    if(argc==2 && !strcmp(argv[1],"--check")) {
        puts("HOST_EVENT_POSTING_AVAILABLE"); return 0;
    }
    if(argc!=4) return 2;
    char *end; long value=strtol(argv[1],&end,10);
    if(*end || value<=1 || value>0x7fffffff) return 2;
    pid_t pid=(pid_t)value; char path[PROC_PIDPATHINFO_MAXSIZE];
    if(proc_pidpath(pid,path,sizeof path)<=0) return 2;
    const char *base=strrchr(path,'/');
    if(!base || strcmp(base+1,"fs-uae")) { fputs("Target is not FS-UAE\n",stderr); return 2; }
    CGKeyCode key;
    if(!strcmp(argv[2],"F6")) key=97;
    else if(!strcmp(argv[2],"F7")) key=98;
    else if(!strcmp(argv[2],"F8")) key=100;
    else return 2;
    if(strcmp(argv[3],"down") && strcmp(argv[3],"up")) return 2;
    CGEventRef event=CGEventCreateKeyboardEvent(NULL,key,!strcmp(argv[3],"down"));
    if(!event) return 2;
    CGEventPostToPid(pid,event); CFRelease(event);
    printf("FS-UAE pid %d: %s %s posted\n",pid,argv[2],argv[3]);
    return 0;
}
