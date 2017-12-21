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

#ifndef _GRIDEYE_AGENT_H_
#define _GRIDEYE_AGENT_H_

/* Inherited from pt twoway protocol */
#define TWOWAY_TAG  0xcf30e506
#define PROTO_VERSION 4

/*
 * Types
 */

/* Message type, in message */
enum mtype{
    MTYPE_CONTROL=0,     /* @see control_hdr: Regular control */
    MTYPE_TWOWAY=8,   /* @see twoway_hdr */
};


/* See pt_2way_req should be 60 bytes (packed). Note that TTL and TOS fields are not 
   aligned. 
   See ds0030_doc_en.pdf
*/
struct twoway_hdr{
    uint8_t  th_ver; /* pt_stream_header */
    uint8_t  th_mtype;
    uint16_t th_label; /* end pt_stream_header */
    uint32_t th_tag;
    uint8_t  th_ver2; /* pt-ping header */
    uint8_t  th_rcode;
    uint8_t  th_ttl; /* actualy [4] */
    uint8_t  th_pad1[3];
    uint8_t  th_tos; /* actualy [4] */
    uint8_t  th_pad2[3];
    uint16_t th_inc;
    uint32_t th_seq0;
    uint32_t th_seq1;
    struct timeval th_t0;
    struct timeval th_t1;
    struct timeval th_t2;
    struct timeval th_t3;
};

/* See pt_2way_req */
struct control_hdr{
    uint8_t   ch_ver;      /* See twoway_hdr, eg 4  */
    uint8_t   ch_mtype;    /* message type. 8 for data two-way */
    uint8_t   ch_pad1;      
    uint8_t   ch_pad2;
    uint32_t  ch_tag2;     /* cf CONTROL_TAG */
    char      ch_xml[0];   /* xml payload */
};

/* Data protocol - how to carry the measurement data */
enum grideye_proto{
  GRIDEYE_PROTO_ERROR=-1,
    GRIDEYE_PROTO_TCP,
    GRIDEYE_PROTO_UDP,
    GRIDEYE_PROTO_HTTP,

};

/*
 * Prototypes
 */
int encode_twoway(char *msg, int pktlen, struct twoway_hdr *th, char *payload);
int decode_twoway(char *msg, int pktlen, struct twoway_hdr *th, char **payload, uint32_t *rlen);
int encode_control(char *msg, int pktlen, struct control_hdr *ch, char *xstr);

int decode_control(char *msg, int pktlen, struct control_hdr *ch, char **payload, uint32_t *rlen);

uint64_t timeval2twamp(struct timeval tv);
int twamp2timeval(uint64_t ts, struct timeval *tv);

//void (*set_signal(int signo, void (*handler)()))();
struct timeval gettimestamp();
char *timevalprint(struct timeval t1);
int  msgdump(FILE *f, char *buf, int len);
int socket_bind_udp(int s, struct in_addr *ifaddr, uint16_t lport, struct sockaddr_in *myaddr);
int socket_bind_tcp(int s, struct in_addr *ifaddr, uint16_t lport, struct sockaddr_in *myaddr);
int ifname2addr(const char *ifname, struct in_addr *ifaddr);
int host2addr(const char *hostname, struct in_addr *haddr);
char *grideye_proto2str(enum grideye_proto proto);
enum grideye_proto grideye_str2proto(char *str);

#endif /* _GRIDEYE_AGENT_H_ */
