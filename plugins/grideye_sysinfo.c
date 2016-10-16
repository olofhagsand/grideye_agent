/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_sysinfo grideye_sysinfo.c
 * run: 
 *   ./grideye_sysinfo
 */
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sysinfo.h>

#include "grideye_plugin_v1.h"

#define LINUX_SYSINFO_LOADS_SCALE (65536)
#define PERCENT (100)
#define DEC_3 (1000)

int 
sysinfo_exit(void)
{
    return 0;
}

/*! Poll sysinfo command for system status
 * @param[out]  outstr  XML string with three parameters described below
 * The string contains the following parameters (or possibly a subset):
 * iwessid
 * iwaddr
 * iwchan
 * iwfreq
 * iwproto
 */
int  
sysinfo_test(int        dummy,
	     char     **outstr)
{
    int             retval = -1;
    struct sysinfo  info;
    unsigned long   uptime;
    unsigned long   loads;
    unsigned long   freeram;
    unsigned long   usedram;
    unsigned long   bufferram;
    unsigned long   procs;
    unsigned long   freeswap;
    unsigned long   usedswap;
    int             memunit;
    char           *str = NULL;
    size_t          slen;

    if (sysinfo(&info) < 0){
	perror("sysinfo");
	goto done;
    }
    memunit = info.mem_unit;
    uptime = info.uptime;
    loads = ((info.loads[0]*PERCENT*DEC_3)/LINUX_SYSINFO_LOADS_SCALE);
    freeram = info.freeram*memunit;
    usedram = info.totalram*memunit - info.freeram*memunit;
    bufferram = info.bufferram*memunit;
    procs = info.procs;
    freeswap = info.freeswap*memunit;
    usedswap = info.totalswap*memunit - info.freeswap*memunit;
    
    if ((slen = snprintf(NULL, 0,
			 "<uptime>%lu</uptime>"
			 "<loads>%lu.%lu</loads>"
			 "<freeram>%lu</freeram>"
			 "<usedram>%lu</usedram>"
			 "<bufferram>%lu</bufferram>"
			 "<procs>%lu</procs>"
			 "<freeswap>%lu</freeswap>"
			 "<usedswap>%lu</usedswap>", 
			 uptime,
			 loads/DEC_3, loads%DEC_3,
			 freeram,
			 usedram,
			 bufferram,
			 procs,
			 freeswap,
			 usedswap)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, 
	     "<uptime>%lu</uptime>"
	     "<loads>%lu.%lu</loads>"
	     "<freeram>%lu</freeram>"
	     "<usedram>%lu</usedram>"
	     "<bufferram>%lu</bufferram>"
	     "<procs>%lu</procs>"
	     "<freeswap>%lu</freeswap>"
	     "<usedswap>%lu</usedswap>", 
	     uptime,
	     loads/DEC_3, loads%DEC_3,
	     freeram,
	     usedram,
	     bufferram,
	     procs,
	     freeswap,
	     usedswap);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    sysinfo_exit,
    sysinfo_test,
    NULL,       /* file callback */
    NULL,       /* input param */
    "xml"       /* output format */
};

/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init_v1(int version)
{
    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    return (void*)&api;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init_v1(1) < 0)
	return -1;
    if (sysinfo_test(0, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    sysinfo_exit();
    return 0;
}
#endif
