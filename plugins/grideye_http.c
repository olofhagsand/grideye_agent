/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_http grideye_http.c
 * run: 
 *   ./grideye_http
 */
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "grideye_plugin_v1.h"

#define _PROGRAM "/usr/lib/nagios/plugins/check_http"

/*! Fork and exec a iwget process
 * @param[in]  prog   Program to exec, eg iwgetid
 * @param[in]  arg1   First argument, eg device
 * @param[in]  arg2   Second argument argument, eg -a, -c, -f, p or NULL
 * @param[in]  buf    Buffer to store output from process
 * @param[in]  buflen Length of buffer
 * @retval -1         Error
 * @retval  n         Number of bytes read
 * @note Assume that the output (if any) is terminated by a LF/CR, or some other
 * non-printable character which can be replaced with \0 and yield a string
 */
static int 
http_fork(const char *prog,
	   const char *arg1,
	   const char *arg2,
	  const char *arg3,
	   char       *buf,
	   int         buflen)
{
    int     retval = -1;
    int     stdin_pipe[2];
    int     stdout_pipe[2];
    int     pid;
    int     status;
    int     len;

    if (pipe(stdin_pipe) < 0){
	perror("pipe");
	goto done;
    }
    if (pipe(stdout_pipe) < 0){
	perror("pipe");
	goto done;
    }
    if ((pid = fork()) != 0){ /* parent */
	close(stdin_pipe[0]); 
	close(stdout_pipe[1]); 
    }
    else { /* child */
	close(0);
	if (dup(stdin_pipe[0]) < 0){
	    perror("dup");
	    return  -1;
	}
	close(stdin_pipe[0]); 
	close(stdin_pipe[1]); 
	close(1); 
	if (dup(stdout_pipe[1]) < 0){
	    perror("dup");
	    return  -1;
	}
	close(stdout_pipe[1]);  
	close(stdout_pipe[0]); 

	if (execl(prog, prog, 
		  arg1,
		  arg2,
		  arg3,
		  (void*)0) < 0){
	    perror("execl");
	    return -1;
	}
	exit(0);
    }
    if (pid < 0){
	perror("fork");
	goto done;
    }
    close(stdin_pipe[1]);
    if ((len = read(stdout_pipe[0], buf, buflen)) < 0){
	perror("read");
	goto done;
    }
    if (len>0)
	buf[len-1] = '\0';
    close(stdout_pipe[0]);
    waitpid(pid, &status, 0);
    retval = len;
 done:
    return retval;
}


/*! Run Nagios check_http plugin
 * @param[out]  outstr  XML string with three parameters described below

HTTP OK: HTTP/1.1 200 OK - 518853 bytes in 1.418 second response time |time=1.417916s;;;0.000000 size=518853B;;;0

 */
int  
http_test(int        host,
	  char     **outstr)
{
    int    retval = -1;
    char   buf[1024];
    int    buflen = sizeof(buf);
    char   code0[64], code1[64];
    int    size;
    double time;
    char  *str = NULL;
    size_t slen;

    if (http_fork(_PROGRAM, "-H", "www.youtube.com", "-S", buf, buflen) < 0)
	goto done;
    sscanf(buf, "%*s %*s %*s %s %s %*s %d %*s %*s %lf\n",
	   code0, code1, &size, &time);
    if ((slen = snprintf(NULL, 0, 
			 "<hstatus>\"%s %s\"</hstatus>"
			 "<htime>%d</htime>"
			 "<hsize>%d</hsize>",
			 code0, code1,
			 (int)(time*1000),
			 size)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    if ((slen = snprintf(str, slen+1, 
			 "<hstatus>\"%s %s\"</hstatus>"
			 "<htime>%d</htime>"
			 "<hsize>%d</hsize>",
			 code0, code1,
			 (int)(time*1000),
			 size)) <= 0)
	goto done;
    *outstr = str;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    NULL,
    http_test,
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
    if (http_test(0, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    return 0;
}
#endif
