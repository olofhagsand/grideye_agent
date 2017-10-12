/*
  Copyright (C) 2015-2016 Olof Hagsand

  This file is part of GRIDEYE.

  GRIDEYE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  GRIDEYE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GRIDEYE; see the file LICENSE.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <net/if.h> 

#include <cligen/cligen.h>
#include <clixon/clixon.h>

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h> /* Dont remove: SIOCGIFADDR will be undefined below */
#elif defined(HAVE_SYS_SOCKIO_H)
#include <sys/sockio.h> /* Dont remove: SIOCGIFADDR will be undefined below */
#endif

#include "grideye_agent.h"

static int 
short2Bytes(int16_t h, 
	    char   *b, 
	    int     offset) 
{
    int16_t n = htons(h);

    memcpy(&b[offset], (char*)&n, 2);
    return 0;
}	

static int 
int2Bytes(int32_t h, 
	  char   *b, 
	  int     offset) 
{
    int32_t n = htonl(h);

    memcpy(&b[offset], (char*)&n, 4);
    return 0;
}	

static uint16_t
Bytes2short(char *b, 
	    int   offset) 
{
    uint16_t n;

    memcpy((char*)&n, &b[offset], 2);
    return ntohs(n);
}	

static uint32_t
Bytes2int(char *b, 
	  int   offset) 
{
    uint32_t n;

    memcpy((char*)&n, &b[offset], 4);
    return ntohl(n);
}	

uint64_t
timeval2twamp(struct timeval tv)
{
    uint64_t sec = tv.tv_sec; //+ 2208988800L;
    uint64_t q = ((uint64_t)(tv.tv_usec*1000)) << 32;
    uint64_t subsec = q/1000000000L;
    return (sec<<32) + subsec;
}

#ifdef notused
uint64_t
twamp2ns(long ts)
{ 
    uint64_t nsfrac = ts&0x00000000ffffffffL;
    uint64_t nsonly = (nsfrac*1000000000L)>>32;
    uint64_t sec = (ts&0xffffffff00000000L)>>32;
    return sec*1000000000L + nsonly;
}
#endif
int
twamp2timeval(uint64_t        ts, 
	      struct timeval *tv)
{ 
    uint64_t nsfrac = ts&0x00000000ffffffffL;
    uint64_t nsonly = (nsfrac*1000000000L)>>32;
    tv->tv_sec = (ts&0xffffffff00000000L)>>32;
    tv->tv_usec = nsonly/1000;
    return 0;
}

/*! Encode Twoway protocol header
 * @param[in,out] msg      Buffer for encoding
 * @param[in]     pktlen   Length of buffer
 * @param[in]     tn       Structured twoway protocol header  
 * @param[in]     payload  A string directly after the header
 */
int
encode_twoway(char              *msg, 
	      int                pktlen, 
	      struct twoway_hdr *th, 
	      char              *payload)
{
    int        retval = -1;
    char      *b = msg;
    int        p = 0;
    int        i;
    int        plen;

    if (strlen(payload))
	if (60 + strlen(payload)+1 > pktlen){
	    clicon_log(LOG_WARNING, "%s: Packet too short (%u) for header (%u) and payload (%u) '%s'", 
		       __FUNCTION__, 
		       pktlen, 
		       60, 
		       (uint32_t)strlen(payload)+1, 
		       payload);
	}
    b[p++] = th->th_ver; 
    b[p++] = th->th_mtype; 
    short2Bytes(th->th_label, b, p); p += 2;

    int2Bytes(th->th_tag, b, p); p += 4;//0xcf30e506
    b[p++] = th->th_ver2; // Version
    b[p++] = th->th_rcode; // Reflector code
    b[p++] = th->th_ttl; p+=3; // ttl + padding
    b[p++] = th->th_tos; p+=3; // tos + padding 
    short2Bytes(th->th_inc, b, p); p+=2; // Incnumber
    int2Bytes(th->th_seq0, b, p); p+=4; // seq-ul
    int2Bytes(th->th_seq1, b, p); p+=4; // seq-dl
    int2Bytes(th->th_t0.tv_sec, b, p); p+=4; // t0 - s
    int2Bytes(th->th_t0.tv_usec, b, p); p+=4; // t0 - us
    if (p>pktlen-4)
	goto skip;
    int2Bytes(th->th_t1.tv_sec, b, p); p+=4; 
    if (p>pktlen-4)
	goto skip;
    int2Bytes(th->th_t1.tv_usec, b, p); p+=4;
    if (p>pktlen-4)
	goto skip;
    int2Bytes(th->th_t2.tv_sec, b, p); p+=4; 
    if (p>pktlen-4)
	goto skip;
    int2Bytes(th->th_t2.tv_usec, b, p); p+=4;
    p+=8; /* t3 */
    assert(p == 60);
    //    fprintf(stderr, "%s: %lu %d %d\n", __FUNCTION__, strlen(payload)+1, pktlen, p);
    plen = strlen(payload)+1;
    if (p + plen > pktlen)
	plen = pktlen-p;
    /*
     * Truncate payload  <-----strlen(payload)+1----->
     *                  p  plen              pktlen
     * --------------------------------------|
     */
    for (i = 0; i< plen; i++)
	b[p++] = payload[i];  
    if (plen != strlen(payload)+1)
	b[pktlen-1] = '\0';
 skip:
    if (p<pktlen)
	memset(&b[p], 0, pktlen-p); 
    p = pktlen; /* padding */
    retval = 0;
    // done:
    return retval;
}

/*! Decode twoway header
 * @param[in]  msg     Packet buffer
 * @param[in]  pktlen  Packet length
 * @param[out] th      Twoway header
 * @param[out] payload pointer to string in payload
 * @param[out] rlen    length of returned header
 */
int
decode_twoway(char              *msg, 
	      int                pktlen, 
	      struct twoway_hdr *th, 
	      char             **payload, 
	      uint32_t          *rlen)
{
    int        retval = -1;
    char      *b = msg;
    int        p = 0;

    if (pktlen < 60)
	goto done;
    th->th_ver     = b[p++];
    th->th_mtype   = b[p++];
    th->th_label   = Bytes2short(b, p); p += 2;
    th->th_tag     = Bytes2int(b, p); p += 4;// Tag
    th->th_ver2    = b[p++];
    th->th_rcode   = b[p++];
    th->th_ttl     = b[p++]; p+=3; // ttl + padding
    th->th_tos     = b[p++]; p+=3; // tos + padding
    th->th_inc     = Bytes2short(b, p); p+=2; // Incnumber
    th->th_seq0    = Bytes2int(b, p); p+=4; // seq-ul
    th->th_seq1    = Bytes2int(b, p); p+=4; // seq-dl
    th->th_t0.tv_sec  = Bytes2int(b, p); p+=4; // t0 - s
    th->th_t0.tv_usec = Bytes2int(b, p); p+=4; // t0 - us
    th->th_t1.tv_sec  = Bytes2int(b, p); p+=4; 
    th->th_t1.tv_usec = Bytes2int(b, p); p+=4;
    th->th_t2.tv_sec  = Bytes2int(b, p); p+=4; 
    th->th_t2.tv_usec = Bytes2int(b, p); p+=4;
    p+=8; // t3
    //    assert(p == 60);
    if ((pktlen>60) & (b[p] !=0) && (payload!=NULL)){
	if (pktlen - 60 < strlen(&b[p])+1)
	    goto done;
	*payload = &b[p];
	p += strlen(&b[p])+1;
    }
    if (rlen)
	*rlen = p;
    retval = 0;
 done:
    return retval;
}

/*! Marshal a control_hdr and an xml body to a char msg buf 
 * @param[out]  msg      Send msg buffer which will be written to
 * @param[in]   pktlen   Length of msg buffer
 * @param[in]   ch       Message header (common to twamp/twoway/control)
 * @param[in]   xstr     XML string.
 * @retval -1 error                                                             * @retval >0 number of bytes encoded.                                          * @see decode_control
 */
int
encode_control(char               *msg, 
	       int                 pktlen, 
	       struct control_hdr *ch, 
	       char               *xstr)
{
    int        retval = -1;
    char      *b = msg;
    int        p = 0;
    cbuf      *cb = NULL;

    b[p++] = ch->ch_ver; 
    b[p++] = ch->ch_mtype; 
    b[p++] = ch->ch_pad1;
    b[p++] = ch->ch_pad2;
    int2Bytes(ch->ch_tag2, b, p); p += 4;

    /* cbuf used to encode xml tree into a string */
    if ((cb = cbuf_new()) == NULL){
	clicon_err(OE_PROTO, errno, "%s: cbuf_new: %s\n", __FUNCTION__, strerror(errno));
	goto done;
    }
    /* If xml given as XML tree (alt is as xstr see below */
    if (xstr){
	clicon_debug(1, "%s: %s", __FUNCTION__, xstr);
	if (strlen(xstr)+1 > pktlen-p){
	    fprintf(stderr, "%s: buffer too short for XML\n", __FUNCTION__);
	    goto done;
	}
	memcpy(b+p, xstr, strlen(xstr)+1);
	p += strlen(xstr)+1;
    }
    retval = p;
 done:
    if (cb)
	cbuf_free(cb);
    return retval;
}

/*! De-marshal a char msg_buf to control_hdr and an xml body 
 * @param[in]   msg         Recv msg buffer which will be written to
 * @param[in]   pktlen      Length of msg buffer
 * @param[in]   ch          Message header (common to twamp/twoway/control)
 * @param[out]  payload     (direct) pointer to string in payload
 * @param[out]  rlen        length of returned header
 * XXX: pktlen not checked
 * @see encode_control
 */
int
decode_control(char               *msg, 
	       int                 pktlen, 
	       struct control_hdr *ch, 
	       char              **payload,
	       uint32_t           *rlen)
{
    int    retval = -1;
    char  *b = msg;
    int    p = 0;

    ch->ch_ver     = b[p++];
    ch->ch_mtype   = b[p++];
    ch->ch_pad1    = b[p++];
    ch->ch_pad2    = b[p++];
    ch->ch_tag2    = Bytes2int(b, p); p += 4;
    if (b[p] !=0 && payload){
	*payload = &b[p];
	p += strlen(&b[p])+1;
    }
    if (rlen)
	*rlen = p;
    retval = 0;
    return retval;
}

/*
 * return current time.
 */
struct timeval
gettimestamp(void)
{  
    struct timeval t;
    gettimeofday(&t, NULL);
    return t;
}

/*
 * Pretty print a timeval in a static string.
 */
char *
timevalprint(struct timeval t1)
{
    static char s[64];

    if (t1.tv_sec < 0 && t1.tv_usec > 0){
	if (t1.tv_sec == -1)
	    snprintf(s, 64, "-%ld.%06ld", (long)t1.tv_sec+1, 1000000-(long)t1.tv_usec);
	else
	    snprintf(s, 64, "%ld.%06ld", (long)t1.tv_sec+1, 1000000-(long)t1.tv_usec);
    }
    else
	snprintf(s, 64, "%ld.%06ld", (long)t1.tv_sec, (long)t1.tv_usec);
    return s;
}

int
msgdump(FILE *f, 
	char *buf, 
	int   len)
{
    int i;

    for (i=0; i<len; i++){
      fprintf(f, "%02hx", (unsigned short)(buf[i]&0xff));
	if ((i+1)%32 == 0)
	    fprintf(f, "\n");
	else
        if ((i+1)%4 == 0)
	    fprintf(f, " ");
    }
    fprintf(f, "\n");

   return 0;
}

/*! Local address and udp port binding
 * ifaddr given -> use interface address otherwise use default
 * local port given -> use that otherwise ephemeral
 * @param[in]  s       Socket
 * @param[in]  inaddr  IPv4 interface address. Use if set, otherwise default
 * @param[in]  lport   UDP port. Use if set, otherwise ephemeral
 * @param[out] myaddr  Resulting local bound socket
 */
int
socket_bind_udp(int                 s,
		struct in_addr     *inaddr,
		uint16_t            lport,
		struct sockaddr_in *myaddr)
{
    int                retval = -1;
    socklen_t          myaddrlen = sizeof(*myaddr);

    clicon_log(LOG_DEBUG, "%s:%hu", __FUNCTION__, lport);
    memset(myaddr, 0, myaddrlen);
#ifdef HAVE_SIN_LEN
    myaddr->sin_len = myaddrlen;
#endif
    myaddr->sin_family = AF_INET;
    if (inaddr)
	memcpy(&myaddr->sin_addr, inaddr, sizeof(*inaddr));	
    else{ /* get default address */
	if (getsockname(s, (struct sockaddr *) myaddr, &myaddrlen) == -1) {
	    clicon_err(OE_UNIX, errno, "getsockname"); 
	    goto done;
	}
    }
    if (lport)
	myaddr->sin_port = htons(lport);
    if (bind(s, (struct sockaddr *) myaddr, myaddrlen) < 0) {
	clicon_err(OE_UNIX, errno, "bind");
	goto done;
    }
    /* Get local port (after first packet sent) */
    if (getsockname(s,  (struct sockaddr *)myaddr, &myaddrlen) < 0){
	clicon_err(OE_UNIX, errno, "getsockname");	    
	goto done;
    }
    retval = 0;
 done:
    return retval;
}

/*! Local address and tcp port binding
 * ifaddr given -> use interface address otherwise use default
 * local port given -> use that otherwise ephemeral
 * @param[in]  s       Socket
 * @param[in]  inaddr  IPv4 interface address. Use if set, otherwise default
 * @param[in]  lport   TCP port. Use if set, otherwise ephemeral
 * @param[out] myaddr  Resulting local bound socket
 */
int
socket_bind_tcp(int                 s,
		struct in_addr     *inaddr,
		uint16_t            lport,
		struct sockaddr_in *myaddr)
{
    int                retval = -1;
    socklen_t          myaddrlen = sizeof(*myaddr);

    clicon_log(LOG_DEBUG, "%s:%hu", __FUNCTION__, lport);
    memset(myaddr, 0, myaddrlen);
#ifdef HAVE_SIN_LEN
    myaddr->sin_len = myaddrlen;
#endif
    myaddr->sin_family = AF_INET;
    if (inaddr)
	memcpy(&myaddr->sin_addr, inaddr, sizeof(*inaddr));	
    else{ /* get default address */
	if (getsockname(s, (struct sockaddr *) myaddr, &myaddrlen) == -1) {
	    clicon_err(OE_UNIX, errno, "getsockname"); 
	    goto done;
	}
    }
    if (lport)
	myaddr->sin_port = htons(lport);
    if (bind(s, (struct sockaddr *) myaddr, myaddrlen) < 0) {
	clicon_err(OE_UNIX, errno, "bind");
	goto done;
    }
    if (listen(s, 1) < 0){
	clicon_err(OE_UNIX, errno, "%s: listen", __FUNCTION__);
	return -1;
    }
    /* Get local port (after first packet sent) */
    if (getsockname(s,  (struct sockaddr *)myaddr, &myaddrlen) < 0){
	clicon_err(OE_UNIX, errno, "getsockname");	    
	goto done;
    }
    retval = 0;
 done:
    return retval;
}


/*! Translate from ifname, eg "dr0" to inet IPv4 address.
 * @param[in]  ifname  Name of interface 
 * @param[out] ifaddr  Inet IPv4 address.
 */
int 
ifname2addr(const char     *ifname, 
	    struct in_addr *ifaddr)
{
    int s;
    struct  ifreq  ifr;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0){
	perror("socket");
	return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    (void) strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
	clicon_err(OE_UNIX, errno, "ioctl SIOCGIFADDR %s\n", ifname);
	return -1;
    }
    close(s);
    *ifaddr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    return 0;
}

/*! Translate from hostname to inet IPv4 address
 * @param[in]  ifname  Name of interface 
 * @param[out] haddr   Inet IPv4 address.
 */
int 
host2addr(const char     *hostname,
	  struct in_addr *haddr)
{
    struct hostent *hp;
    int             saddr;

    if ((saddr = inet_addr(hostname)) < 0){
	/* symbolic name */
	hp = gethostbyname(hostname);
	if (hp == 0)
	    return -1;
	memcpy((char*)&saddr, (char*)hp->h_addr_list[0], sizeof(saddr));
    }
    haddr->s_addr = saddr;
    return 0; /* OK */
}

char *
grideye_proto2str(enum grideye_proto proto)
{
    switch(proto){
    case GRIDEYE_PROTO_TCP:
	return "tcp";
	break;
    case GRIDEYE_PROTO_UDP:
	return "udp";
	break;
    case GRIDEYE_PROTO_HTTP:
	return "http";
	break;
    default:
	break;
    }
    return NULL;
}

enum grideye_proto
grideye_str2proto(char *str)
{
    if (strcmp(str, "udp")==0)
	return GRIDEYE_PROTO_UDP;
    else if (strcmp(str, "tcp")==0)
	    return GRIDEYE_PROTO_TCP;
    else if (strcmp(str, "http")==0)
	return GRIDEYE_PROTO_HTTP;
    return -1;
}
