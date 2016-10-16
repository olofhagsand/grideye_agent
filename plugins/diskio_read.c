/*! Random disk read
 * Precondition: a large file, typically 2*ram size
 * For every test:
 *   - Open the existing file.
 *   - Read 'ior' bytes from a random position using lseek(2)
 *   - Close the file
 * compile:
 *   gcc -O2 -Wall -o diskio_read diskio_read.c
 * run: 
 *   /bin/dd if=/dev/zero of=GRIDEYE_LARGEFILE bs=1M count=1K # optional
 *   ./diskio_read GRIDEYE_LARGEFILE 1024
 * 
 *  Copyright (C) 2015-2016 Olof Hagsand
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "grideye_plugin_v1.h"

static int debug = 0;

/* Use global variables, could encapsulate in handle */
static char    *_filename = NULL;
static uint64_t _filesize = 0;

int
diskio_read_exit(void)
{
    unlink(_filename);
    if (_filename)
	free(_filename);
    return 0;
}

/*
 * @param[in]  len      Nr of bytes to read from file
 * @param[out]  t_us  Latency in micro-seconds
 */
int
diskio_read_test(int       len,
		 char    **outstr)
{                                                                      
    int      retval = -1;
    int      fd;
    char    *buf = NULL;
    ssize_t  pos;
    off_t    off;
    int      n;
    int      i; 
    struct timeval t0;
    struct timeval t1;
    struct timeval dt;  /* t1-t0 */
    uint64_t t_us;
    char    *str = NULL;
    size_t   slen;

    if (len == 0){
	retval = 0;
	goto done;
    }
    if (len > _filesize)
	len = _filesize;
    if ((buf = malloc(len)) == NULL)
	goto done;
    n = _filesize/len;
    i = random()%n;
    off = i*len;
    gettimeofday(&t0, NULL);
    /* open file and read from it */
    if ((fd = open(_filename, O_RDONLY)) < 0)
	goto done;
    if (lseek(fd, off, SEEK_SET) < 0)
	goto done;
    if ((pos = read(fd, buf, len)) < 0)
	goto done;
    if (pos != len)
	goto done;
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    close(fd);
    t_us = dt.tv_usec+dt.tv_sec*1000000;
    if (debug)
      fprintf(stderr, "%s: %s size: %d:%d\n", 
	      __FUNCTION__, _filename, (int)off, (int)_filesize);
    if ((slen = snprintf(NULL, 0, "<tior>%" PRIu64 "</tior>", t_us)) <= 0)
	goto done;
    if ((str = malloc(slen+1)) == NULL)
	goto done;
    snprintf(str, slen+1, "<tior>%" PRIu64 "</tior>", t_us);
    *outstr = str;
    retval = 0;
 done:
    if (buf)
	free(buf);
    return retval;
}

/*! IO read init code.  Called when agent starts
 * Compute file size unless given
 * Fill file with zeroes
 * Create file
 * @param[in]      filename  Filename of read file.
 * @param[in,out]  filesizep Size of file in bytes. If 0 set to 2xRAM
 * @retval     -1  open(RDONLY) or lseek fail
 * @retval     0   OK
 */
int
diskio_read_file(const char *writefile, 
		 const char *largefile,
		 const char *device)
{
    int      retval = -1;
    //    uint64_t ram = 0;    
    int      page_size;
    int      num_pages;
    int      fd = -1;
    struct stat st;

    if (largefile == NULL || !strlen(largefile)){
	errno = EINVAL;
	goto done;
    }
    if (stat(largefile, &st) < 0){
	errno = ENOENT;
	goto done;
    }
    if ((_filename = strdup(largefile)) == NULL)
	return -1;
    /*
     * Compute file size unless given: 2xRAM size
     */

    if ((page_size = sysconf(_SC_PAGESIZE)) < 0)
	goto done;
    if ((num_pages = sysconf(_SC_PHYS_PAGES)) < 0)
	goto done;
    //    ram = page_size/1024 * (num_pages/1024);
    //    fprintf(stderr, "%s ramsize: %" PRIu64 " Mbytes\n", __FUNCTION__, ram);    
    if ((fd = open(_filename, O_RDONLY)) < 0)
	goto done;
    if ((_filesize = lseek(fd, 0, SEEK_END)) < 0)
	goto done;
    retval = 0;
 done:
    return retval;
}

static const struct grideye_plugin_api_v1 api = {
    1,
    GRIDEYE_PLUGIN_MAGIC,
    diskio_read_exit,
    diskio_read_test,
    diskio_read_file,
    "ior", /* input param */
    "xml"  /* output format */
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

    int            b;
    char          *f;
    char          *str = NULL;

    if (argc != 3){
	fprintf(stderr, "usage %s <file> <bytes>\n", argv[0]);
	return -1;
    }
    f = argv[1];
    b = atoi(argv[2]);
    if (grideye_plugin_init_v1(1) < 0)
	return -1;
    if (diskio_read_file(NULL, f, NULL) < 0)
	return -1;
    if (diskio_read_test(b, &str) < 0)
	return -1;
    fprintf(stdout, "%s\n", str);
    free(str);
    diskio_read_exit();
    return 0;
}
#endif
