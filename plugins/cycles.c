/* Code thanks to Torbjörn Granlund tg@gmplib.org
 *
 * This is a basic BogoMips test running a specified number of empty loops
 *
 * compile:
 *   gcc -O2 -Wall -o cycles cycles.c cycles_test.s
 * run: 
 *   ./cycles
 */
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "grideye_plugin_v1.h"

int 
cycles_exit(void)
{
  return 0;
}

void cycles_test1(void);

int 
cycles_test(int    dummy, 
	    char **outstr)
{
    int             retval = -1;
    struct timespec ts;
    uint64_t        t0;
    uint64_t        t1;
    uint64_t        t_us;
    char           *str = NULL;
    size_t          slen;
    //    int             j;

    //    for (j=0;j<20;j++)
    //	cycles_test1();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    t0 = 1000000000ul * ts.tv_sec + ts.tv_nsec;

    cycles_test1();

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    t1 = 1000000000ul * ts.tv_sec + ts.tv_nsec;

    // 13 comes from the 13 insns in foo's loop
    // 1000000000 comes from the loop count in foo
    // The result is billions as the time in ns is inverted here.
    //  fprintf(stderr, "%.2f billion insns/sec\n", (double) 13 * 1000000000 / (t1 - t0));
    //  fprintf(stderr, "%.2f billion insns/sec\n", (double) 13 * 10000000 / (t1 - t0));
    t_us = (t1-t0)/1000;
    if ((slen = snprintf(NULL, 0, "<tcyc>%" PRIu64 "</tcyc>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tcyc>%" PRIu64 "</tcyc>", t_us);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    cycles_exit,
    cycles_test,
    NULL, /* file fn */
    NULL, /* input param */
    "xml" /* output format */
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

    if (grideye_plugin_init_v1(1) == NULL)
	return -1;
    if (cycles_test(0, &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    if (cycles_exit() < 0)
	return -1;
    return 0;
}
#endif
