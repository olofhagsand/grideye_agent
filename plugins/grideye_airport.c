/* 
 * 
 * compile:
 *   gcc -O2 -Wall -o grideye_airport grideye_airport.c -lm
 * run: 
 *   ./grideye_airport
Program att använda:
$ /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I
    agrCtlRSSI: -44
    agrExtRSSI: 0
   agrCtlNoise: -92
   agrExtNoise: 0
         state: running
       op mode: station
    lastTxRate: 878
       maxRate: 217
lastAssocStatus: 0
   802.11 auth: open
     link auth: wpa2
         BSSID: 58:b6:33:79:3e:7c
          SSID: eduroam
           MCS: 7
       channel: 124,80 


De värdena som är av intresse är:
agrCtlRSSI = signalstyrka (wlevel)
agrCtlNoise = brusstryka (wnoise)
BSSID = AP mac (iwaddr)
channel = kanal (iwchan), kanalbredd
 */
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "grideye_plugin_v1.h"

static const char *_airport_prog = "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport";
static char *_device = NULL;

int 
airport_exit(void)
{
    if (_device){
	free(_device);
	_device = NULL;
    }
    return 0;
}

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
airport_fork(const char *prog,
	   const char *arg1,
	   const char *arg2,
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

/*!
 * @param[in]  p      The string to use (+ offset)
 * @param[in]  offset Added offset on string
 * XXX: Why remove and then add double quotes?
 */
static int
stringadd(char  *p,
	  int    offset,
	  char  *keyword,
	  char **s0,
	  int   *s0len)
{
    int retval = -1;
    int slen;

    if (p==NULL)
	return 0;
    p += offset;
    if (p[0] == '\"')
	p++;
    /* Strip optional " if they exists and add them (again) below */
    if (p[strlen(p)-1] == '\"')
	p[strlen(p)-1] = '\0';
    slen = strlen(p)+2*strlen(keyword)+5+2; /* 7 is <>""</>  */
    if ((*s0 = realloc(*s0, *s0len+slen+1)) == NULL){
	perror("realloc");
	goto done;
    }	
    /* Always add double quotes (may have been removed above) */
    snprintf(*s0+*s0len, slen+1, "<%s>\"%s\"</%s>", keyword, p, keyword);
    *s0len += slen;
    retval = 0;
 done:
    return retval;
}

/*! Poll /proc wireless file for device status 
 * @param[out]  outstr  XML string with three parameters described below
 * The string contains the following parameters (or possibly a subset):
 * iwessid
 * iwaddr
 * iwchan
 * iwfreq
 * iwproto
 */
int  
airport_test(int       dummy,
	  char     **outstr)
{
    int             retval = -1;
    char            buf[256];
    int             buflen = sizeof(buf);
    int             len;
    char           *s0=NULL; /* whole string */
    int             s0len=0; /* length of whole string */
    char           *p;

    /* 
     *  The fork code executes iwgetid and returns a string
     */
    buflen = sizeof(buf);
    if ((len = airport_fork(_airport_prog, _device, "-r", buf, buflen)) < 0)
	goto done;
    if (len && stringadd(buf, 0, "iwessid", &s0, &s0len) < 0)
	goto done;

    if ((len = airport_fork(_airport_prog, _device, "-a", buf, buflen)) < 0)
	goto done;
    if (len && stringadd(rindex(buf, ' '), 1, "iwaddr", &s0, &s0len) < 0)
	goto done;

    if ((len = airport_fork(_airport_prog, _device, "-c", buf, buflen)) < 0)
	goto done;
    if (len && stringadd(rindex(buf, ':'), 1, "iwchan", &s0, &s0len) < 0)
	goto done;
    
    if ((len = airport_fork(_airport_prog, _device, "-f", buf, buflen)) < 0)
	goto done;
    /* Strange: sometimes ':', sometimes '=' */
    if ((p = rindex(buf, ':')) == NULL)
	p = rindex(buf, '=');
    if (len && stringadd(p, 1, "iwfreq", &s0, &s0len) < 0)
	goto done;

    if ((len = airport_fork(_airport_prog, _device, "-p", buf, buflen)) < 0)
	goto done;
    if (len && stringadd(rindex(buf, ':'), 1, "iwproto", &s0, &s0len) < 0)
	goto done;
    *outstr = s0;
    retval = 0;
 done:
    return retval;
}

/*! Init grideye test module. Check if file exists that is used for polling state */
int 
airport_file(const char *filename, 
	   const char *largefile,
	   const char *device)
{
    int         retval = -1;

    if (device == NULL){
	errno = EINVAL;
	goto done;
    }
    if ((_device = strdup(device)) == NULL)
	goto done;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    airport_exit,
    airport_test,
    airport_file, /* file callback */
    NULL,       /* input param */
    "xml"       /* output format */
};

/* Grideye agent plugin init function must be called grideye_plugin_init */
void *
grideye_plugin_init_v1(int version)
{
    struct stat st;

    if (version != GRIDEYE_PLUGIN_VERSION)
	return NULL;
    if (stat(_airport_prog, &st) < 0)
	return NULL;
    return (void*)&api;
}

#ifndef _NOMAIN
int main() 
{
    char   *str = NULL;

    if (grideye_plugin_init_v1(1) < 0)
	return -1;
    if (airport_file(NULL, NULL, "wlan0") < 0)
	return -1;
    if (airport_test(0, &str) < 0)
	return -1;
    if (str){
	fprintf(stdout, "%s\n", str);
	free(str);
    }
    airport_exit();
    return 0;
}
#endif
