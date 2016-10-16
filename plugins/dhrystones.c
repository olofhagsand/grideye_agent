/*! Dhrystone compute test
 * For every test:
 *   - run dhrystone test for a specificed number of dhrystones and measure 
 *     latency
 * compile:
 *   gcc -O2 -Wall -o dhrystones dhrystones.c dhry_1.c dhry_2.c
 * run: 
 *   ./dhrystones 30000
 * 
 *  Author: Reinhold P. Weicker
 *  Olof Hagsand: wrapping code
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>

#include "grideye_plugin_v1.h"

int
dhrystones_exit(void)
{
    return 0;
}

extern int dhry_main(int Number_Of_Runs, int display); /* In dhry_1.c */

int
dhrystones_test(int        dhrystones,
		char     **outstr)
{
    int            retval = -1;
    struct timeval t0;
    struct timeval t1;
    struct timeval dt;  /* t1-t0 */                                            
    size_t         slen;
    uint64_t       t_us;
    char          *str = NULL;

    gettimeofday(&t0, NULL);
    if (dhry_main(dhrystones, 0) < 0)
	goto done;
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    t_us = dt.tv_usec+dt.tv_sec*1000000;
    if ((slen = snprintf(NULL, 0, "<tcmp>%" PRIu64 "</tcmp>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tcmp>%" PRIu64 "</tcmp>", t_us);
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    dhrystones_exit,
    dhrystones_test,
    NULL, /* file */
    "cmp", /* input params */
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
int 
main(int   argc, 
     char *argv[])
{
    int     d;
    char   *str = NULL;

    if (argc != 2){
	fprintf(stderr, "usage %s <dhrystones>\n", argv[0]);
	return -1;
    }
    d = atoi(argv[1]);
    grideye_plugin_init_v1(1);
    if (dhrystones_test(d, &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    dhrystones_exit();
    return 0;
}
#endif
